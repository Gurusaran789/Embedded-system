
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <cstring>

constexpr const char* CSV_FILE = "shop_data.csv";
constexpr int LCD_ADDR = 0x27;
constexpr const char* SERIAL_DEVICE = "/dev/serial0";
constexpr speed_t PRINTER_BAUD = B9600;

struct Item {
    std::string barcode;
    std::string name;
    int price = 0;
};

struct CartEntry {
    int sno;
    std::string name;
    int qty;
    int price;
};

class SimpleLCD {
    int fd;
public:
    SimpleLCD(): fd(-1) {}
    bool init(int addr = LCD_ADDR) {
        wiringPiSetup();
        fd = wiringPiI2CSetup(addr);
        if (fd < 0) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        write4(0x33,0x08); write4(0x32,0x08);
        cmd(0x28); cmd(0x0C); cmd(0x06); cmd(0x01);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return true;
    }
private:
    void toggleEnable(unsigned char data) {
        wiringPiI2CWrite(fd, data | 0x04);
        delayMicroseconds(500);
        wiringPiI2CWrite(fd, data & ~0x04);
        delayMicroseconds(500);
    }
    void write4(unsigned char data, unsigned char mode) {
        unsigned char high = (data & 0xF0) | mode;
        unsigned char low  = ((data << 4) & 0xF0) | mode;
        wiringPiI2CWrite(fd, high); toggleEnable(high);
        wiringPiI2CWrite(fd, low);  toggleEnable(low);
    }
    void cmd(unsigned char c) { write4(c, 0x08); }
public:
    void clear() { cmd(0x01); delay(2); }
    void setCursor(int row, int col) {
        int row_offsets[] = {0x00,0x40,0x14,0x54};
        cmd(0x80 | (col + row_offsets[row]));
    }
    void writeStringAt(int row, int col, const std::string &s) {
        setCursor(row,col);
        for (char c: s) write4(c, 0x09);
    }
};

class Printer {
    int fd = -1;
public:
    bool init(const char* dev = SERIAL_DEVICE) {
        fd = open(dev, O_RDWR | O_NOCTTY);
        if (fd < 0) return false;
        struct termios options;
        tcgetattr(fd, &options);
        cfsetispeed(&options, PRINTER_BAUD);
        cfsetospeed(&options, PRINTER_BAUD);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        tcsetattr(fd, TCSANOW, &options);
        return true;
    }
    void writeLine(const std::string &s) {
        if (fd < 0) return;
        write(fd, s.c_str(), s.size());
        write(fd, "\n", 1);
    }
    void writeText(const std::string &s) {
        if (fd < 0) return;
        write(fd, s.c_str(), s.size());
    }
    void cut() {
        if (fd < 0) return;
        unsigned char cut_cmd[] = {0x1d, 'V', 1};
        write(fd, cut_cmd, sizeof(cut_cmd));
    }
    void closeDev() { if (fd>=0) ::close(fd); fd=-1; }
    ~Printer(){ closeDev(); }
};

class SmartTrolley {
    std::vector<Item> items;
    std::vector<CartEntry> cart;
    SimpleLCD lcd;
    Printer printer;
    int totalCost = 0;
public:
    SmartTrolley() {}
    void start() {
        lcd.init();
        lcd.clear();
        lcd.writeStringAt(0,0,"Initialising...");
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        loadCSV(CSV_FILE);
        lcd.clear(); lcd.writeStringAt(0,0,"Please Scan...");
        std::cout << "Loaded " << items.size() << " items from " << CSV_FILE << "\n";
        bool prOK = printer.init();
        if (!prOK) std::cout << "Printer not opened. Printing disabled.\n";
        runLoop();
    }
private:
    void loadCSV(const std::string &file) {
        std::ifstream ifs(file);
        if (!ifs.is_open()) {
            std::cout << "CSV not found: " << file << "\n";
            return;
        }
        std::string line;
        while (std::getline(ifs,line)) {
            if (line.size()<3) continue;
            std::istringstream ss(line);
            std::string b,n,p;
            if (!std::getline(ss,b,',')) continue;
            if (!std::getline(ss,n,',')) continue;
            if (!std::getline(ss,p,',')) continue;
            Item it; it.barcode = trim(b); it.name = trim(n); it.price = std::stoi(trim(p));
            items.push_back(it);
        }
    }
    static std::string trim(const std::string &s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a==std::string::npos) return "";
        size_t z = s.find_last_not_of(" \t\r\n");
        return s.substr(a, z-a+1);
    }
    Item* findByBarcode(const std::string &code) {
        for (auto &it: items) if (it.barcode == code) return &it;
        return nullptr;
    }
    void addToCart(Item *it) {
        CartEntry e; e.sno = cart.size()+1; e.name = it->name; e.qty = 1; e.price = it->price;
        cart.push_back(e);
        totalCost += it->price;
    }
    void printReceipt() {
        // build and send
        printer.writeLine("======================");
        auto t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        char buf[128];
        std::snprintf(buf,sizeof(buf),"DATE: %04d-%02d-%02d %02d:%02d:%02d",
                      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        printer.writeLine(buf);
        printer.writeLine("CIRCUIT DIGEST - VSB COLLEGE");
        printer.writeLine("------------------------------");
        printer.writeLine("S.No  ITEM             QTY  PRICE");
        printer.writeLine("------------------------------");
        for (auto &c: cart) {
            char line[128];
            std::snprintf(line,sizeof(line),"%2d    %-15.15s  %2d   %4d",
                          c.sno, c.name.c_str(), c.qty, c.price);
            printer.writeLine(line);
        }
        printer.writeLine("------------------------------");
        std::snprintf(buf,sizeof(buf),"SUBTOTAL: %d", totalCost);
        printer.writeLine(buf);
        printer.writeLine("DISCOUNT: 0");
        printer.writeLine("VAT @0% : 0");
        printer.writeLine("------------------------------");
        std::snprintf(buf,sizeof(buf),"BILL TOTAL: %d", totalCost);
        printer.writeLine(buf);
        printer.writeLine("------------------------------");
        printer.writeLine("THANK YOU");
        printer.cut();
    }
    void runLoop() {
        std::string scode;
        while (true) {
            std::cout << "Scan the barcode: ";
            if (!std::getline(std::cin, scode)) break;
            scode = trim(scode);
            if (scode.empty()) continue;
            if (scode == "8906128542687" || scode == "PRINT") {
                lcd.clear(); lcd.writeStringAt(0,0,"Printing Invoice...");
                printReceipt();
                cart.clear(); totalCost = 0;
                lcd.clear(); lcd.writeStringAt(0,0,"Thanks for Shopping");
                lcd.writeStringAt(1,0,"With Us");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                lcd.clear(); lcd.writeStringAt(0,0,"Please Scan...");
                continue;
            }
            Item* it = findByBarcode(scode);
            if (!it) {
                std::cout << "Unknown Barcode or Item Not Registered\n";
                lcd.clear(); lcd.writeStringAt(0,0,"Item Not Found...");
                lcd.writeStringAt(2,0,"Scan Again...");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                lcd.clear(); lcd.writeStringAt(0,0,"Please Scan...");
                continue;
            }
            addToCart(it);
            std::cout << "Added: " << it->name << " Rs." << it->price << "   Total: " << totalCost << "\n";
            lcd.clear();
            std::string line1 = "Item(s) added";
            lcd.writeStringAt(0,0,line1);
            lcd.writeStringAt(1,0, std::to_string(cart.size()) + ". " + it->name);
            lcd.writeStringAt(3,0, "Cart Total: Rs." + std::to_string(totalCost));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            lcd.clear(); lcd.writeStringAt(0,0,"Please Scan...");
        }
    }
};

int main() {
    SmartTrolley st;
    st.start();
    return 0;
}

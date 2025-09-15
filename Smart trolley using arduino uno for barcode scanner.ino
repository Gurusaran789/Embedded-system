# import sys
#  
# sys.path.insert(0, '/home/pi/Ali Proj/Libraries')
import gspread
from time import sleep
from RPLCD.i2c import CharLCD
lcd = CharLCD('PCF8574', 0x27)
from escpos.printer import Serial
from datetime import datetime
now = datetime.now()
dt_string = now.strftime("%b/%d/%Y %H:%M:%S")
lcd.cursor_pos = (0, 0)
lcd.write_string("Initiallising...")
#locating the spread sheet json file
gc = gspread.service_account(filename='/home/pi/Ali Proj/Proj 4 Shoping Cart with Thermal Printer/Shopping Cart on 20_4 LCD/shop-data-thermal-585dc7bffa1f.json')
#sheet name is to be passed
sh = gc.open("Shop Data for Thermal")
worksheet=sh.get_worksheet(0)
count=0
item_cost=0
totalCost=0
SNo=0
scode=""
qty=1
scodePrev=0
item_name=""
entryF=[]
""" 9600 Baud, 8N1, Flow Control Enabled """
p = Serial(devfile='/dev/serial0',
           baudrate=9600,
           bytesize=8,
           parity='N',
           stopbits=1,
           timeout=1.00,
           dsrdtr=True)
p.set(
    align="center",
    font="a",
    width=1,
    height=1,    
    )
def print_receipt():
    p.text("\n")
    p.set(
            align="center",
            font="a",
            width=1,
            height=1,    
        )
    #Printing the image
    p.image("/home/pi/Ali Proj/Proj 3 Interfacing thermal printer with pi/CD_new_Logo_black.png",impl="bitImageColumn")
    #printing the initial data
    p.set(width=2,
         height=2,
         align="center",)
    p.text(" ===============\n")
    p.text("Tax Invoice\n")
    p.text(" ===============\n")
    p.set(width=1,
         height=1,
         align="left",)
    p.text("CIRCUIT DIGEST\n")
    p.text("VSB COLLEGE\n")
    p.text("LOCATION : KARUR\n")
    p.text("TEL : 0141222585\n")
    p.text("GSTIN : 08AAMFT88558855\n")
    p.text("Bill No. : \n\n")
    p.text("DATE : ")
    p.text(dt_string)
    p.text("\n")
    p.text("CASHIER : \n")
    p.text(" ===========================\n")
    p.text("S.No     ITEM   QTY   PRICE\n")
    p.text(" -------------------------------\n")
    print(text_F)
    p.text(text_F)
    p.text(" -------------------------------\n")
    p.set(
            # underline=0,
            align="right",
         )
    p.text("     SUBTOTAL:  ")
    p.text(totalCostS)
    p.text("\n")          
    p.text("     DISCOUNT:  0\n")
    p.text("     VAT @ 0%: 0\n")
    p.text(" ===========================\n")
    p.set(align="center",  
         )
    p.text("    BILL TOTAL: ")
    p.text(totalCostS)
    p.text("\n")
    p.text(" --------------------------\n")
    p.text("THANK YOU\n")   
    p.set(width=2,
         height=2,
         align="center",)
    p.text(" ===============\n")
    p.text("Please scan\nto Pay\n")
    p.text(" ===============\n")
    p.set(
            align="center",
            font="a",
            width=1,
            height=1,
            density=2,
            invert=0,
            smooth=False,
            flip=False,      
        )
    p.qr("9509957951@ybl",native=True,size=12)
    p.text("\n")
    p.barcode('123456', 'CODE39')
    #if your printer has paper cuting facility then you can use this function
    p.cut()
    print("prnting done")   
lcd.cursor_pos = (0, 0)
lcd.write_string('Please Scan...')
while 1:
    try:
        scode=input("Scan the barcode")       
        if scode=="8906128542687": #Bill Printing Barcode
            print("done shopping ")
            lcd.clear()
            print(*entryF)
            print("in string")
            print(len(entryF))
            text_F=" "
            for i in range(0,len(entryF)):
                text_F=text_F+entryF[i]
                i=i+1           
            lcd.cursor_pos = (0, 0)
            lcd.write_string("Thanks for Shopping")           
            lcd.cursor_pos = (1, 7)
            lcd.write_string("With Us")           
            lcd.cursor_pos = (3, 0)
            lcd.write_string("Printing Invoice...")
            print_receipt() 
        else:
            cell=worksheet.find(scode)
            print("found on R%sC%s"%(cell.row,cell.col))
            item_cost = worksheet.cell(cell.row, cell.col+2).value
            item_name = worksheet.cell(cell.row, cell.col+1).value
            lcd.clear()
            SNo=SNo+1   
            entry = [SNo,item_name,qty,item_cost]
            entryS=str(entry)+'\n'
            print("New Item ",*entry)
            lcd.cursor_pos = (0, 2)
            lcd.write_string(str(SNo))
            lcd.cursor_pos = (0, 5)
            lcd.write_string("Item(s) added")           
            lcd.cursor_pos = (1, 1)
            lcd.write_string(item_name)           
            lcd.cursor_pos = (2, 5)
            lcd.write_string("of Rs.")
            lcd.cursor_pos = (2, 11)
            lcd.write_string(item_cost)           
            item_cost=int(item_cost)
            totalCost=item_cost+totalCost           
            lcd.cursor_pos = (3, 4)
            lcd.write_string("Cart Total")           
            lcd.cursor_pos = (3, 15)
            lcd.write_string(str(totalCost))           
            entryF.append(entryS)   #adding entry in Final Buffer
            sleep(2)           
    except:
        print("Unknown Barcode or Item Not Registered")
        lcd.clear()
        lcd.cursor_pos = (0, 0)
        lcd.write_string("Item Not Found...")
        lcd.cursor_pos = (2, 0)
        lcd.write_string("Scan Again...")
        sleep(2)

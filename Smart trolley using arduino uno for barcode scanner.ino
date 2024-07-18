#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize the LCD with the I2C address (usually 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Array of commands to display
const char* commands[] = {
  "total prize = 85",
  "Scan the Products",
  "Dermi COOL Rs50",
  "Fair&Lovely Rs30",
  "HiFi Rs5 ",
  "Print Products",
  "total item = 3",
  "total prize = 85"
};

const int numCommands = 8;
const int displayTime = 4000; // Time in milliseconds to display each command

void setup() {
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  // Print the first command to the LCD
  lcd.print(commands[1]);
}

void loop() {
  for (int i = 1; i < numCommands; i++) {
    // Clear the display
    lcd.clear();
    // Print the current command
    lcd.print(commands[i]);
    // Wait for the specified display time
    delay(displayTime);
  }
  
  // Clear the display and print the first command again
  lcd.clear();
  lcd.print(commands[0]);
  
  // Stop the loop
  while (true) {
    // Do nothing
  }
}

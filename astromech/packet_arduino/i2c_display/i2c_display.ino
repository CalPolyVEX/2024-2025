/**
 * @file i2c_display.ino
 * @brief This code is just to make sure the LCD is functioning correctly
 * It also shows some cool stuff which can be done with the LCD.
 */

#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip

// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

byte battery[8] = {
    0b01110,
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111};

uint8_t rightarrow[8] = {
    0b01000,
    0b00100,
    0b00010,
    0b11111,
    0b11111,
    0b00010,
    0b00100,
    0b01000};

uint8_t leftarrow[8] = {
    0b00010,
    0b00100,
    0b01000,
    0b11111,
    0b11111,
    0b01000,
    0b00100,
    0b00010};

uint8_t uparrow[8] = {
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100};

uint8_t downarrow[8] = {
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b10101,
    0b01110,
    0b00100};

void setup() {
    int status;

    // initialize LCD with number of columns and rows:
    // hd44780 returns a status from begin() that can be used
    // to determine if initalization failed.
    // the actual status codes are defined in <hd44780.h>
    // See the values RV_XXXX
    //
    // looking at the return status from begin() is optional
    // it is being done here to provide feedback should there be an issue
    //
    // note:
    //  begin() will automatically turn on the backlight
    //
    status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status) // non zero status means it was unsuccesful
    {
        // hd44780 has a fatalError() routine that blinks an led if possible
        // begin() failed so blink error code using the onboard LED if possible
        hd44780::fatalError(status); // does not return
    }

    // initalization was successful, the backlight should be on now

    // Print a message to the LCD
}

void loop() {
    // Creating char
    static unsigned battery_lvl = 0;
    lcd.createChar(0, battery);
    lcd.createChar(1, rightarrow);

    lcd.print("Astromech");
    /* lcd.setCursor(2, 2);
    lcd.print('\001'); */
    switch (battery_lvl % 4) {
    case 0:
        right_arrow();
        break;

    case 1:
        left_arrow();
        break;

    case 2:
        up_arrow();
        break;

    case 3:
        down_arrow();
        break;

    default:
        break;
    }

    output_battery(battery_lvl, 10);
    delay(2000);
    lcd.clear();
    battery_lvl = (battery_lvl + 1) % 10;
}

void right_arrow() {
    lcd.createChar(1, rightarrow);
    lcd.setCursor(LCD_COLS - 1, 2);
    lcd.print('\001');
}

void left_arrow() {
    lcd.createChar(2, leftarrow);
    lcd.setCursor(LCD_COLS - 1, 2);
    lcd.print('\002');
}

void up_arrow() {
    lcd.createChar(3, uparrow);
    lcd.setCursor(LCD_COLS - 1, 2);
    lcd.print('\003');
}

void down_arrow() {
    lcd.createChar(4, downarrow);
    lcd.setCursor(LCD_COLS - 1, 2);
    lcd.print('\004');
}

/**
 * @brief Prints battery level in top right corner.
 *
 *
 * @param battery_level current voltage of battery.
 * @param out_of voltage of battery when full.
 */
void output_battery(unsigned battery_level, unsigned out_of) {
    // battery level from 0 to 5
    int i;
    int out_of_5 = (double)battery_level / (double)out_of * 5.0;
    for (int i = 6; i > 1; i--) {
        battery[i] = 0b10001;
    }
    for (int i = 6; i > 6 - out_of_5; i--) {
        battery[i] = 0b11111;
    }
    lcd.createChar(0, battery);
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.print('\000');
}

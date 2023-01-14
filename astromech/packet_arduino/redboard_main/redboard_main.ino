#include "redboard_main.h"

hd44780_I2Cexp lcd(LCD_ADDRESS); // declare lcd object: auto locate & auto config expander chip

void setup() {

    // Set up the motors
    motor_setup();
    receiver_setup();
    int status;
    status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status) // non zero status means it was unsuccesful
    {
        // hd44780 has a fatalError() routine that blinks an led if possible
        // begin() failed so blink error code using the onboard LED if possible
        hd44780::fatalError(status); // does not return
    }

    // Setup LED Controller
    setupLED();

    // Set up the USB reading
    //SerialUSB.begin(9600);
    //SerialUSB.setTimeout(0);
}

void loop() {
    // if (rc_toggle) {
    // eval_rc_input()
    // } else {}

    // Computer Input Mode
    get_input();

    // Receiver Mode
    receiver_loop();
}

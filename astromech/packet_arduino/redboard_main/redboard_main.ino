#include "redboard_main.h"

hd44780_I2Cexp lcd(LCD_ADDRESS); // declare lcd object: auto locate & auto config expander chip

void setup() {



    // Set up the motors
    /*
    motor_setup();
    receiver_setup();
    int status;
    status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status) // non zero status means it was unsuccesful
    {
        // hd44780 has a fatalError() routine that blinks an led if possible
        // begin() failed so blink error code using the onboard LED if possible
        hd44780::fatalError(status); // does not return
    }*/

    // Setup LED Controller
    setupLED();

    // Set up the USB reading
    //SerialUSB.begin(9600);
    //SerialUSB.setTimeout(0);

    //PORT->Group[0].DIRSET.reg |= PORT_PA17;
    //PORT->Group[0].OUT.reg = PORT_PA17;
}

void loop() {
    // if (rc_toggle) {
    // eval_rc_input()
    // } else {}

    // Computer Input Mode
    // get_input();


    unsigned int end_time = millis() + 5000;
    while (millis() < end_time) {}
    char bytes[5] = {'@', '0', 'T', '3', 13};
    sendLEDCommand((byte*)bytes, 5);

    unsigned int end_time2 = millis() + 5000;
    while (millis() < end_time2) {}
    char bytes2[5] = {'@', '0', 'T', '2', 13};
    sendLEDCommand((byte*)bytes2, 5);

    unsigned int end_time3 = millis() + 5000;
    while (millis() < end_time3) {}
    char bytes3[5] = {'@', '0', 'T', '6', 13};
    sendLEDCommand((byte*)bytes3, 5);

    // Receiver Mode
    //receiver_loop();

}

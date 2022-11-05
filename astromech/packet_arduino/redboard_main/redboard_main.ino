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

    // Set up the USB reading
    SerialUSB.begin(115200);
    SerialUSB.setTimeout(0);
}

void loop() {
    // static int i = 0;
    // if (rc_toggle) {
    // eval_rc_input()
    // } else {}

    // Computer Input Mode
    // pc_get_input();
    // receiver_loop();

    // Receiver Mode
    pc_get_input(receiver_loop());
    // lcd.write(i++);

    /* NOTE: data cannot be sent by serial from PC
    to arduino if the VSCode serial monitor is open*/

    // if(!receiver_loop()) {
    //     digitalWrite(LED_BUILTIN, 1);
    //     get_input();
    // } 
    // else {
    //     if (SerialUSB.available()) {
    //         SerialUSB.read();
    //     }
    // }

    // defaults to Receiver Mode
    // but can switch to Computer Input Mode
    // static uint8_t prev=2, cur;
    // cur = receiver_loop();
    // if((prev != cur && prev != 2) || (prev == 2 && cur == 1)) {
    //     if(!prev) { // changing from RC to PC
    //         SerialUSB.end();
    //     } else { // staying on PC
    //         SerialUSB.begin(115200);
    //     }
    // }
    // if(!cur) {
    //     get_input();
    // }
    // prev = cur;
}
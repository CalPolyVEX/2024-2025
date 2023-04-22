#include "packet_arduino.h"

hd44780_I2Cexp lcd(LCD_ADDRESS); // declare lcd object: auto locate & auto config expander chip
void setup() {
    delay(1000); // give the computer to detect the board before using serial

    // Set up the serial USB
    SerialUSB.begin(115200);
    SerialUSB.setTimeout(0);
    SerialUSB.println("---- Board reset ----");

    // Setup Wire
    Wire.begin();

    // Set up the motors
    motor_setup();
    receiver_setup();

    int lcd_status;
    lcd_status = lcd.begin(LCD_COLS, LCD_ROWS);
    // non zero status means it was unsuccesful
    if (lcd_status) {
        lcd.clear();
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print("Astromech");
    }*/

    // init for onboard LEDs
    setup_i2c();

    // Setup Logic Engine Controller
    setupLogicEngine();

    // Initialize the Amplifier for Sound
    resetAmplifier();
    setAmplifierGain(30);

    // startup blink
    for (int j = 0; j < 2; j++) {
        for (int i = 1; i <= 4; i++) {
            led_on(i);
            delay(100);
            led_off(i);
        }
    }

    for (int i = 5; i < 15; i++) {
        uint8_t val = btn_read(i);
        lcd.clear();
        delay(1000);
        lcd.setCursor(0, 0);
        lcd.print(i);
        lcd.print(" ");
        lcd.print(val);
    }

    // check for debug mode activation
    int debug_status = btn_read(DB1); // get DB1 value
    if(debug_status && lcd_status) {
        delay(1000);
        lcd.setCursor(1, 0);
        lcd.print("Debug Mode Initiated");
        debug_loop();
        exit(0);
    } else {
        delay(1000);
        lcd.setCursor(1, 0);
        lcd.print("Receiver Mode Initiated");
    }
}

void loop() {

    /* NOTE: data cannot be sent by serial from PC
    to arduino if the VSCode serial monitor is open*/

    // Receiver Mode
    pc_get_input(receiver_loop());

}

// old loop() contents
    // if (rc_toggle) {
        // eval_rc_input()
    // } else {}

    // Computer Input Mode
    // pc_get_input();
    // receiver_loop();
    // if(i == 0) {
    //     send_reon_command(REON_WHITE, 27);
    //     delay(3000);
    //     i++;
    // } else if(i == 1) {
    //     send_reon_command(REON_OFF, 27);
    //     delay(3000);
    //     i++;
    // } else if(i == 2) {
    //     // send_reon_command(REON_ON, 27);
    //     // delay(3000);
    //     i = 0;
    // }

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
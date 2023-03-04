#include "packet_arduino.h"

hd44780_I2Cexp lcd(LCD_ADDRESS); // declare lcd object: auto locate & auto config expander chip

void setup() {

    // Setup Wire
    Wire.begin();

    // Set up the motors
    motor_setup();
    receiver_setup();
#ifdef LCD_DEBUG
    int status;
    status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status) // non zero status means it was unsuccesful
    {
        // hd44780 has a fatalError() routine that blinks an led if possible
        // begin() failed so blink error code using the onboard LED if possible
        hd44780::fatalError(status); // does not return
    }
#endif

    // init I2C for REON holoprojectors
    Wire.begin();

    // init for onboard LEDs
    setup_i2c();

    // Setup Logic Engine Controller
    setupLogicEngine();

    // Set up the USB reading
    SerialUSB.begin(115200);
    SerialUSB.setTimeout(0);

    // Initialize the Amplifier for Sound
    resetAmplifier();
    setAmplifierGain(30);
}

void loop() {
    static int i = 0;
    // if (rc_toggle) {
    // eval_rc_input()
    // } else {}

    // Computer Input Mode
    // pc_get_input();
    // receiver_loop();

    // Receiver Mode
    pc_get_input(receiver_loop());

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
}
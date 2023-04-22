#include "i2c_pin_led.h"

TCA9534 ioex;
// const uint8_t IOEX_ADDR = 0x20; // A0 = A1 = A2 = 0

void setup_i2c() {
    Wire.setClock(400000); // set clock to 400KHz
    ioex.attach(Wire);
    ioex.setDeviceAddress(IOEX_ADDR); // A0 = A1 = A2 = 0
    ioex.config(TCA9534::Config::IN); // set all pins to input

    // LEDs
    ioex.config(3, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(3, TCA9534::Level::H);    // turn off LED1

    ioex.config(2, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(2, TCA9534::Level::H);    // turn off LED2

    ioex.config(1, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(1, TCA9534::Level::H);    // turn off LED3

    ioex.config(0, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(0, TCA9534::Level::H);    // turn off LED4

    // Debug Buttons
    ioex.config(4, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(4, TCA9534::Level::H);    // turn off debug button 1

    ioex.config(5, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(5, TCA9534::Level::H);    // turn off debug button 2

    ioex.config(6, TCA9534::Config::OUT); // change port P1 to output
    ioex.output(6, TCA9534::Level::H);    // turn off debug button 2

    //   ioex.config(7, TCA9534::Config::OUT); // set backlight control pin
}

// void backlight_on() {
//   ioex.output(7, TCA9534::Level::H);
// }

// void backlight_off() {
//   ioex.output(7, TCA9534::Level::L);
// }

void led_on(int num) {
    if (num == 4)
        ioex.output(0, TCA9534::Level::L);
    else if (num == 3)
        ioex.output(1, TCA9534::Level::L);
    else if (num == 2)
        ioex.output(2, TCA9534::Level::L);
    else if (num == 1)
        ioex.output(3, TCA9534::Level::L);
    // ioex.output(num, TCA9534::Level::L);
}

void led_off(int num) {
    if (num == 4)
        ioex.output(0, TCA9534::Level::H);
    else if (num == 3)
        ioex.output(1, TCA9534::Level::H);
    else if (num == 2)
        ioex.output(2, TCA9534::Level::H);
    else if (num == 1)
        ioex.output(3, TCA9534::Level::H);
    //    ioex.output(num, TCA9534::Level::H);
}
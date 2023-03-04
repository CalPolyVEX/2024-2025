#include "i2c_pin_led.h"

TCA9534 ioex;
// const uint8_t IOEX_ADDR = 0x20; // A0 = A1 = A2 = 0

void setup_i2c() 
{
  Wire.setClock(400000); //set clock to 400KHz
  ioex.attach(Wire);
  ioex.setDeviceAddress(IOEX_ADDR); // A0 = A1 = A2 = 0
  ioex.config(TCA9534::Config::IN); // set all pins to input

  ioex.config(LED1, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(LED1, TCA9534::Level::H); // turn off LED1
  
  ioex.config(LED2, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(LED2, TCA9534::Level::H); // turn off LED2
  
  ioex.config(LED3, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(LED3, TCA9534::Level::H); // turn off LED3

  ioex.config(LED4, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(LED4, TCA9534::Level::H); // turn off LED4

//   ioex.config(7, TCA9534::Config::OUT); // set backlight control pin
}

// void backlight_on() {
//   ioex.output(7, TCA9534::Level::H);
// }

// void backlight_off() {
//   ioex.output(7, TCA9534::Level::L);
// }

void led_on(int num) {
  // if (num == 4)
  //   ioex.output(0, TCA9534::Level::L);
  // else if (num == 3)
  //   ioex.output(1, TCA9534::Level::L);
  // else if (num == 2)
  //   ioex.output(2, TCA9534::Level::L);
  // else if (num == 1)
  //   ioex.output(3, TCA9534::Level::L);
  ioex.output(num, TCA9534::Level::L);
}

void led_off(int num) {
  // if (num == 4)
  //   ioex.output(0, TCA9534::Level::H);
  // else if (num == 3)
  //   ioex.output(1, TCA9534::Level::H);
  // else if (num == 2)
  //   ioex.output(2, TCA9534::Level::H);
  // else if (num == 1)
  //   ioex.output(3, TCA9534::Level::H);
  ioex.output(num, TCA9534::Level::H);
}
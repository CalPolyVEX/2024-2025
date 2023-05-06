// This is to fix the reference of Serial in TCA9534
#define Serial SerialUSB
#include <TCA9534.h>

#include <Wire.h>

#define LED1 1
#define LED2 2
#define LED3 3
#define LED4 4

#define IOEX_ADDR (const uint8_t) 0x38

void setup_i2c();
void led_on(int num);
void led_off(int num);
#include <TCA9534.h>
#include <Wire.h>

#define LED1 3
#define LED2 2
#define LED3 1
#define LED4 0

void setup_i2c();
void led_on(int num);
void led_off(int num);
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "redboard_main.h"

// Setup the USART for the LED Controller
void setupLED();

// Send Command to LED Controller
void sendLEDCommand(uint8_t* package, uint8_t package_size);

#endif
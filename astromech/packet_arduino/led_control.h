#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "packet_arduino.h"

// Setup the USART for the LED Controller
void setupLED();

// Send Command to LED Controller
void sendLEDCommand(uint8_t command_major, uint8_t command_minor, uint8_t color,
                    uint8_t speed);
void sendLEDCommand(uint8_t preset_index);

#endif
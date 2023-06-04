#ifndef LOGIC_ENGINE_CONTROL_H
#define LOGIC_ENGINE_CONTROL_H

#include "packet_arduino.h"

// Setup the USART for the Logic Engine Controller
void setupLogicEngine();

// Send Command to LED Controller
void sendLogicEngineCommand(uint8_t command_major, uint8_t command_minor, uint8_t color,
                            uint8_t speed);
void sendLogicEngineCommand(uint8_t preset_index);
void sendLogicEngineString(char* str, int display_num);

void update_logic_engine_debug(int value);

#endif
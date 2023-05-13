//#include <TCA9534.h>
#include <Wire.h>
#include "nvm_control.h"

// debug buttons
#define DB0 4   // decrementor button
#define DB1 5   // incrementor button
#define DB2 6   // selector button

// The number of debug functions
#define DEBUG_MODE_COUNT 9

// debug function IDs
#define MIN_MAX_MOTOR_SPEED_DEBUG 0   // motor speed minmax debug
#define MOTOR_SPEED_SCALAR_DEBUG 1   // motor speed scalar debug
#define MOTOR_SPEED_BIAS_DEBUG 2     // motor speed bias debug
#define MIN_MAX_SERVO_SPEED_DEBUG 3   // servo speed minmax debug
#define SERVO_SPEED_SCALAR_DEBUG 4   // servo speed scalar debug
#define LOGIC_ENGINE_DEBUG 5   // logic engine debug
#define SOUNDBOARD_DEBUG 6   // soundboard debug
#define HOLOPROJECTOR_DEBUG 7   // holoprojector debug
#define PSI_DEBUG 8  // PSI debug

#define TRANS 9 // transitioning between functions

#define IOEX_ADDR (const uint8_t)0x20

// functions
void initialize_debug();

void reset_to_default();

uint8_t get_btn_val(int num);
uint8_t btn_read(int num, int& btn_delay);
void debug_loop();


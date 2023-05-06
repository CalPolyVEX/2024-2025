//#include <TCA9534.h>
#include <Wire.h>
#include "nvm_control.h"

// debug buttons
#define DB0 4
#define DB1 5
#define DB2 6

// debug function IDs
#define TRANS 0 // transitioning between functions
#define MSD 1   // motor speed debug
#define SSD 2   // servo speed debug
#define LED 3   // logic engine debug
#define SBD 4   // soundboard debug
#define HPD 5   // holoprojector debug
#define PSID 6  // PSI debug

#define IOEX_ADDR (const uint8_t)0x20

// functions
void initialize_debug();

void reset_to_default();

uint8_t btn_read(int num);
void debug_loop();


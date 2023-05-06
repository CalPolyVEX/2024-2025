//#include <TCA9534.h>

#include <Wire.h>

#define DB1 5
#define DB2 6
#define DB3 7

#define IOEX_ADDR (const uint8_t)0x20

void initialize_debug();

void reset_to_default();

uint8_t btn_read(int num);
void debug_loop();


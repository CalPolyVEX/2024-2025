#include <Wire.h>

// REON holoprojector I2C addresses
#define HP_FRNT_ADDR 25
#define HP_TOP_ADDR 26
#define HP_REAR_ADDR 27

// REON I2C commands
#define REON_OFF 1   // off
#define REON_WHITE 2 // on w/ solid white color
#define REON_ON 3    // on w/ random colors

void send_reon_command(int cmd, int hp_addr);

// Holoprojector Debug
void update_holoprojector_debug(int value);
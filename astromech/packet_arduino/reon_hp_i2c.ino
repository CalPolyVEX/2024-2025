#include "reon_hp_i2c.h"



void send_reon_command(int cmd, int hp_addr) {
    /* send commands via I2C to the REON holoprojector
    with address `hp_addr`*/

    Wire.beginTransmission(hp_addr);

    Wire.write(uint8_t(cmd));

    Wire.endTransmission(true);
}

// Holoprojector Debug
void update_holoprojector_debug(int value)
{
    // Get the Address and the Color
    int address = floor(value / 3);
    int color = value - (3 * address);

    // Send the Command
    send_reon_command(color + REON_OFF, address + HP_FRNT_ADDR);
}
#include "reon_hp_i2c.h"

void send_reon_command(int cmd, int hp_addr) {
    /* send commands via I2C to the REON holoprojector
    with address `hp_addr`*/

    Wire.beginTransmission(hp_addr);

    Wire.write(uint8_t(cmd));

    Wire.endTransmission(true);
}
#ifndef RC_DATA_COLLECTION_H
#define RC_DATA_COLLECTION_H

#include "i2c_pin_led.h"
#include "packet_arduino.h"
#include "pc_decoder.h"
#include "rc_decoder.h"
#include "reon_hp_i2c.h"

class Queue;

void receiver_setup();
bool receiver_loop();
void decodeData();

#endif
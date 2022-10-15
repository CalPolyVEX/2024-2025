#ifndef RC_DATA_COLLECTION_H
#define RC_DATA_COLLECTION_H

#include "rc_decoder.h"
#include "redboard_main.h"

class Queue;

void receiver_setup();
uint8_t receiver_loop();
void decodeData(); 

#endif
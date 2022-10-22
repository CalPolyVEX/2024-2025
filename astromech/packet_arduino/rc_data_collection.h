#ifndef RC_DATA_COLLECTION_H
#define RC_DATA_COLLECTION_H

#include "rc_decoder.h"
#include "redboard_main.h"

class Queue;

void receiver_setup();
void receiver_loop();
void decodeData(); 

#endif
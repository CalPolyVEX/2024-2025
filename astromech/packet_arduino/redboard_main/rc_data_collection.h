#ifndef RC_DATA_COLLECTION_H
#define RC_DATA_COLLECTION_H

#include "rc_decoder.h"
#include "redboard_main.h"
#include "pc_decoder.h"

class Queue;

void receiver_setup();
bool receiver_loop();
void decodeData(); 

#endif
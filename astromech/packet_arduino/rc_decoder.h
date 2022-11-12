#ifndef RC_DECODER_H
#define RC_DECODER_H

#include "motor_servo_control.h"
#include "packet_arduino.h"

// mapped indices to the global array
#define RIGHT_STICK_CHANNEL_HOR 0
#define RIGHT_STICK_CHANNEL_VER 1

void eval_rc_input();
void control_motors(uint8_t hor_val, uint8_t ver_val);

#endif
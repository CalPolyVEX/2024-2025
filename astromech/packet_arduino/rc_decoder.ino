#include "rc_decoder.h"

void control_motors(byte ver_val, byte hor_val) {
    // These calculations might need to be
    // changed based on calibrations!!
    // center both values at 0
    int16_t hor_val_origin = hor_val - 127;
    int16_t left_motor = (int16_t)ver_val + hor_val_origin;
    int16_t right_motor = (int16_t)ver_val - hor_val_origin;

    // keep values within bounds
    // The Clamping of Values is Now Done in the Motor Servo Control

    // ver_val represents throttle, hor_val represents steering

    // set left motor: throttle + steering
    change_motor_speed(0, left_motor - 127);
    // set right motor: throttle - steering
    change_motor_speed(1, right_motor - 127);
}

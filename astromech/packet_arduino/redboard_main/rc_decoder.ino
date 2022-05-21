#include "rc_decoder.h"

// This might be moved to another file
uint8_t rc_array[12];

void eval_rc_input() {
    control_motors(rc_array[RIGHT_STICK_CHANNEL_HOR],
                   rc_array[RIGHT_STICK_CHANNEL_VER]);
}

void control_motors(uint8_t hor_val, uint8_t ver_val) {
    // center both values at 0
    int16_t hor_val_origin = hor_val - 127;
    int16_t ver_val_origin = ver_val - 127;

    // ver_val represents throttle, hor_val represents steering

    // set left motor: throttle + steering
    change_motor_speed(0, ver_val_origin + hor_val_origin);
    // set right motor: throttle - steering
    change_motor_speed(1, ver_val_origin - hor_val_origin);
}

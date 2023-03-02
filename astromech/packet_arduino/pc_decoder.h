#ifndef PC_DECODER_H
#define PC_DECODER_H

#include "logic_engine_control.h"
#include "motor_servo_control.h"
#include "packet_arduino.h"
#include "reon_hp_i2c.h"
#include "tsunami_control.h"

#define MOTOR_TIMOUT 1000 // ms

bool check_crc(uint8_t *data, int len);
unsigned short commandChecker(char *data);
void eval_input(uint8_t *data, int size, bool pc_mode);
void pc_get_input(bool timeout);
void pc_dump_input();
// void get_input();

#endif
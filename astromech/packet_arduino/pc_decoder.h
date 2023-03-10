#ifndef PC_DECODER_H
#define PC_DECODER_H

#include "logic_engine_control.h"
#include "motor_servo_control.h"
#include "packet_arduino.h"
#include "reon_hp_i2c.h"
#include "tsunami_control.h"

#define MOTOR_TIMEOUT 1000 // ms

/* command IDs*/
// #define LEFT_MOTOR_CMD 0
// #define RIGHT_MOTOR_CMD 1
// #define SERVO_CMD 2
// #define LCD_CURSOR_CMD 3
// #define LCD_PRINT_STR_CMD 4
// #define LCD_CURSOR_PRINT_CMD 5
// #define LCD_CLEAR_CMD 6
// #define LOGIC_PRESET_CMD 7
// #define LOGIC_RAW_CMD 8
// #define TSUN_SOUND_CMD 9
// #define TSUN_AMP_CMD 10
// #define REON_CMD 11

bool check_crc(uint8_t *data, int len);
unsigned short commandChecker(char *data);
void eval_input(uint8_t *data, int size, bool pc_mode);
void pc_get_input(bool timeout);
void pc_dump_input();
// void get_input();

#endif

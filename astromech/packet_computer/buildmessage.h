#ifndef BUILD_MESSAGE_H
#define BUILD_MESSAGE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* packet size */
#define MIN_PACKET_SIZE 5
#define MAX_PACKET_SIZE 26
#define ARDUINO_PACKET_SIZE 8

/* payload size */
#define MOTOR_CTRL_PAYLOAD 1
#define SERVO_CTRL_PAYLOAD 2
#define SET_LCD_PAYLOAD 1
#define LOGIC_PRESET_PAYLOAD 1
#define LOGIC_RAW_PAYLOAD 4
#define TSUN_SOUND_PAYLOAD 2
#define TSUN_AMP_PAYLOAD 1
#define REON_PAYLOAD 2

/* File Descriptor Initialization */
int init_serial();

/* reading data from arduino */
void read_arduino_data();
void get_input_buffer(uint8_t** buffer, int** size);

/* packet construction */
uint8_t set_motor(int direction, uint8_t speed);
uint8_t set_servo(uint8_t servo_num, uint8_t position);
uint8_t set_cursor_lcd(uint8_t col, uint8_t row);
uint8_t print_string(unsigned num_chars, uint8_t *string);
uint8_t print_string_at(uint8_t col, uint8_t row, unsigned num_chars, uint8_t *string);
uint8_t clear_lcd();
uint8_t logic_preset_cmd(uint8_t preset_index);
uint8_t logic_raw_cmd(uint8_t command_major, uint8_t command_minor, uint8_t color, uint8_t speed);
uint8_t tsunami_sound_cmd(uint8_t preset_index, int8_t volume);
uint8_t tsunami_amp_cmd(uint8_t gain);
uint8_t set_reon(uint8_t reon_addr, uint8_t reon_state);

/* writing packet to file */
void send_set_motor(int direction, int8_t speed);
void send_set_servo(uint8_t servo_num, uint8_t position);
void send_set_lcd(uint8_t col, uint8_t row);
void send_print_string(uint8_t *s);
void send_print_string_at(uint8_t col, uint8_t row, uint8_t *s);
void send_clear_lcd();
void send_led_preset_cmd(uint8_t preset_idx);
void send_tsun_sound_cmd(uint8_t preset_idx, uint8_t volume);
void send_tsun_amp_cmd(uint8_t gain);
void send_set_reon(uint8_t reon_addr, uint8_t reon_state);

void insert_header(uint8_t payload_len, uint8_t cmd);

/* checksum */
uint8_t *calc_crc(int max_ind);

#endif

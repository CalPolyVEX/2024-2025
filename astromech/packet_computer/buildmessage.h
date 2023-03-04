#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint8_t *set_motor(int direction, uint8_t speed);
uint8_t *set_servo(uint8_t servo_num, uint8_t position);
uint8_t *set_lcd(uint8_t col, uint8_t row);
uint8_t *print_string(unsigned num_chars, uint8_t *string);
uint8_t *print_string_at(uint8_t col, uint8_t row, unsigned num_chars, uint8_t *string);
uint8_t *clear_lcd();
uint8_t *led_preset_cmd(uint8_t preset_index);
void send_set_motor(int direction, uint8_t speed, int fd);
void send_set_servo(uint8_t servo_num, uint8_t position, int fd);
void send_set_lcd(uint8_t col, uint8_t row, int fd);
void send_print_string(uint8_t *s, int fd);
void send_print_string_at(uint8_t col, uint8_t row, uint8_t *s, int fd);
void send_clear_lcd(int fd);
void send_led_preset_cmd(uint8_t preset_idx, int fd);
uint8_t *calc_crc(int max_ind);

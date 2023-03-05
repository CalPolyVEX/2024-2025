#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* packet size */
#define MIN_PACKET_SIZE 5
#define MAX_PACKET_SIZE 26

/* payload size */
#define MOTOR_CTRL_PAYLOAD 1
#define SERVO_CTRL_PAYLOAD 2
#define SET_LCD_PAYLOAD 1
#define LED_PRESET_PAYLOAD 1
#define LED_RAW_PAYLOAD 4
#define TSUN_SOUND_PAYLOAD 2
#define TSUN_AMP_PAYLOAD 1
#define REON_PAYLOAD 2

/* command IDs */
#define MOTOR_COMMAND_LEFT 0
#define MOTOR_COMMAND_RIGHT 1
#define SERVO_CMD 2
#define SET_CURSOR_CMD 3
#define PRINT_STR_CMD 4
#define LCD_PRINT_STR 5
#define LCD_CLR_CMD 6
#define LED_PRESET_CMD 7
#define LED_RAW_CMD 8
#define TSUN_SOUND 9
#define TSUN_AMP 10
#define REON_CMD 11

#define DEV_F_NAME "/dev/cu.usbmodem144101"

/* packet construction */
uint8_t *set_motor(int direction, uint8_t speed);
uint8_t *set_servo(uint8_t servo_num, uint8_t position);
uint8_t *set_lcd(uint8_t col, uint8_t row);
uint8_t *print_string(unsigned num_chars, uint8_t *string);
uint8_t *print_string_at(uint8_t col, uint8_t row, unsigned num_chars, uint8_t *string);
uint8_t *clear_lcd();
uint8_t *led_preset_cmd(uint8_t preset_index);
uint8_t *set_reon(uint8_t reon_addr, uint8_t reon_state);

/* writing packet to file */
void send_set_motor(int direction, uint8_t speed, int fd);
void send_set_servo(uint8_t servo_num, uint8_t position, int fd);
void send_set_lcd(uint8_t col, uint8_t row, int fd);
void send_print_string(uint8_t *s, int fd);
void send_print_string_at(uint8_t col, uint8_t row, uint8_t *s, int fd);
void send_clear_lcd(int fd);
void send_led_preset_cmd(uint8_t preset_idx, int fd);
void send_set_reon(uint8_t reon_addr, uint8_t reon_state, int fd);

/* checksum */
uint8_t *calc_crc(int max_ind);

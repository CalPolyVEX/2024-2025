#include "PacketCRC.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOTOR_CONT_PAYLOAD 1
#define SERVO_CONT_PAYLOAD 2
#define SET_LCD_PAYLOAD 1

#define MIN_PACKET_SIZE 5

#define MOTOR_COMMAND_LEFT 0
#define MOTOR_COMMAND_RIGHT 1
#define SERVO_CMD 2
#define SET_CURSOR_CMD 3
#define PRINT_STR_CMD 4

uint8_t *set_motor(int direction, uint8_t speed);
uint8_t *set_servo(uint8_t servo_num, uint8_t position);
uint8_t *set_LCD(uint8_t col, uint8_t row);
uint8_t *print_string(unsigned num_uint8_ts, uint8_t *string);
void format_crc(unsigned num_uint8_ts, uint8_t *string, uint8_t *chksum);
void send_set_motor(int direction, uint8_t speed, int fd);
void send_set_servo(uint8_t servo_num, uint8_t position, int fd);
void send_set_lcd(uint8_t col, uint8_t row, int fd);
void send_print_string(uint8_t *s, int fd);

int main(int argc, char *argv[]) {
    send_print_string((uint8_t *)"Hello, World!", STDOUT_FILENO);
    return 0;
}

void send_set_motor(int direction, uint8_t speed, int fd) {
    uint8_t *packet = set_motor(direction, speed);
    write(fd, packet, MIN_PACKET_SIZE + MOTOR_CONT_PAYLOAD);
    free(packet);
}

void send_set_servo(uint8_t servo_num, uint8_t position, int fd) {
    uint8_t *packet = set_servo(servo_num, position);
    write(fd, packet, MIN_PACKET_SIZE + SERVO_CONT_PAYLOAD);
    free(packet);
}

void send_set_lcd(uint8_t col, uint8_t row, int fd) {
    uint8_t *packet = set_LCD(col, row);
    write(fd, packet, MIN_PACKET_SIZE + SET_LCD_PAYLOAD);
    free(packet);
}

void send_print_string(uint8_t *s, int fd) {
    unsigned len = strlen((char *)s);
    uint8_t *packet = print_string(len, s);
    /* 2: for null byte and numuint8_ts */
    write(fd, packet, MIN_PACKET_SIZE + len + 2);
    free(packet);
}

/* */
uint8_t *set_motor(int direction, uint8_t speed) {
    uint8_t *chksum, *packet = malloc(MIN_PACKET_SIZE + MOTOR_CONT_PAYLOAD);
    if (!packet) {
        perror("Set Motot packet malloc");
        exit(EXIT_FAILURE);
    }
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(MOTOR_CONT_PAYLOAD);
    if (direction == MOTOR_COMMAND_LEFT) {
        packet[2] = MOTOR_COMMAND_LEFT;
    } else {
        packet[2] = MOTOR_COMMAND_RIGHT;
    }
    packet[3] = speed;
    packet[4] = (uint8_t)0x00; /*to be changed */
    packet[5] = (uint8_t)0x00;
    chksum = packet + 4;
    format_crc(MIN_PACKET_SIZE + MOTOR_CONT_PAYLOAD, packet, chksum);

    return packet;
}

uint8_t *set_servo(uint8_t servo_num, uint8_t position) {
    uint8_t *chksum, *packet = malloc(MIN_PACKET_SIZE + SERVO_CONT_PAYLOAD);

    if (!packet) {
        perror("Set Motot packet malloc");
        exit(EXIT_FAILURE);
    }
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SERVO_CONT_PAYLOAD);
    packet[2] = SERVO_CMD;
    packet[3] = servo_num;
    packet[4] = position;
    packet[5] = (uint8_t)0x00; /* to be changed */
    packet[6] = (uint8_t)0x00;
    chksum = packet + 5;
    format_crc(MIN_PACKET_SIZE + SERVO_CONT_PAYLOAD, packet, chksum);

    return packet;
}

uint8_t *set_LCD(uint8_t col, uint8_t row) {
    uint8_t *chksum, *packet = malloc(MIN_PACKET_SIZE + SET_LCD_PAYLOAD);
    if (!packet) {
        perror("Set Motot packet malloc");
        exit(EXIT_FAILURE);
    }
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SET_LCD_PAYLOAD);
    packet[2] = SET_CURSOR_CMD;
    /* 2 LSB for col and 6 MSB are for row*/
    packet[3] = 0x00;
    packet[3] = packet[3] | row;
    packet[3] = packet[3] | (col << 2);
    packet[4] = (uint8_t)0x00;
    packet[5] = (uint8_t)0x00;
    chksum = packet + 4;
    format_crc(MIN_PACKET_SIZE + SET_LCD_PAYLOAD, packet, chksum);

    return packet;
}


uint8_t *print_string(unsigned num_uint8_ts, uint8_t *string) {
    uint8_t *chksum, *packet = malloc(MIN_PACKET_SIZE + num_uint8_ts + 2);

    if (!packet) {
        perror("Set Motor packet malloc");
        exit(EXIT_FAILURE);
    }

    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(num_uint8_ts + 2);
    packet[2] = PRINT_STR_CMD;
    packet[3] = num_uint8_ts + 2;
    sprintf((char *)(packet + 4), "%s", string);
    chksum = packet + 5 + num_uint8_ts;
    packet[5 + num_uint8_ts] = 0x00;
    packet[6 + num_uint8_ts] = 0x00;
    format_crc(MIN_PACKET_SIZE + num_uint8_ts + 2, packet, chksum);
    /*  packet[4 + num_uint8_ts] = chksum[0];
        packet[5 + num_uint8_ts] = (uint8_t)0x00; */

    return packet;
}

void format_crc(unsigned num_uint8_ts, uint8_t *string, uint8_t *chksum) {
    /* calculate crc-16 of packet, and format it into len 2 array*/
    PacketCRC crc = crc_init(0x9B, num_uint8_ts);
    fprintf(stderr, "%s\n", string);
    fprintf(stderr, "%d\n", num_uint8_ts);
    uint8_t calcsum = calculate(crc, string, num_uint8_ts);
    fprintf(stderr,"%x\n\n", calcsum);
    memcpy(chksum, &calcsum, 1);
    freeTable(crc);
}

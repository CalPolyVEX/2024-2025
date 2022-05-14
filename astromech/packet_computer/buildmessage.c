#include "buildmessage.h"
/*
make a global array for everything
make command 5 combining 3 and 4: done, computer side only, not tested
checksum check the github command: done, works
make a packet, run it here and compare with arduino: works
try /dev/.cu in linux: works
crc without the marker: done
no need for numchars: https://github.com/jsseng/ue4/tree/master/herbie_nn/herbie2_pcb/firmware/firmware_redboard_v2
https://github.com/jsseng/ue4/blob/master/herbie_nn/ros/motor_control/src/pub_odometry.cpp

don't include 0xFF in checksum.

We can only print one line of the lcd at a time: how to deal with clearing??
*/
#define MAX_PACKET_SIZE 26

#define MOTOR_CTRL_PAYLOAD 1
#define SERVO_CTRL_PAYLOAD 2
#define SET_LCD_PAYLOAD 1

#define MIN_PACKET_SIZE 5

#define MOTOR_COMMAND_LEFT 0
#define MOTOR_COMMAND_RIGHT 1
#define SERVO_CMD 2
#define SET_CURSOR_CMD 3
#define PRINT_STR_CMD 4
#define LCD_PRINT_STR 5
#define LCD_CLR_CMD 6

#define DEV_F_NAME "/dev/cu.usbmodem142301"

uint8_t packet[MAX_PACKET_SIZE];
/*
packet[0] -> start marker (0xFF)
packet[1] -> payload size
packet[2] -> command
packet[3] -> 1st payload byte
packet[4] -> 2nd payload byte (if applicable)
packet[5] -> checksum
*/
// all servos are off at 131

int main(int argc, char *argv[]) {
    int fd;
    char buff[1000];

    // fd = open(DEV_F_NAME, O_WRONLY);
    fd = STDOUT_FILENO;
    // int fd_read = open("command", O_RDONLY);

    if (fd < 0) {
        perror(DEV_F_NAME);
        exit(EXIT_FAILURE);
    }
    // int bytes_read = read(fd_read, buff, 1000);
    // printf("bytes read: %d\n", bytes_read);

    // write(fd, buff, bytes_read);
    //
    // send_set_lcd(2, 3, fd);
    // write(fd, buff, 8);
    for (int i = 0; i < 100; i++) {
        sprintf(buff, "%d", i);
        send_print_string_at(0, 0, (uint8_t *)buff, fd);
    }
    // send_print_string_at(0, 0, (uint8_t *)"1", fd);
    // send_set_motor(MOTOR_COMMAND_LEFT, 127, fd);
    // send_set_motor(MOTOR_COMMAND_RIGHT, 253, fd);
    // send_set_servo(0, 131, fd);
    // send_set_servo(1, 131, fd);
    // send_set_servo(2, 131, fd);
    // send_set_servo(3, 131, fd);
    // send_print_string_at(2, 3, (uint8_t *)"wooooo", fd);

    close(fd);

    return 0;
}

void send_set_motor(int direction, uint8_t speed, int fd) {
    uint8_t *packet = set_motor(direction, speed);
    write(fd, packet, MIN_PACKET_SIZE + MOTOR_CTRL_PAYLOAD);
}

void send_set_servo(uint8_t servo_num, uint8_t position, int fd) {
    uint8_t *packet = set_servo(servo_num, position);
    write(fd, packet, MIN_PACKET_SIZE + SERVO_CTRL_PAYLOAD);
}

void send_set_lcd(uint8_t col, uint8_t row, int fd) {
    uint8_t *packet = set_lcd(col, row);
    write(fd, packet, MIN_PACKET_SIZE + SET_LCD_PAYLOAD);
}

void send_print_string(uint8_t *s, int fd) {
    unsigned len = strlen((char *)s);
    uint8_t *packet = print_string(len, s);
    /* 2: for null byte and numuint8_ts */
    write(fd, packet, MIN_PACKET_SIZE + len);
}

void send_print_string_at(uint8_t col, uint8_t row, uint8_t *s, int fd) {
    unsigned len = strlen((char *)s);
    uint8_t *packet = print_string_at(col, row, len, s);
    /* 2: for null byte and numuint8_ts */
    write(fd, packet, MIN_PACKET_SIZE + len + SET_LCD_PAYLOAD);
}

void send_clear_lcd(int fd) {
    uint8_t *packet = clear_lcd();
    write(fd, packet, MIN_PACKET_SIZE);
}

/**
 * @brief Creates the set motor packet
 * 0: 0xFF, 1: packet_size, 2: command, 3: payload
 *
 * @param direction
 * @param speed
 * @return uint8_t*
 */
uint8_t *set_motor(int direction, uint8_t speed) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(MOTOR_CTRL_PAYLOAD);
    if (direction == MOTOR_COMMAND_LEFT) {
        packet[2] = MOTOR_COMMAND_LEFT;
    } else {
        packet[2] = MOTOR_COMMAND_RIGHT;
    }
    packet[3] = speed;
    calc_crc(3 + MOTOR_CTRL_PAYLOAD);

    return packet;
}

uint8_t *set_servo(uint8_t servo_num, uint8_t position) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SERVO_CTRL_PAYLOAD);
    packet[2] = SERVO_CMD;
    packet[3] = servo_num;
    packet[4] = position;
    calc_crc(3 + SERVO_CTRL_PAYLOAD);

    return packet;
}

uint8_t *set_lcd(uint8_t col, uint8_t row) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SET_LCD_PAYLOAD);
    packet[2] = SET_CURSOR_CMD;
    /* 2 LSB for row and 6 MSB are for col */
    packet[3] = 0x00;
    packet[3] = packet[3] | row;
    packet[3] = packet[3] | (col << 2);
    calc_crc(3 + SET_LCD_PAYLOAD);

    return packet;
}

/**
 * @brief Prints a string to the LCD display.
 * We can only print up to 20 chars at a time.
 * the string, doesn;t need to be null terminated
 *
/ * @param num_chars
 * @param string
 * @return uint8_t*
 */
uint8_t *print_string(unsigned num_chars, uint8_t *string) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(num_chars);
    packet[2] = PRINT_STR_CMD;
    memcpy(packet + 3, string, num_chars);
    calc_crc(3 + num_chars);

    return packet;
}

uint8_t *print_string_at(uint8_t col, uint8_t row, unsigned num_chars,
                         uint8_t *string) {

    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(num_chars + SET_LCD_PAYLOAD);
    packet[2] = LCD_PRINT_STR;
    packet[3] = 0x00;
    packet[3] = packet[3] | row;
    packet[3] = packet[3] | (col << 2);
    memcpy(packet + 4, string, num_chars);
    calc_crc(3 + SET_LCD_PAYLOAD + num_chars);

    return packet;
}

uint8_t *clear_lcd() {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)0;
    packet[2] = (uint8_t)LCD_CLR_CMD;
    calc_crc(3);
    return packet;
}

uint8_t *calc_crc(int max_ind) {
    /* calculate crc16 (source: ue4/herbie_nn/ros/motor_control/src/pub_odometry.cpp)*/
    unsigned short crc = 0;
    int byte;
    const unsigned short crctable[256] =
        {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
            0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
            0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
            0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
            0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
            0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
            0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
            0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
            0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
            0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
            0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
            0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
            0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
            0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
            0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
            0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
            0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
            0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
            0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
            0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
            0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
            0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
            0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
            0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
            0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
            0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
            0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
            0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
            0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
            0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
            0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
            0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

    // Calculates CRC16 of nBytes of data in byte array message
    for (byte = 1; byte < max_ind; byte++) {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ packet[byte])];
    }

    packet[max_ind] = (crc >> 8) & 0xFF; // send the high byte of the crc
    packet[max_ind + 1] = crc & 0xFF;    // send the low byte of the crc
    return packet;
}

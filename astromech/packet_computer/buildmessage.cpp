#include "buildmessage.h"
#include "../packet_arduino/pc_decoder_info.h"
#include <libserial/SerialPort.h>
#include <cstdlib>
#include <fstream>

/*
make a global array for everything
make command 5 combining 3 and 4: done, computer side only, not tested
checksum check the github command: done, works
make a packet, run it here and compare with arduino: works
try /dev/.cu in linux: works
crc without the marker: done
no need for numchars:
https://github.com/jsseng/ue4/tree/master/herbie_nn/herbie2_pcb/firmware/firmware_redboard_v2
https://github.com/jsseng/ue4/blob/master/herbie_nn/ros/motor_control/src/pub_odometry.cpp

don't include 0xFF in checksum.

We can only print one line of the lcd at a time: how to deal with clearing??
*/

// Output Buffer
uint8_t packet[MAX_PACKET_SIZE];

// Input Buffer
uint8_t data[256];

// The Last Input Buffer
uint8_t last_read_input[256];

// The Index Into the Input Buffer
int data_index = 0;

// The Size of the Last Input
int last_input_size = 0;

// Specify the Consant USB Port
constexpr const char* const SERIAL_PORT = "/dev/ttyUSB-Redboard-Turbo";
//constexpr const char* const SERIAL_PORT = "/dev/ttyACM0";

// The Serial Port Object
LibSerial::SerialPort serial_port;

/*
packet[0] -> start marker (0xFF)
packet[1] -> payload size
packet[2] -> command
packet[3] -> 1st payload byte
packet[4] -> 2nd payload byte (if applicable)
packet[5] -> checksum
*/
// all servos are off at 131

/* Commenting Out Since Main Should Only Be Used for Manual Testing
int main(int argc, char *argv[]) {
    int fd = STDOUT_FILENO;

    // int bytes_read = read(fd_read, buff, 1000);
    // printf("bytes read: %d\n", bytes_read);

    // write(fd, buff, bytes_read);
    //
    // send_set_lcd(2, 3, fd);
    // write(fd, buff, 8);
    // for (int i = 0; i < 90000; i++) {
    //     // sprintf(buff, "%d", i);
    //     // send_print_string_at(0, 0, (uint8_t *)buff, fd);
    //     send_set_motor(MOTOR_COMMAND_RIGHT, 255, fd);
    // }
    // send_print_string_at(0, 0, (uint8_t *)"1", fd);
    for (int i = 0; i < 1000; i++) {
        send_set_motor(LEFT_MOTOR_CMD, 100, fd);
        send_set_motor(RIGHT_MOTOR_CMD, 100, fd);
    }

    // ############## servo test #################//
    // for (int i = 0; i < 1000; i++) {
    //     if(i%100)
    //         send_set_servo(0, 250, fd);
    //     else
    //         send_set_servo(0, 131, fd);
    // }
    // ############# end servo test ##############//

    // ############## reon test #################//
    // for (int i = 0; i < 1000; i++) {
    //     if (i % 100)
    //         send_set_reon(27, 2, fd); // reon rear off
    //     else
    //         send_set_reon(27, 2, fd); // reon rear white
    // }
    // send_set_reon(27, 3, fd); // reon rear white
    // ############# end reon test ##############//

    // send_set_servo(1, 131, fd);
    // send_set_servo(2, 131, fd);
    // send_set_servo(3, 131, fd);
    // send_print_string_at(2, 3, (uint8_t *)"wooooo", fd);
    // send_set_lcd(12, 2, fd);
    // send_led_preset_cmd(4, fd);

    close(fd);

    return 0;
}
*/


// This Function is Used to Initialize the File Descriptor on the ROS2 End
int init_serial() {

    // Open the Serial Port
    serial_port.Open(SERIAL_PORT);
    
    // Set Baud Rate of 115200
    serial_port.SetBaudRate(LibSerial::BaudRate::BAUD_115200);

    // Set Number of Bits to Max Packet Size
    serial_port.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);

    // Turn Off Flow Control
    serial_port.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);

    // Disable Parity
    serial_port.SetParity(LibSerial::Parity::PARITY_NONE);

    // Dissable All Stop Bits
    serial_port.SetStopBits(LibSerial::StopBits::STOP_BITS_1);

    return 0;
}

void read_arduino_data() {

    // Read Data Into Buffer
    while (serial_port.GetNumberOfBytesAvailable()) {
        serial_port.ReadByte(*(data + data_index), 0);
        if (data[data_index] == '\n' || data_index == 255) {

            // If Packet Size is Not the Specified Value, Packet is Invalid
            // Valid Packets are Copied Into Input Buffer
            if (data_index <= ARDUINO_PACKET_SIZE) {

                // Copy Last Read Input
                for (int i = 0; i < data_index; i++)
                    last_read_input[i] = data[i];

                // Store Size of Last Read Input
                last_input_size = data_index;
            }

            // Reset Buffer Index
            data_index = 0;
        }

        else
            data_index++;
    }
}

void get_input_buffer(uint8_t** buffer, int** size) {
    *buffer = last_read_input;
    *size = &last_input_size;
}

void send_set_motor(int direction, int8_t speed) {
    uint8_t packet_len = set_motor(direction, speed);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_set_servo(uint8_t servo_num, uint8_t position) {
    uint8_t packet_len = set_servo(servo_num, position);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_set_lcd(uint8_t col, uint8_t row) {
    uint8_t packet_len = set_cursor_lcd(col, row);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_print_string(uint8_t *s) {
    unsigned len = strlen((char *)s);
    uint8_t packet_len = print_string(len, s);
    /* 2: for null byte and numuint8_ts */
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_print_string_at(uint8_t col, uint8_t row, uint8_t *s) {
    unsigned len = strlen((char *)s);
    uint8_t packet_len = print_string_at(col, row, len, s);
    /* 2: for null byte and numuint8_ts */
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_clear_lcd() {
    uint8_t packet_len = clear_lcd();
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_led_preset_cmd(uint8_t preset_idx) {
    uint8_t packet_len = logic_preset_cmd(preset_idx);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_led_raw_cmd(uint8_t command_major, uint8_t command_minor, uint8_t color, uint8_t speed) {
    uint8_t packet_len = logic_raw_cmd(command_major, command_minor, color, speed);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_tsun_sound_cmd(uint8_t preset_idx, uint8_t volume) {
    uint8_t packet_len = tsunami_sound_cmd(preset_idx, volume);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_tsun_amp_cmd(uint8_t gain) {
    uint8_t packet_len = tsunami_amp_cmd(gain);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

void send_set_reon(uint8_t reon_addr, uint8_t reon_state) {
    uint8_t packet_len = set_reon(reon_addr, reon_state);
    std::vector<uint8_t> packet_vector(packet, packet + packet_len);
    serial_port.Write(packet_vector);
}

/**
 * @brief Creates the set motor packet
 * 0: 0xFF, 1: packet_size, 2: command, 3: payload
 *
 * @param direction
 * @param speed
 * @return uint8_t*
 */
uint8_t set_motor(int direction, uint8_t speed) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(MOTOR_CTRL_PAYLOAD);
    if (direction == LEFT_MOTOR_CMD) {
        packet[2] = LEFT_MOTOR_CMD;
    } else {
        packet[2] = RIGHT_MOTOR_CMD;
    }
    packet[3] = speed;
    calc_crc(3 + MOTOR_CTRL_PAYLOAD);

    return MIN_PACKET_SIZE + MOTOR_CTRL_PAYLOAD;
}

uint8_t set_servo(uint8_t servo_num, uint8_t position) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SERVO_CTRL_PAYLOAD);
    packet[2] = SERVO_CMD;
    packet[3] = servo_num;
    packet[4] = position;
    calc_crc(3 + SERVO_CTRL_PAYLOAD);

    return MIN_PACKET_SIZE + SERVO_CTRL_PAYLOAD;
}

uint8_t set_cursor_lcd(uint8_t col, uint8_t row) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(SET_LCD_PAYLOAD);
    packet[2] = LCD_CURSOR_CMD;
    /* 2 LSB for row and 6 MSB are for col */
    packet[3] = 0x00;
    packet[3] = packet[3] | row;
    packet[3] = packet[3] | (col << 2);
    calc_crc(3 + SET_LCD_PAYLOAD);

    return MIN_PACKET_SIZE + SET_LCD_PAYLOAD;
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
uint8_t print_string(unsigned num_chars, uint8_t *string) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(num_chars);
    packet[2] = LCD_PRINT_STR_CMD;
    memcpy(packet + 3, string, num_chars);
    calc_crc(3 + num_chars);

    return MIN_PACKET_SIZE + num_chars;
}

uint8_t print_string_at(uint8_t col, uint8_t row, unsigned num_chars, uint8_t *string) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)(num_chars + SET_LCD_PAYLOAD);
    packet[2] = LCD_CURSOR_PRINT_CMD;
    packet[3] = 0x00;
    packet[3] = packet[3] | row;
    packet[3] = packet[3] | (col << 2);
    memcpy(packet + 4, string, num_chars);
    calc_crc(3 + SET_LCD_PAYLOAD + num_chars);

    return MIN_PACKET_SIZE + SET_LCD_PAYLOAD + num_chars;
}

uint8_t clear_lcd() {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)0;
    packet[2] = (uint8_t)LCD_CLEAR_CMD;
    calc_crc(3);
    return MIN_PACKET_SIZE;
}

uint8_t logic_preset_cmd(uint8_t preset_index) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)LOGIC_PRESET_PAYLOAD;
    packet[2] = (uint8_t)LOGIC_PRESET_CMD;
    packet[3] = preset_index;
    calc_crc(3 + LOGIC_PRESET_PAYLOAD);
    return MIN_PACKET_SIZE + LOGIC_PRESET_PAYLOAD;
}

uint8_t logic_raw_cmd(uint8_t command_major, uint8_t command_minor, uint8_t color, uint8_t speed) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)LOGIC_RAW_PAYLOAD;
    packet[2] = (uint8_t)LOGIC_RAW_CMD;
    packet[3] = command_major;
    packet[4] = command_minor;
    packet[5] = color;
    packet[6] = speed;
    calc_crc(3 + LOGIC_RAW_PAYLOAD);
    return MIN_PACKET_SIZE + LOGIC_PRESET_PAYLOAD;
}

uint8_t tsunami_sound_cmd(uint8_t preset_index, int8_t volume) {
    insert_header(TSUN_SOUND_PAYLOAD, TSUN_SOUND_CMD);
    packet[3] = preset_index;
    packet[4] = volume;
    calc_crc(3 + TSUN_SOUND_PAYLOAD);
    return MIN_PACKET_SIZE + TSUN_SOUND_PAYLOAD;
}

uint8_t tsunami_amp_cmd(uint8_t gain) {
    insert_header(TSUN_AMP_PAYLOAD, TSUN_AMP_CMD);
    packet[3] = gain;
    calc_crc(3 + TSUN_AMP_PAYLOAD);
    return MIN_PACKET_SIZE + TSUN_AMP_PAYLOAD;
}

uint8_t set_reon(uint8_t reon_addr, uint8_t reon_state) {
    packet[0] = (uint8_t)0xFF;
    packet[1] = (uint8_t)REON_PAYLOAD;
    packet[2] = (uint8_t)REON_CMD;
    packet[3] = reon_addr;
    packet[4] = reon_state;
    calc_crc(3 + REON_PAYLOAD);
    return MIN_PACKET_SIZE + REON_PAYLOAD;
}

void insert_header(uint8_t payload_len, uint8_t cmd) {
    packet[0] = 0xFF;
    packet[1] = payload_len;
    packet[2] = cmd;
}

uint8_t *calc_crc(int max_ind) {
    /* calculate crc16 (source:
     * ue4/herbie_nn/ros/motor_control/src/pub_odometry.cpp)*/
    unsigned short crc = 0;
    int byte;
    const unsigned short crctable[256] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a,
        0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294,
        0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462,
        0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509,
        0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695,
        0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
        0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948,
        0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4,
        0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b,
        0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f,
        0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
        0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046,
        0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
        0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e,
        0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691,
        0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
        0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d,
        0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16,
        0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8,
        0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1, 0xef1f, 0xff3e,
        0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93,
        0x3eb2, 0x0ed1, 0x1ef0};

    // Calculates CRC16 of nBytes of data in byte array message
    for (byte = 1; byte < max_ind; byte++) {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ packet[byte])];
    }

    packet[max_ind] = (crc >> 8) & 0xFF; // send the high byte of the crc
    packet[max_ind + 1] = crc & 0xFF;    // send the low byte of the crc
    return packet;
}
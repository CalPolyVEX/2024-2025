#include "pc_decoder.h"
#include "pc_decoder_info.h"

// #define DEBUG

#ifdef DEBUG
uint16_t rx_crc;
uint16_t comp_crc;
#endif

// stores correct values used for checking valid packets
const uint16_t crctable[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
    0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
    0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
    0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
    0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
    0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
    0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
    0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
    0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
    0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

// The array of bytes from USB
uint8_t bytes[300];

//////////////////////////////////////////////////
// check_crc - check the crc of the incoming packet
bool check_crc(uint8_t *data, int len) // len is the length including the CRC
{
    uint16_t crc = 0;
    uint16_t received_crc;

    received_crc = (data[len - 2] << 8) & 0xFF00; // the crc is the last 2 bytes of the packet
    received_crc |= data[len - 1];

    // Calculates CRC16 of the (n-2) bytes of data in the packet
    for (int byte = 1; byte < (len - 2); byte++) {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
    }

#ifdef DEBUG
    rx_crc = received_crc;
    comp_crc = crc;
#endif

    return crc == received_crc;
}

// Reads the command and returns the value
unsigned short commandChecker(char *data) {
    char command = data[2];
    return short(command);
}

// Evaluate commands
void eval_input(uint8_t *data, int size, bool pc_mode) {
    int end_message = size - 2;
    // Check packet for errors
    if (!check_crc(data, size)) {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("CRCF");
#ifdef DEBUG
        lcd.setCursor(0, 1);
        lcd.print(rx_crc, 16);
        lcd.print("comp");
        lcd.print(comp_crc, 16);
#endif
        return;
    }
#ifdef DEBUG
    lcd.setCursor(0, 1);
    lcd.print("CRCT");
    lcd.print(rx_crc, 16);
    lcd.print("comp");
    lcd.print(comp_crc, 16);
#endif

    // Check command and send off payload to correct function
    switch (data[2]) {
    case LEFT_MOTOR_CMD: // Set left motor
        if (pc_mode) {
            int16_t new_motor_speed = *(int8_t)(data + 3);
            change_motor_speed(0, new_motor_speed);
            //change_motor_speed(0, int(data[3]));
        }
        break;

    case RIGHT_MOTOR_CMD: // Set right motor
        if (pc_mode) {
            int16_t new_motor_speed = *(int8_t)(data + 3);
            change_motor_speed(1, new_motor_speed);
            //change_motor_speed(1, int(data[3]));
        }
        break;

    case SERVO_CMD: // Set servo
        if (pc_mode) {
            set_servo_angle((unsigned short)(data[3]), (unsigned int)(data[4]));
        }
        break;

    case LCD_CURSOR_CMD: // Set LCD cursor
        lcd.setCursor(((uint8_t)(data[3] & 0xFC)) >> 2, data[3] & 0x03);
        break;

    case LCD_PRINT_STR_CMD: // Print string
        for (int i = 3; i < end_message; i++)
            lcd.write(data[i]);
        break;

    case LCD_CURSOR_PRINT_CMD: // set cursor then Print string
        lcd.setCursor((data[3] & 0xFC) >> 2, data[3] & 0x03);
        for (int i = 4; i < end_message; i++)
            lcd.write(data[i]);
        break;

    case LCD_CLEAR_CMD: // clear screen
        lcd.clear();
        break;

    case LOGIC_PRESET_CMD: // Send Command to Logic Engine Using Presets
        sendLogicEngineCommand(data[3]);
        break;

    case LOGIC_RAW_CMD: // Send Command to Logic Engine Controller Using Raw Commands
        sendLogicEngineCommand(data[3], data[4], data[5], data[6]);
        break;

    // Send Command to Play a Sound Through the Tsunami (Value 1 = Sound Index, Value 2 = Volume
    // (Signed))
    case TSUN_SOUND_CMD:
        playTsunamiSound(data[3], data[4]);
        break;

    case TSUN_AMP_CMD: // Set the Gain of the Amplifier for Sound
        setAmplifierGain(data[3]);
        break;

    case REON_CMD: // set REON HP sequence for all three HPs
        send_reon_command(int(data[4]), int(data[3]));
        break;

    case PSI_PRO_CMD: // Set Command to set Light Sequence Both PSIs (Look to PSI Github for Valid
                      // Command Numbers)
        sendPSICommand(data[3]);
        break;
    };
}

void pc_get_input(bool pc_mode) {
    // void get_input() {
    static uint16_t byte_count = 0;
    static uint32_t last_packet_time = millis();
    uint16_t test_val;
    // static uint8_t payload;
    // static char cur_byte;
    // if no command decoded in last 1 second

    /* testing buffer rerouting*/

    /* end testing buffer rerouting*/

    if (pc_mode && (millis() - last_packet_time > MOTOR_TIMEOUT)) {
        // CHECK IF 0 is the actual minimum speed.
        change_motor_speed(0, 0);
        change_motor_speed(1, 0);
        set_servo_angle(0, 0);
        set_servo_angle(1, 0);
        set_servo_angle(2, 0);
        set_servo_angle(3, 0);
    }
    if (SerialUSB.available()) {
        if (byte_count == 0) { // confirming start of packet
            if ((test_val = SerialUSB.read()) == 0xFF) {
                bytes[byte_count++] = 0xFF;
            } else {
                lcd.setCursor(0, 3);
                lcd.print("INVALSTRT");
                lcd.print(test_val);
            }
        } else if (byte_count == 1) {
            // we are already reading a packet
            // read the payload size
            bytes[byte_count++] = SerialUSB.read();
        } else if (byte_count < bytes[1] + 5) {
            // read until we get everything else
            bytes[byte_count++] = SerialUSB.read();
        }
        if (byte_count >= 2 && byte_count == bytes[1] + 5) {
            eval_input(bytes, byte_count, pc_mode);
            last_packet_time = millis();
            byte_count = 0;
        }
    }

    // alternative option, may be revisited

    //     if (SerialUSB.available()) {
    //         cur_byte = SerialUSB.read();
    //         if (cur_byte == 0xFF) {
    //             byte_count++;
    //             bytes[0] = 0xFF;
    //             payload = SerialUSB.read();
    //             bytes[1] = payload;
    //             // This doesn't block because timeout is set to 0
    //             // packet size is only the size of the payload
    //             while (byte_count < payload + 5) {
    //                 bytes[byte_count++] = SerialUSB.read();
    //             }
    //             if ((byte_count +=
    //                  SerialUSB.readBytes(bytes + 2, payload + 3)) ==
    //                 payload + 5) {
    //                 eval_input(bytes, byte_count);
    //             }
    // #ifdef DEBUG
    //             lcd.print("E");
    // #endif
    //         }
    // }
}

#include "RedBoardTurboWorkingServoFile.h"
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip

// #define DEBUG

// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

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

// The array of bytes from USB
char bytes[256];

//////////////////////////////////////////////////
// check_crc - check the crc of the incoming packet
bool check_crc(char *data, int len) // len is the length including the CRC
{
    unsigned short crc = 0;
    unsigned short received_crc;

    received_crc = (data[len - 2] << 8) & 0xFF00; // the crc is the last 2 bytes of the packet
    received_crc |= data[len - 1];

    // Calculates CRC16 of the (n-2) bytes of data in the packet
    for (int byte = 1; byte < (len - 2); byte++) {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
    }

    return crc == received_crc;
}

// Reads the command and returns the value
unsigned short commandChecker(char *data) {
    char command = data[2];
    return short(command);
}

// Evaluate commands
void eval_input(char *data, int size) {
#ifdef DEBUG
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Working?");
    lcd.setCursor(0, 1);
    lcd.print(int(data[1]));
    delay(3000);
#endif
    // Check packet for errors
    if (!check_crc(data, size)) {
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.clear();
        lcd.print("Chksum");
        lcd.setCursor(0, 1);
        lcd.print(data[size - 2], 16);
        lcd.print(data[size - 1], 16);
        delay(2000);
#endif
        // return;
    }

    // Get command
    // int command = commandChecker(data);

    // Check command and send off payload to correct function
    switch (data[2]) {
    // Set left motor
    case 0:
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.clear();
        lcd.print("left");
        delay(1000);
#endif
        change_motor_speed(0, int(data[3]));
        break;
    // Set right motor
    case 1:
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.clear();
        lcd.print("right");
        delay(1000);
#endif
        change_motor_speed(1, int(data[3]));
        break;
    // Set servo
    case 2:
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.clear();
        lcd.print("Servo");
        delay(1000);
#endif
        set_servo_angle(int(data[3]), int(data[4]));
        break;
    // Set LCD cursor
    case 3:
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.print("cursor");
        delay(1000);
#endif
        lcd.setCursor(data[3] && 0x03, (data[3] && 0xFC) >> 2);
        break;
    // Print string
    case 4:
#ifdef DEBUG
        lcd.setCursor(0, 0);
        lcd.print("print");
        delay(1000);
#endif
        int end_message = size - 2;
        lcd.clear();
        for (int i = 3; i < end_message; i++)
            lcd.write(data[i]);
        delay(2000);
        break;
    };
}

// Get input
// void get_input() {
//     static unsigned short int byte_count = 0;
//     static unsigned short int max_byte_count = 5;
//     // If USB is recieving data, collect data
//     if (SerialUSB.available()) {
//         char cur_byte;

//         // Get input from serialUSB
//         while (byte_count < max_byte_count) {
//             if (SerialUSB.available()) {
//                 cur_byte = SerialUSB.read();
//                 // If current byte is the start marker, start reading data
//                 if (!byte_count && cur_byte == 0xFF) {
//                     byte_count++;
//                     bytes[0] = 0xFF;
//                     max_byte_count = SerialUSB.peek();
// #ifdef DEBUG
//                     lcd.setCursor(0, 1);
//                     lcd.print(max_byte_count);
//                     delay(2000);
// #endif
//                 }

//                 // If reading, insert byte into bytes array
//                 else if (byte_count) {
//                     bytes[byte_count] = cur_byte;
//                     byte_count++;
//                 }
//             }
//         }

//         // If data was collected, process input
//         if (byte_count && byte_count == max_byte_count) {
//             eval_input(bytes, byte_count + 5);
//             max_byte_count = 5;
//             byte_count = 0;
//         }
//     }
// }

void get_input() {
    unsigned short byte_count = 0;
    unsigned short payload;
    char cur_byte;

    if (SerialUSB.available()) {
        cur_byte = SerialUSB.read();
        if (cur_byte == 0xFF) {
            byte_count++;
            bytes[0] = 0xFF;
            payload = SerialUSB.peek();
#ifdef DEBUG
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Payload");
            lcd.setCursor(0, 1);
            lcd.print(int(payload));
            delay(3000);
#endif
            bytes[1] = payload;
            // readBytes is ignoring the value peeked for some reason
            // This doesn't block because timeout is set to 0
            if ((byte_count =
                     SerialUSB.readBytes(bytes + 2, payload + 4) + 2) ==
                payload + 5) {
                eval_input(bytes, byte_count);
            }
#ifdef DEBUG2
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("payload 2");
            lcd.setCursor(0, 1);
            lcd.print((int)byte_count);
            lcd.setCursor(0, 2);
            lcd.print((int)bytes[1]);

            delay(3000);
#endif
        }
    }
}

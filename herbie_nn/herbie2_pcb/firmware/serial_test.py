#!/usr/bin/python3

import serial
from array import array
import binascii
from time import sleep
import time

crctable = array('H', [
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
   0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0])

def create_motor_packet(speed_left, speed_right):
    crc = 0
    packet = bytearray(8)

    packet[0] = 128 #roboclaw address
    packet[1] = 34  #set left and right motor command

    packet[2] = (speed_left >> 8) & 0xFF; #send the high byte of the duty cycle
    packet[3] = speed_left & 0xFF; #send the low byte of the duty cycle

    packet[4] = (speed_right >> 8) & 0xFF; #send the high byte of the duty cycle
    packet[5] = speed_right & 0xFF; #send the low byte of the duty cycle

    #Calculates CRC16 of nBytes of data in byte array message
    for byte in range(6):
        crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

    packet[6] = (crc >> 8) & 0xFF; #send the high byte of the crc
    packet[7] = crc & 0xFF; #send the low byte of the crc

    #hex_bytes = binascii.hexlify(packet)
    #print(hex_bytes) # b'0a160a04' which is twice as long as in_bytes

    return packet

def create_set_cursor_packet(col, row):
    crc = 0
    packet = bytearray(6)

    packet[0] = 128 #roboclaw address
    packet[1] = 1  #set cursor
    packet[2] = col  #column
    packet[3] = row  #row

    #Calculates CRC16 of nBytes of data in byte array message
    for byte in range(4):
        crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

    packet[4] = (crc >> 8) & 0xFF; #send the high byte of the crc
    packet[5] = crc & 0xFF; #send the low byte of the crc

    return packet

def create_clear_screen_packet():
    crc = 0
    packet = bytearray(4)

    packet[0] = 128 #roboclaw address
    packet[1] = 0  #clear screen

    #Calculates CRC16 of nBytes of data in byte array message
    for byte in range(2):
        crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

    packet[2] = (crc >> 8) & 0xFF; #send the high byte of the crc
    packet[3] = crc & 0xFF; #send the low byte of the crc

    return packet

def create_string_packet(str):
    crc = 0
    packet = bytearray(len(str) + 5)

    packet[0] = 128 #roboclaw address

    packet[1] = 2   #string command

    packet[2] = len(str) #set the length

    new_string_bytes = bytes(str,"ascii")
    for i in range(len(new_string_bytes)):
        packet[3+i] = new_string_bytes[i]

    #Calculates CRC16 of nBytes of data in byte array message
    for byte in range(len(str)+3):
        crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

    packet[-2] = (crc >> 8) & 0xFF; #send the high byte of the crc
    packet[-1] = crc & 0xFF; #send the low byte of the crc

    return packet

def create_led_packet(num, state):
    crc = 0
    packet = bytearray(5)

    packet[0] = 128 #roboclaw address

    if state == 1:
        packet[1] = 7   #LED on command
    else:
        packet[1] = 8   #LED off command

    packet[2] = num; #set the LED number

    #Calculates CRC16 of nBytes of data in byte array message
    for byte in range(3):
        crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

    packet[3] = (crc >> 8) & 0xFF; #send the high byte of the crc
    packet[4] = crc & 0xFF; #send the low byte of the crc

    return packet

def blink_led(ser):
    for i in range (30000):
        p = create_led_packet(1,1)
        ser.write(p)     # turn LED on
        sleep(.020)
        p = create_led_packet(1,0)
        ser.write(p)     # turn LED off
        sleep(.020)
        if i % 1000 == 0:
            print(i)

    ser.close()             # close port

def string_test(ser):
    p = create_clear_screen_packet()
    ser.write(p)     # clear screen

    for i in range (30000):
        # p = create_clear_screen_packet()
        # ser.write(p)     # clear screen
        # sleep(.000010)

        p = create_set_cursor_packet(3,0)
        ser.write(p)     # set cursor
        sleep(.000010)
        p = create_string_packet(str(float(i)*1.12345))
        ser.write(p)     # send string
        sleep(.000010)

        p = create_led_packet(2,1)
        ser.write(p)     # turn LED on
        sleep(.000010)

        p = create_set_cursor_packet(3,1)
        ser.write(p)     # set cursor
        sleep(.000010)
        p = create_string_packet(str(i*2))
        ser.write(p)     # send string
        sleep(.000010)

        p = create_led_packet(2,0)
        ser.write(p)     # turn LED off
        sleep(.0010)

        print(str(ser.read(11)))

        if i % 1000 == 0:
            print(i)

    ser.close()             # close port

def motor_test(ser):
    while(1):
        for j in range(50):
            for i in range(50):
                cur_time = int(round(time.time() * 1000))
                # p = create_motor_packet(j,i)
                p = create_motor_packet(240,i)
                ser.write(p)     # write a string
                packet = ser.read(12)
                last_time = cur_time
                count += 1
                # if (count % 30 == 0):
                #     print(cur_time - last_print_time)
                #     last_print_time = cur_time

                hex_bytes = binascii.hexlify(packet)
                print(hex_bytes) # b'0a160a04' which is twice as long as in_bytes
                # sleep(.01) #sleep 30ms

def main():
    ser = serial.Serial('/dev/ttyACM0', 921600, timeout=1)  # open serial port
    print(ser.name)         # check which port was really used
    count = 0
    last_print_time = 0

    #blink_led(ser)
    string_test(ser)
    exit()

    # print(ser.read(30))
    print (ser.baudrate)
    ser.close()             # close port

if __name__ == "__main__":
    # execute only if run as a script
    main()

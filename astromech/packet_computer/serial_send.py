#!/usr/bin/python3
"""
"""
import serial
import time
import sys

START_DELAY = 0  # no need to be changed

# should be 256 or 255 for redboard turbo
# not quite sure
# can't be higher
SERIAL_BUFF_SIZE = 32
# this is the delay between writes
# each write is buff size
DELAY_BETWEEN_WRITES = 0.01  # goal is to make this as low as possible

if (len(sys.argv) != 4 and len(sys.argv) != 3):
    print('usage: python3 serial_send.py dev_file baud_rate [input_file]')
    sys.exit(-1)

dev_file = sys.argv[1]
baud_rate = int(sys.argv[2])
if (len(sys.argv) == 4):
    input_file = sys.argv[3]
else:
    input_file = '/dev/stdin'

with serial.Serial(dev_file, baud_rate) as ser:
    time.sleep(START_DELAY)
    with open(input_file, 'rb') as in_f:
        file_data = in_f.read(SERIAL_BUFF_SIZE)
        while len(file_data) != 0:
            print(file_data.hex())
            print()
            ser.write(file_data)
            file_data = in_f.read(SERIAL_BUFF_SIZE)
            time.sleep(DELAY_BETWEEN_WRITES)

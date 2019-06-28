#!/usr/bin/env python
import serial
import time
import rospy
from std_msgs.msg import Empty

class Lcd:
    def init_serial_port(self):
        self.ser = serial.Serial('/dev/ttyS0', 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)
        rospy.init_node("lcd_node",log_level=rospy.DEBUG)

        self.loop_closure = rospy.Subscriber('/loop_closure', Empty, self.loop_closure_cb, queue_size=1)

        #change_baud()
        self.clear_screen()
        self.print_test()
        #backlight_on()
        #set_contrast(230)
        self.autoscroll_on()
        self.clear_screen()

    def loop_closure_cb(self, e):
        self.clear_screen()
        self.print_test()
        time.sleep(3)
        self.clear_screen()

    def autoscroll_on(self):
        values = bytearray([254, 0x51])
        self.ser.write(values)

    def set_contrast(c):
        values = bytearray([254, 0x50, c])
        self.ser.write(values)

    def backlight_off(self):
        values = bytearray([254, 0x46])
        self.ser.write(values)

    def backlight_on(self):
        values = bytearray([254, 0x42, 1])
        self.ser.write(values)

    def change_baud(self):
        values = bytearray([254, 57, 8])
        self.ser.write(values)

    def clear_screen(self):
        values = bytearray([254, 88])
        self.ser.write(values)

    def print_test(self):
        self.ser.write(b'Loop Closure')

if __name__ == "__main__":
    node = Lcd()
    node.init_serial_port()
    rospy.spin()


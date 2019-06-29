#!/usr/bin/env python
import serial
import time
import rospy
from std_msgs.msg import Empty

class Lcd:
    def init_serial_port(self):
        self.ser = serial.Serial('/dev/ttyS0', 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)

    def init_ros(self):
        rospy.init_node("lcd_node",log_level=rospy.DEBUG)
        #rospy.on_shutdown(self.shutdown)
        rospy.loginfo("Starting LCD node")

        self.loop_closure = rospy.Subscriber('/loop_closure', Empty, self.loop_closure_cb, queue_size=1)

    def init_lcd_node(self):
        #change_baud()
        time.sleep(2.5)
        self.clear_screen()
        #backlight_on()
        #set_contrast(230)
        self.autoscroll_on()
        self.clear_screen()
        self.set_cursor_position(1,1)
        self.print_string("Starting...")
        self.lc = 0

    def shutdown(self):
        self.clear_screen()
        self.set_cursor_position(1,1)
        self.print_string("Shutdown...")
    def close(self):
        self.ser.close()

    def loop_closure_cb(self, e):
        if self.lc == 0:
            self.clear_screen()
            self.set_cursor_position(1,2)
            self.print_string("Map started")

        self.set_cursor_position(1,1)
        self.print_string("LC: " + str(self.lc))
        time.sleep(2)
        self.set_cursor_position(1,1)
        self.print_string("       ")
        self.lc += 1

    def set_cursor_position(self, col, row):
        values = bytearray([254, 0x47, col, row])
        self.ser.write(values)

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

    def print_string(self, s):
        self.ser.write(str.encode(s))

if __name__ == "__main__":
    node = Lcd()
    node.init_serial_port()
    node.init_ros()
    node.init_lcd_node()
    rospy.spin()


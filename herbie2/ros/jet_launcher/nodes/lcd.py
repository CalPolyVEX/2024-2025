#!/usr/bin/env python
import serial
import time
import rospy
from std_msgs.msg import Empty
from std_msgs.msg import Int32MultiArray

class Lcd:
    def init_serial_port(self):
        try:
            self.ser = serial.Serial('/dev/lcd', 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE)
            return True
        except:
            return False

    def init_ros(self):
        rospy.init_node("lcd_node",log_level=rospy.DEBUG)
        #rospy.on_shutdown(self.shutdown)
        rospy.loginfo("Starting LCD node")

        self.loop_closure = rospy.Subscriber('/update_loop_closure_lcd', Int32MultiArray, self.loop_closure_cb, queue_size=1)

    def init_lcd_node(self):
        #change_baud()
        time.sleep(2.5)
        self.clear_screen()
        #self.backlight_on()
        self.set_contrast(240)
        self.backlight_off()
        self.autoscroll_on()
        self.clear_screen()
        self.set_cursor_position(1,1)
        self.print_string("Starting...")
        self.lc = 0
        self.pd = 0
        self.loop_closure_init = 0

    def shutdown(self):
        self.clear_screen()
        self.set_cursor_position(1,1)
        self.print_string("Shutdown...")

    def close(self):
        self.ser.close()

    def loop_closure_cb(self, a):
        if self.loop_closure_init == 0:
            self.clear_screen()
            self.set_cursor_position(1,2)
            self.print_string("Map started")
            self.loop_closure_init = 1
        else:
            if a.data[0] != 0:
                self.lc += 1
            if a.data[1] != 0:
                self.pd += 1

            self.set_cursor_position(1,1)
            self.print_string("LC: {0:3d} PD: {1:3d}".format(self.lc, self.pd))
            time.sleep(2)
            self.set_cursor_position(1,1)
            self.print_string("          ")

    def set_cursor_position(self, col, row):
        values = bytearray([254, 0x47, col, row])
        self.ser.write(values)

    def autoscroll_on(self):
        values = bytearray([254, 0x51])
        self.ser.write(values)

    def set_contrast(self,c):
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
    if node.init_serial_port() == True:
        node.init_ros()
        node.init_lcd_node()
        rospy.spin()

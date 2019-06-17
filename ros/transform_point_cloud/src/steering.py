#!/usr/bin/python

# adapted from https://github.com/recantha/EduKit3-RC-Keyboard/blob/master/rc_keyboard.py
import sys, termios, tty, os, time
import rospy
from geometry_msgs.msg import Twist
from std_msgs.msg import String
import threading

linear_velocity = 0
angular_velocity = 0
pub = 0
l = threading.Lock()
e = 0

def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)

    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def read_keys():
    global linear_velocity, angular_velocity, l, e
    t_l=0
    t_a=0

    while True:
        char = getch()

        if (char == "q"):
            e = 1
            exit(0)

        if (char == "s"):
            t_a += .02

        elif (char == "f"):
            t_a -= .02

        elif (char == "e"):
            t_l += .02

        elif (char == "d"):
            t_l -= .02

        elif (char == "a"):
            t_l = 0
            t_a = 0

        l.acquire()
        linear_velocity = t_l
        angular_velocity = t_a
        l.release()

def steering():
    global linear_velocity, angular_velocity, pub, l, e

    while not rospy.is_shutdown():
        vel_msg = Twist()
        vel_msg.linear.y = 0
        vel_msg.linear.z = 0
        vel_msg.angular.x = 0
        vel_msg.angular.y = 0

        l.acquire()
        vel_msg.linear.x = linear_velocity
        vel_msg.angular.z = angular_velocity
        l.release()

        pub.publish(vel_msg)
        rate.sleep()

        if e == 1:
            break

if __name__ == '__main__':
    try:
        print "Press 'q' to quit."
        print "Press 'a' to stop. (s,d,f,e) to control."
        pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)
        rospy.init_node('steering', anonymous=True)
        rate = rospy.Rate(10) # 10hz
        x = threading.Thread(target=steering)
        y = threading.Thread(target=read_keys)
        y.start()
        x.start()
    except rospy.ROSInterruptException:
        pass

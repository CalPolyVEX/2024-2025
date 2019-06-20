#!/usr/bin/python
import pygame
from time import sleep
import rospy, sys, os
from geometry_msgs.msg import Quaternion, Twist
import subprocess, threading

class JoystickNode:
   def __init__(self):
      os.environ["SDL_VIDEODRIVER"] = "dummy"
      pygame.joystick.init()

      #check the number of joysticks
      num_joystick = pygame.joystick.get_count()
      if num_joystick==0:
         print "No joysticks found."
         exit(0)

      rospy.init_node("joystick_node",log_level=rospy.DEBUG)
      rospy.on_shutdown(self.shutdown)
      rospy.loginfo("Connecting to joystick")

      self.motor_command_pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)

      rospy.sleep(1)

      pygame.display.init()
      pygame.joystick.Joystick(0).init()

   def record_bag(self):
      proc1 = subprocess.Popen('cd /mnt/temp;./ue4/ros/jet_launcher/launch/record_zed.sh', shell=True)

   def toggle_led(self):
      proc = subprocess.Popen('rosservice call /zed/zed_node/toggle_led', shell=True)

   def run(self):
      old_linear = 0
      old_angular = 0
      start = 0
      recording_start = 0
      recording_hold = 0

      # Prints the joystick's name
      JoyName = pygame.joystick.Joystick(0).get_name()
      rospy.logdebug("Name of the joystick: %s", JoyName)
      # Gets the number of axes
      JoyAx = pygame.joystick.Joystick(0).get_numaxes()
      rospy.logdebug("Number of axes: %d", JoyAx)

      # Gets the number of buttons
      JoyButtons = pygame.joystick.Joystick(0).get_numbuttons()
      rospy.logdebug("Number of buttons: %d", JoyButtons)

      r_time = rospy.Rate(10)
      vel_msg = Twist()

      #do not print SDL messages
      sys.stdout = os.devnull
      sys.stderr = os.devnull

      t = threading.Timer(61,self.toggle_led)
      t.cancel()

      while not rospy.is_shutdown():
         pygame.event.pump()

         #wait for a press on button 1 to begin reading the left analog stick
         if start == 0 and pygame.joystick.Joystick(0).get_button(0) == 1:
            start = 1

         if start == 1:
            # Prints the values for axis0
            axis0 = pygame.joystick.Joystick(0).get_axis(0)
            axis1 = pygame.joystick.Joystick(0).get_axis(1)
            axis2 = pygame.joystick.Joystick(0).get_axis(2)
            axis3 = pygame.joystick.Joystick(0).get_axis(3)
            # rospy.logdebug("axis 0: %f", axis0)
            # rospy.logdebug("axis 1: %f", axis1)
            # rospy.logdebug("axis 2: %f", axis2)
            # rospy.logdebug("axis 3: %f", axis3)
            # rospy.logdebug("button 0: %d", pygame.joystick.Joystick(0).get_button(0))

            vel_msg.linear.x = -.3 * axis1
            vel_msg.angular.z = -0.7 * axis0
            # vel_msg.linear.x = .5*old_linear + .5* -.3 * axis1
            # vel_msg.angular.z = .5*old_angular + .5* -0.7 * axis0
            old_linear = vel_msg.linear.x
            old_angular = vel_msg.angular.z

            self.motor_command_pub.publish(vel_msg)

         #wait for a press on button 2 to begin reading recording rosbag
         button2 = pygame.joystick.Joystick(0).get_button(1)
         if recording_start == 0 and button2 == 1 and t.finished:
            recording_start = 1
            rospy.loginfo('Starting recording')
            self.toggle_led()
            self.record_bag()
            t = threading.Timer(11,self.toggle_led)
            t.start()
         elif recording_start == 1 and button2 == 1:
            button2 = 1
         else:
            recording_start = 0

         r_time.sleep()

   def shutdown(self):
      rospy.loginfo("Shutting down")

if __name__ == "__main__":
    try:
        node = JoystickNode()
        node.run()
    except rospy.ROSInterruptException:
        pass
    rospy.loginfo("Exiting")

#!/usr/bin/python
import pygame
from time import sleep
import rospy, sys, os
from geometry_msgs.msg import Quaternion, Twist

class JoystickNode:
   def __init__(self):
      #check the number of joysticks
      os.environ["SDL_VIDEODRIVER"] = "dummy"
      pygame.joystick.init()
      num_joystick = pygame.joystick.get_count()
      if num_joystick==0:
         print "No joysticks found."
         exit(0)

      rospy.init_node("joystick_node",log_level=rospy.DEBUG)
      rospy.on_shutdown(self.shutdown)
      rospy.loginfo("Connecting to joystick")

      self.motor_command_pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)

      #rospy.Subscriber("cmd_vel", Twist, self.cmd_vel_callback, queue_size=1)

      rospy.sleep(1)

      pygame.display.init()
      pygame.joystick.Joystick(0).init()

   def run(self):
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

      while not rospy.is_shutdown():
         pygame.event.pump()
         # Prints the values for axis0
         axis0 = pygame.joystick.Joystick(0).get_axis(0)
         axis1 = pygame.joystick.Joystick(0).get_axis(1)
         axis2 = pygame.joystick.Joystick(0).get_axis(2)
         axis3 = pygame.joystick.Joystick(0).get_axis(3)
         rospy.logdebug("axis 0: %f", axis0)
         rospy.logdebug("axis 1: %f", axis1)
         rospy.logdebug("axis 2: %f", axis2)
         rospy.logdebug("axis 3: %f", axis3)
         rospy.logdebug("button 0: %d", pygame.joystick.Joystick(0).get_button(0))

         vel_msg.linear.x = -1.0 * axis1
         vel_msg.angular.z = -0.5 * axis0

         self.motor_command_pub.publish(vel_msg)

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

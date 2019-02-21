#!/usr/bin/python
import pygame
from time import sleep
import rospy
from geometry_msgs.msg import Quaternion, Twist

class JoystickNode:
   def __init__(self):
      #self.lock = threading.Lock()
      rospy.init_node("joystick_node",log_level=rospy.DEBUG)
      rospy.on_shutdown(self.shutdown)
      rospy.loginfo("Connecting to joystick")

      self.motor_command_pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)

      #rospy.Subscriber("cmd_vel", Twist, self.cmd_vel_callback, queue_size=1)

      rospy.sleep(1)

      pygame.display.init()
      pygame.joystick.init()
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

      while not rospy.is_shutdown():
         pygame.event.pump()
         # Prints the values for axis0
         axis0 = pygame.joystick.Joystick(0).get_axis(0)
         axis1 = pygame.joystick.Joystick(0).get_axis(1)
         rospy.logdebug("axis 0: %f", axis0)
         rospy.logdebug("axis 1: %f", axis1)
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

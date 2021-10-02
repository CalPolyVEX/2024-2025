#!/usr/bin/python
import pygame
from time import sleep
import rospy, sys, os
from geometry_msgs.msg import Quaternion, Twist
from std_msgs.msg import Empty
from std_msgs.msg import Int8
import subprocess, threading
from lcd import Lcd

class JoystickNode:
   def shutdown(self):
      pygame.joystick.quit()
      pygame.quit()
      rospy.loginfo("Shutting down")

   def __init__(self):
      os.environ["SDL_VIDEODRIVER"] = "dummy"
      pygame.display.init()
      pygame.joystick.init()
      pygame.joystick.quit()
      pygame.quit()
      pygame.display.init()
      pygame.joystick.init()
      pygame.event.clear()

      #check the number of joysticks
      num_joystick = pygame.joystick.get_count()
      if num_joystick==0:
         print "No joysticks found."
         exit(0)

      pygame.display.init()
      pygame.joystick.Joystick(0).init()

      rospy.init_node("joystick_node",log_level=rospy.DEBUG)
      rospy.on_shutdown(self.shutdown)
      rospy.loginfo("Connecting to joystick")

      self.motor_command_pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)
      self.robot_stop_pub = rospy.Publisher('/robot_stop', Empty, queue_size=1)
      self.autonomous_pub = rospy.Publisher('/autonomous', Int8, queue_size=1)
      self.autonomous_led_pub = rospy.Publisher('/read_encoder_cmd', Int8, queue_size=1)

      rospy.sleep(1)

      #workaround for joystick bug
      button1 = 0
      pygame.event.pump()
      while pygame.joystick.Joystick(0).get_button(0) == 1:
         pygame.event.pump()
         if button1 == 0:
            l = Lcd()
            l.init_serial_port()
            l.clear_screen()
            l.print_string('Press button 1.')
            l.close()
            button1 = 1

   def print_raffle(self):
      i = 0
      l = Lcd()
      l.init_serial_port()
      delay = .1
      count = 30

      for j in range(0,5):
         for i in range(0,count):
            l.clear_screen()
            l.print_string(str('Press ' + str(i)))
            sleep(delay)
         count = count / 2
         delay = delay * 2

   def run(self):
      self.print_raffle()
      old_linear = 0
      old_angular = 0
      start = 0
      send_cmd_vel = 0
      recording_start = 0
      recording_hold = 0
      robot_stop = 0
      autonomous = 0
      autonomous_led_state = 0
      button9 = 0
      button1_hold = 0
      button2_hold = 0
      button4_hold = 0

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
      # sys.stdout = os.devnull
      # sys.stderr = os.devnull

      rospy.signal_shutdown('Quit')

      #while not rospy.is_shutdown():
      while True:
         pygame.event.pump()

         #wait for a press on button 1 to begin reading the left analog stick
         button1 = pygame.joystick.Joystick(0).get_button(0)
         if start == 0 and button1 == 1:
            start = 1
            send_cmd_vel = send_cmd_vel ^ 1;
         elif start == 1 and button1 == 1:
            #hold down
            button1_hold += 1
            if button1_hold == 25: #button1 held for 3 seconds
               print "Button1 hold"
               l = Lcd()
               l.init_serial_port()
               l.clear_screen()
               l.print_string('Exiting...')
               l.close()
               command = 'kill -INT `cat /mnt/temp/r.pid`'
               os.system(command)
         elif start == 1 and button1 == 0:
            start = 0
            button1_hold = 0

         if send_cmd_vel == 1:
            # Prints the values for axis0
            axis0 = pygame.joystick.Joystick(0).get_axis(0)
            axis1 = pygame.joystick.Joystick(0).get_axis(1)
            # rospy.logdebug("axis 0: %f", axis0)
            # rospy.logdebug("axis 1: %f", axis1)
            # rospy.logdebug("button 0: %d", pygame.joystick.Joystick(0).get_button(0))

            vel_msg.linear.x = -.5 * axis1
            vel_msg.angular.z = -1.3 * axis0
            # vel_msg.linear.x = .5*old_linear + .5* -.3 * axis1
            # vel_msg.angular.z = .5*old_angular + .5* -0.7 * axis0
            old_linear = vel_msg.linear.x
            old_angular = vel_msg.angular.z

            self.motor_command_pub.publish(vel_msg)

         #press button 2 to begin recording rosbag
         button2 = pygame.joystick.Joystick(0).get_button(1)
         if button2 == 1:
            button2_hold += 1

            #start recording if button2 held down
            if button2_hold == 20:
               if recording_start == 0:
                  recording_start = 1
                  rospy.loginfo('Starting recording')
                  self.toggle_led()

                  topics = rospy.get_published_topics()
                  print topics
                  planning_mode = 0
                  for x in topics:
                     if "passthrough" in x[0]:
                        planning_mode = 1
                     elif "output_video" in x[0]:
                        planning_mode = 2

                  if planning_mode == 1:
                     self.record_bag_debugging()
                  elif planning_mode == 2:
                     self.record_bag_debugging_cnn()
                  else:
                     self.record_bag()

                  #start a new timer to toggle the led when recording complete
                  #t = threading.Timer(122,self.toggle_led) #run after 4 minutes
                  # t.start()
               else:
                  recording_start = 0
                  rospy.loginfo('Stopping recording')
                  self.toggle_led()
                  self.stop_record_bag()
               button2_hold = 0
         else:
            button2_hold = 0

         #press button 3 to stop robot
         button3 = pygame.joystick.Joystick(0).get_button(2)
         if robot_stop == 0 and button3 == 1:
            robot_stop = 1
            rospy.loginfo('Stopping robot')
            self.robot_stop_pub.publish(Empty())
            autonomous_led_state = autonomous_led_state ^ 1
            aled = Int8()
            aled.data = autonomous_led_state
            self.autonomous_led_pub.publish(aled)
         elif robot_stop == 1 and button3 == 1:
            button3 = 1
         else:
            robot_stop = 0

         #press button 4 to send navigation goal
         button4 = pygame.joystick.Joystick(0).get_button(3)
         if autonomous == 0 and button4 == 1:
            autonomous = 1
            rospy.loginfo('Toggle autonomous mode')
            autonomous_command = Int8()
            autonomous_command.data = 1 #switch to autonomous mode
            self.autonomous_pub.publish(autonomous_command)
         elif autonomous == 1 and button4 == 1:
            button4_hold += 1

            if button4_hold == 20:
               #cancel all goals
               autonomous_command = Int8()
               autonomous_command.data = 2 #cancel all goals
               self.autonomous_pub.publish(autonomous_command)
               button4_hold = 0
         else:
            autonomous = 0
            button4_hold = 0

         #check for shutdown button
         button10 = pygame.joystick.Joystick(0).get_button(9)
         if button10 == 1:
            #p = subprocess.Popen("ps", "a | grep \"roslaunch jet\"")
            #    out = p.communicate()
            out = "test"
            print out

         r_time.sleep()

if __name__ == "__main__":
    try:
        node = JoystickNode()
        node.run()
    except rospy.ROSInterruptException:
        pass
    rospy.loginfo("Exiting")

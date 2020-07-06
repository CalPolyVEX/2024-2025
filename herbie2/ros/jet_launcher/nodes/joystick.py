#!/usr/bin/python
import pygame
from time import sleep
import rospy, sys, os
from geometry_msgs.msg import Twist
from std_msgs.msg import Empty
from std_msgs.msg import Int8
from std_msgs.msg import Int16
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
         print ("No joysticks found.")
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
      self.arduino_cmd_pub = rospy.Publisher('/arduino_cmd', Int16, queue_size=1)

      rospy.sleep(1)

      #workaround for joystick bug
      arduino_val = Int16()
      ready=0
      button1 = 0
      # pygame.event.pump()
      # while pygame.joystick.Joystick(0).get_button(0) == 1:
      #    print ("stuck")
      #    pygame.event.pump()
      #    if button1 == 0:
      #       l = Lcd()
      #       l.init_serial_port()
      #       l.clear_screen()
      #       l.print_string('Press button 1.')
      #       l.close()
      #       button1 = 1
      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.print_string('Ready to start.')
         l.close()

      while ready==0:
         #wait for buttons 3 and 4 to be pressed simultaneously
         sleep(.1)
         pygame.event.pump()
         button2 = pygame.joystick.Joystick(0).get_button(2)
         button3 = pygame.joystick.Joystick(0).get_button(3)
         if button2==1 and button3==1:
            ready = 1
            for i in range(3):
               #blink the blue LED on the encoder shield 3 times
               arduino_val.data = 1
               self.arduino_cmd_pub.publish(arduino_val)
               sleep(.2)
               arduino_val.data = 0
               self.arduino_cmd_pub.publish(arduino_val)
               sleep(.2)
            sleep(2)

      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.close()

   def record_bag(self):
      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.print_string('Recording...')
         l.close()

      rec_topics = "rosbag record /zed_node/rgb/image_rect_color \
         /zed_node/depth/depth_registered /zed_node/rgb/camera_info \
         /tf /tf_static /ekf_node/odom /roboclaw_twist /cmd_vel /scan_filtered \
         __name:=my_bag_recorder"
      proc1 = subprocess.Popen('cd /mnt/temp;' + rec_topics, shell=True)

   def record_bag_debugging(self):
      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.print_string('Recording...')
         l.close()

      rec_topics = "rosbag record \
         /zed/data_throttled_image /zed/data_throttled_camera_info /laser_scan_filtered \
         /tf /tf_static /ekf_node/odom /obstacles_cloud \
         /passthrough/output /gps/fix \
         /planner/move_base/local_planner/local_costmap \
         /planner/move_base/local_planner/local_costmap_updates /autonomous /map \
         __name:=my_bag_recorder"
      proc1 = subprocess.Popen('cd /mnt/temp;' + rec_topics, shell=True)

   def record_bag_debugging_cnn(self):
      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.print_string('Recording...')
         l.close()

      rec_topics = "rosbag record /image_converter/output_video /planner/move_base/status \
         /zed/data_throttled_image /zed/data_throttled_camera_info /laser_scan_filtered \
         /tf /tf_static /ekf_node/odom \
         /planner/move_base/local_planner/local_costmap \
         /planner/move_base/local_planner/local_costmap_updates /map \
         __name:=my_bag_recorder"
      proc1 = subprocess.Popen('cd /mnt/temp;' + rec_topics, shell=True)

   def stop_record_bag(self):
      l = Lcd()
      if l.init_serial_port() == True:
         l.clear_screen()
         l.print_string('Stop Recording.')
         l.close()
      proc1 = subprocess.Popen('cd /mnt/temp;rosnode kill /my_bag_recorder', shell=True)

   def run(self):
      old_linear = 0
      old_angular = 0
      start = 0
      send_cmd_vel = 0
      recording_start = 0
      recording_hold = 0
      robot_stop = 0
      autonomous = 0
      autonomous_led_state = 0
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
      arduino_val = Int16()

      #do not print SDL messages
      sys.stdout = open(os.devnull, "w")
      sys.stderr = open(os.devnull, "w")
      # sys.stdout = os.devnull
      # sys.stderr = os.devnull

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
               #ending program and all nodes
               print ("Button1 hold")

               # l = Lcd()
               # l.init_serial_port()
               # l.clear_screen()
               # l.print_string('Exiting...')
               # l.close()
               # command = 'kill -INT `cat /tmp/ramdisk/r.pid`'
               # os.system(command)
               l = Lcd()
               if l.init_serial_port() == True:
                   l.clear_screen()
                   l.print_string('Exiting...')
                   l.close()

               #blink LED 3 times
               for i in range(3):
                  arduino_val.data = 1
                  self.arduino_cmd_pub.publish(arduino_val)
                  sleep(.2)
                  arduino_val.data = 0
                  self.arduino_cmd_pub.publish(arduino_val)
                  sleep(.2)
               sleep(1)
               sys.exit()
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

                  #turn on the LED send a command to the Arduino
                  arduino_val.data = 1 #turn on blue LED
                  self.arduino_cmd_pub.publish(arduino_val)
                  self.arduino_cmd_pub.publish(arduino_val)

                  topics = rospy.get_published_topics()
                  print (topics)
                  planning_mode = 0
                  for x in topics:
                     if "passthrough" in x[0]:
                        planning_mode = 1
                     #elif "output_video" in x[0]:
                     #   planning_mode = 2

                  if planning_mode == 1:
                     self.record_bag_debugging()
                  elif planning_mode == 2:
                     self.record_bag_debugging_cnn()
                  else:
                     self.record_bag()

               else:
                  recording_start = 0
                  rospy.loginfo('Stopping recording')
                  self.stop_record_bag()

                  #turn off the LED send a command to the Arduino
                  arduino_val.data = 0 #turn off blue LED
                  self.arduino_cmd_pub.publish(arduino_val)
                  self.arduino_cmd_pub.publish(arduino_val)
         else:
            if button2_hold != 0:
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

         r_time.sleep()

if __name__ == "__main__":
    try:
        node = JoystickNode()
        node.run()
    except rospy.ROSInterruptException:
        pass
    rospy.loginfo("Exiting")

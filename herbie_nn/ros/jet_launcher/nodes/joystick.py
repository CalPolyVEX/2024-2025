#!/usr/bin/python
import pygame
from time import sleep
import rospy, sys, os
from geometry_msgs.msg import Twist
from geometry_msgs.msg import PoseWithCovarianceStamped
from std_msgs.msg import Empty
from std_msgs.msg import Int8
from std_msgs.msg import Int16
from std_msgs.msg import Int32MultiArray
import subprocess, threading
from array import array

crctable = array('H', [
   0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
   0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
   0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
   0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
   0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
   0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
   0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
   0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
   0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
   0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
   0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
   0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
   0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
   0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
   0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
   0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
   0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
   0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
   0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
   0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
   0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
   0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
   0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
   0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
   0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
   0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
   0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
   0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
   0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
   0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
   0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
   0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0])

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

      self.motor_command_pub = rospy.Publisher('/manual_cmd_vel', Twist, queue_size=2)
      self.robot_stop_pub = rospy.Publisher('/robot_stop', Empty, queue_size=1)
      self.autonomous_pub = rospy.Publisher('/autonomous', Int8, queue_size=1)
      self.autonomous_led_pub = rospy.Publisher('/read_encoder_cmd', Int8, queue_size=1)
      self.herbie_board_pub = rospy.Publisher('/control_board', Int32MultiArray, queue_size=5)
      self.nav_goal_pub = rospy.Publisher('/nav_goal', Empty, queue_size=1)

      rospy.sleep(1)

      #workaround for joystick bug
      ready=0
      button1 = 0

      p = Int32MultiArray()
      p.data = self.create_cursor_packet(0,1)
      self.herbie_board_pub.publish(p)

      p = Int32MultiArray()
      p.data = self.create_string_packet('Ready to start.')
      self.herbie_board_pub.publish(p)

      while ready==0:
         #wait for buttons 3 and 4 to be pressed simultaneously
         sleep(.1)

         pygame.event.pump()
         button2 = pygame.joystick.Joystick(0).get_button(2)
         button3 = pygame.joystick.Joystick(0).get_button(3)
         if button2==1 and button3==1:
            ready = 1
            for i in range(3):
               #blink the yellow LED 3 on the Herbie board 3 times
               p = Int32MultiArray()
               p.data = self.create_led_packet(3,1) #led on
               self.herbie_board_pub.publish(p)
               sleep(.2)

               p = Int32MultiArray()
               p.data = self.create_led_packet(3,0) #led off
               self.herbie_board_pub.publish(p)
               sleep(.2)
            sleep(.2)

      p = Int32MultiArray()
      p.data = self.create_cursor_packet(0,1)
      self.herbie_board_pub.publish(p)

      p = Int32MultiArray()
      p.data = self.create_string_packet('                ')
      self.herbie_board_pub.publish(p)

   def record_bag(self):
      p = Int32MultiArray()
      p.data = self.create_cursor_packet(0,1)
      self.herbie_board_pub.publish(p)

      p = Int32MultiArray()
      p.data = self.create_string_packet('R++++   ')
      self.herbie_board_pub.publish(p)

      rec_topics = "rosbag record \
         /nav_output_video/compressed /see3cam_cu20/image_raw_live/compressed /point_cloud \
         /move_base/local_costmap/costmap /move_base/global_costmap/costmap /tf /tf_static /ekf_node/odom /roboclaw_twist /cmd_vel \
         /move_base/DWAPlannerROS/local_plan /move_base/local_costmap/footprint /move_base/DWAPlannerROS/global_plan /move_base/TebLocalPlannerROS/global_plan /move_base/TebLocalPlannerROS/local_plan \
         __name:=my_bag_recorder"
      proc1 = subprocess.Popen('cd /mnt/temp;' + rec_topics, shell=True)

   def record_bag_debugging(self):
      p = Int32MultiArray()
      p.data = self.create_cursor_packet(0,1)
      self.herbie_board_pub.publish(p)

      p = Int32MultiArray()
      p.data = self.create_string_packet('R....')
      self.herbie_board_pub.publish(p)

      rec_topics = "rosbag record \
         /tf /tf_static /ekf_node/odom /obstacles_cloud \
         /autonomous /map \
         __name:=my_bag_recorder"
      proc1 = subprocess.Popen('cd /mnt/temp;' + rec_topics, shell=True)

   def stop_record_bag(self):
      p = Int32MultiArray()
      p.data = self.create_cursor_packet(0,1)
      self.herbie_board_pub.publish(p)

      p = Int32MultiArray()
      p.data = self.create_string_packet('SR....')
      self.herbie_board_pub.publish(p)

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
      servo_toggle = 0

      # Prints the joystick's name
      JoyName = pygame.joystick.Joystick(0).get_name()
      rospy.logdebug("Name of the joystick: %s", JoyName)
      # Gets the number of axes
      JoyAx = pygame.joystick.Joystick(0).get_numaxes()
      rospy.logdebug("Number of axes: %d", JoyAx)

      # Gets the number of buttons
      JoyButtons = pygame.joystick.Joystick(0).get_numbuttons()
      rospy.logdebug("Number of buttons: %d", JoyButtons)

      r_time = rospy.Rate(10) #run the loop at 10Hz
      vel_msg = Twist()

      #do not print SDL messages
      sys.stdout = open(os.devnull, "w")
      sys.stderr = open(os.devnull, "w")

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

               p = Int32MultiArray()
               p.data = self.create_cursor_packet(0,0)
               self.herbie_board_pub.publish(p)

               p = Int32MultiArray()
               p.data = self.create_string_packet('Exiting...')
               self.herbie_board_pub.publish(p)

               #blink LED 3 times
               for i in range(3):
                  p = Int32MultiArray()
                  p.data = self.create_led_packet(3,1)
                  self.herbie_board_pub.publish(p)
                  sleep(.2)

                  p = Int32MultiArray()
                  p.data = self.create_led_packet(3,0)
                  self.herbie_board_pub.publish(p)
                  sleep(.2)

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

            vel_msg.linear.x = -.4 * axis1
            vel_msg.angular.z = -1.0 * axis0
            old_linear = vel_msg.linear.x
            old_angular = vel_msg.angular.z

            self.motor_command_pub.publish(vel_msg)

         #press button 2 to begin recording rosbag ###########################
         button2 = pygame.joystick.Joystick(0).get_button(1)
         if button2 == 1:
            button2_hold += 1

            #start recording if button2 held down
            if button2_hold == 20:
               if recording_start == 0:
                  recording_start = 1
                  rospy.loginfo('Starting recording')

                  #turn on the LED send a command to the Arduino
                  #TODO

                  topics = rospy.get_published_topics()
                  print (topics)
                  planning_mode = 0
                  for x in topics:
                     if "passthrough" in x[0]:
                        planning_mode = 1

                  self.record_bag()

               else:
                  recording_start = 0
                  rospy.loginfo('Stopping recording')
                  self.stop_record_bag()

                  #turn off the LED send a command to the Arduino
                  #TODO
         else:
            if button2_hold != 0:
                button2_hold = 0

         #press button 3 to stop robot #######################################
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

         #press button 4 to send navigation goal #############################
         button4 = pygame.joystick.Joystick(0).get_button(3)
         if autonomous == 0 and button4 == 1:
            autonomous = 1
            rospy.loginfo('Toggle autonomous mode')
            autonomous_command = Int8()
            autonomous_command.data = 1 #switch to autonomous mode
            self.autonomous_pub.publish(autonomous_command)
         elif autonomous == 1 and button4 == 1:
            button4_hold += 1

            if button4_hold == 3:
               #cancel all goals
               autonomous_command = Int8()
               autonomous_command.data = 0 #cancel all goals
               self.autonomous_pub.publish(autonomous_command)
               button4_hold = 0
         else:
            autonomous = 0
            button4_hold = 0

         #press button 8 to toggle servo position ############################
         button8 = pygame.joystick.Joystick(0).get_button(7)
         if button8 == 1:
            print "--change servo position--"

            button8_hold += 1

            if button8_hold == 5:
                if servo_toggle == 0:
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(0,100)
                    self.herbie_board_pub.publish(p)
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(1,100)
                    self.herbie_board_pub.publish(p)
                    servo_toggle = 1
                elif servo_toggle == 1:
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(0,1900)
                    self.herbie_board_pub.publish(p)
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(1,1900)
                    self.herbie_board_pub.publish(p)
                    servo_toggle = 2
                else:
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(0,1000)
                    self.herbie_board_pub.publish(p)
                    p = Int32MultiArray()
                    p.data = self.create_servo_packet(1,1000)
                    self.herbie_board_pub.publish(p)
                    servo_toggle = 0

                button8_hold = 0
         else:
            button8_hold = 0

         #press button 7 to send navigation goal ############################
         button7 = pygame.joystick.Joystick(0).get_button(6)
         if button7 == 1:
            print "--sending goal--"

            button7_hold += 1

            if button7_hold == 5:
                p = Empty()
                self.nav_goal_pub.publish(p)

                button7_hold = 0
         else:
            button7_hold = 0

         r_time.sleep()

   def create_clear_screen_packet(self):
      crc = 0
      packet = []
      # bytearray(9)

      packet.append(8)      #packet size
      packet.append(128)    #roboclaw address

      for i in range(5):
          packet.append(0)

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,7):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet

   def create_led_packet(self, num, state):
      crc = 0
      packet = []
      # bytearray(9)

      packet.append(8)      #packet size
      packet.append(128)    #roboclaw address

      if state == 1:
          packet.append(7)  # led on
      else:
          packet.append(8)  # led off

      packet.append(num) #led num

      for i in range(3):
          packet.append(0)

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,7):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet

   def create_servo_packet(self, num, position):
      crc = 0
      packet = []

      packet.append(8)      #packet size
      packet.append(128)    #roboclaw address

      packet.append(6)      #servo command

      packet.append(num)    #servo num
      packet.append((position >> 8) & 0xff)
      packet.append(position  & 0xff)
      packet.append(0)

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,7):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet

   def create_string_packet(self, string):
      crc = 0
      packet = []

      packet.append(len(string) + 5)      #packet size
      packet.append(128)    #roboclaw address

      packet.append(2)      #print string command

      packet.append(len(string))    #length of string in bytes

      for i in range(len(string)):
        packet.append(ord(string[i]))

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,len(string)+4):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet

   def create_cursor_packet(self, col, row):
      crc = 0
      packet = []

      packet.append(8)      #packet size
      packet.append(128)    #roboclaw address

      packet.append(1)      #cursor command

      packet.append(col)    #column
      packet.append(row)    #row
      packet.append(0)
      packet.append(0)

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,7):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet

   def create_int_packet(self, val):
      crc = 0
      packet = []

      packet.append(8)      #packet size
      packet.append(128)    #roboclaw address

      packet.append(3)      #cursor command

      packet.append(val & 0xff)    #column
      packet.append((val >> 8) & 0xff)    #row
      packet.append((val >> 16) & 0xff)
      packet.append((val >> 24) & 0xff)

      #Calculates CRC16 of nBytes of data in byte array message
      for byte in range(1,7):
          crc = (((crc << 8) & 0xffff) ^ crctable[((crc >> 8) ^ packet[byte]) & 0xff]) & 0xffff;

      packet.append((crc >> 8) & 0xFF) #send the high byte of the crc
      packet.append(crc & 0xFF) #send the low byte of the crc

      return packet


if __name__ == "__main__":
    try:
        node = JoystickNode()
        node.run()
    except rospy.ROSInterruptException:
        pass
    rospy.loginfo("Exiting")

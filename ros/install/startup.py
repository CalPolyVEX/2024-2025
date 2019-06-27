#!/usr/bin/python
import pygame

from time import sleep
import rospy, sys, os, time
import subprocess, threading

class JoystickNode:
   def __init__(self):
      os.environ["SDL_VIDEODRIVER"] = "dummy"
      pygame.joystick.init()

      #check the number of joysticks
      num_joystick = pygame.joystick.get_count()
      if num_joystick==0:
         print ("No joysticks found.")
         exit(0)

      pygame.display.init()
      pygame.joystick.Joystick(0).init()

   def run(self):
      # Prints the joystick's name
      JoyName = pygame.joystick.Joystick(0).get_name()
      print("Name of the joystick: %s", JoyName)
      # Gets the number of axes
      JoyAx = pygame.joystick.Joystick(0).get_numaxes()
      print("Number of axes: %d", JoyAx)

      start_time = time.time()
      print start_time
      button10 = 0
      print time.time() - start_time

      #wait for a button press for 10 seconds
      while (time.time() - start_time) < 10.0:
         pygame.event.pump()
         sleep(.1)
         button10 = pygame.joystick.Joystick(0).get_button(9)
         if button10 == 1:
            print "button pressed"
            break;
      if button10 == 0:
         print "timeout"
      else:
         #run the launch file
         pygame.quit()
         command = 'roslaunch jet_launcher robot.launch localization:=true planning:=true'
         os.system(command)

if __name__ == "__main__":
   node = JoystickNode()
   node.run()

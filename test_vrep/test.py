import vrep
import sys
import numpy as np

class _Getch:
    """Gets a single character from standard input.  Does not echo to the
screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()


class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch


class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()

def initConnection():
   global clientID

   vrep.simxFinish(-1) #close all opened connections
   clientID = vrep.simxStart('127.0.0.1', 19999, True, True, 5000, 5)

   if clientID != -1:
      print ("Connected to remote API server")
   else:
      print ("Not connected to remote API server")
      sys.exit("Could not connect")

clientID = 0
initConnection()

#get handles to the left and right motor
err_code,l_motor_handle = vrep.simxGetObjectHandle(clientID,"Pioneer_p3dx_leftMotor", vrep.simx_opmode_blocking)
err_code,r_motor_handle = vrep.simxGetObjectHandle(clientID,"Pioneer_p3dx_rightMotor", vrep.simx_opmode_blocking)
err_code,laser_scanner_handle = vrep.simxGetObjectHandle(clientID,"Pioneer_LaserScanner_2d", vrep.simx_opmode_blocking)
err_code,signal = vrep.simxGetStringSignal(clientID,"test", vrep.simx_opmode_streaming)


getch = _Getch()
a = 'a'

while True:
   #a = raw_input('')
   #print a
   if a == 's':
      err_code = vrep.simxSetJointTargetVelocity(clientID,l_motor_handle,0,vrep.simx_opmode_blocking)
      err_code = vrep.simxSetJointTargetVelocity(clientID,r_motor_handle,0,vrep.simx_opmode_blocking)
   else:
      err_code = vrep.simxSetJointTargetVelocity(clientID,l_motor_handle,1.0,vrep.simx_opmode_blocking)
      err_code = vrep.simxSetJointTargetVelocity(clientID,r_motor_handle,.20,vrep.simx_opmode_blocking)

   err_code,signal = vrep.simxGetStringSignal(clientID,"test", vrep.simx_opmode_buffer)
   
   x = np.fromstring(signal, dtype='>f2')
   #print '--------------'
   #for i in range(9):
   #   print x[i]
   print x.size
      

   #key = getch()
   #print key

vrep.simxFinish(-1) #close all opened connections

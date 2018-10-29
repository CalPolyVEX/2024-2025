import vrep
import sys

vrep.simxFinish(-1) #close all opened connections
clientID = vrep.simxStart('127.0.0.1', 19999, True, True, 5000, 5)

if clientID != -1:
   print ("Connected to remote API server")
else:
   print ("Not connected to remote API server")
   sys.exit("Could not connect")

err_code,l_motor_handle = vrep.simxGetObjectHandle(clientID,"Pioneer_p3dx_leftMotor", vrep.simx_opmode_blocking)
err_code,r_motor_handle = vrep.simxGetObjectHandle(clientID,"Pioneer_p3dx_rightMotor", vrep.simx_opmode_blocking)

print err_code

err_code = vrep.simxSetJointTargetVelocity(clientID,l_motor_handle,1.0,vrep.simx_opmode_blocking)

#!/usr/bin/python

#max resolution: 740x415
#(18,45) = (527,339)
#(27, 27) = (673,409)
#(9,63) = (424,285)
#(54,54) = (708,283)
#(0,36) = (357,394)

import cv2, numpy as np, sys

objectPoints = np.array([[18,0,45], 
                        [27,0,27], 
                        [9,0,63], 
                        [54,0,54], 
                        [0,0,36]], dtype=np.float32)
objectPoints = .0254*objectPoints #convert to meters

imagePoints = np.array([[527,339], 
                        [673,409], 
                        [424,285], 
                        [708,283], 
                        [357,394]], dtype=np.float32)
imagePoints = 2.6 * imagePoints #scale to 1920x1080

#intrinsic matrix
cameraMatrix = np.array([[1258.513767, 0.000000, 949.143263],
                         [0.000000, 1260.515476, 587.553871],
                         [0.000000, 0.000000, 1.000000]])

#distortion coefficients
distCoeffs = np.array([[-0.350545], [0.098685], [-0.004605], [-0.001945], [0.000000]])

retval, rvec, tvec = cv2.solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, flags=cv2.SOLVEPNP_ITERATIVE)
#retval, rvec, tvec = cv2.solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs)
print "rotation vector:"
print rvec
print "translation vector:"
print tvec

rmat, rmat_jacobian = cv2.Rodrigues(rvec)
print "rotation matrix:"
print rmat

#concatenate
r_t_max = np.concatenate((rmat,tvec), axis=1)
print "r|t matrix:"
print r_t_max

print 'A matrix:'
A = np.matmul(cameraMatrix, r_t_max)
print A
A = np.delete(A,1,1)
print A

#compute inverse of r_t
r_t_inv = np.linalg.inv(A)
print "inverse r|t:"
print r_t_inv

print "---testing---"
imagepoint = np.array([2.6*527,2.6*339,1],dtype=np.float32)
ans = np.matmul(r_t_inv, imagepoint)
ans = ans / .0254 #convert back to inches
print ans
print objectPoints

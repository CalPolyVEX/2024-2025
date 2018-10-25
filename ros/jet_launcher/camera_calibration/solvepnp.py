#!/usr/bin/python

import cv2, numpy as np

objectPoints = np.random.random((10,3,1))
imagePoints = np.random.random((10,2,1))

#intrinsic matrix
cameraMatrix = np.array([[1258.513767, 0.000000, 949.143263],
                         [0.000000, 1260.515476, 587.553871],
                         [0.000000, 0.000000, 1.000000]])

#distortion coefficients
distCoeffs = np.array([[-0.350545], [0.098685], [-0.004605], [-0.001945], [0.000000]])

retval, rvec, tvec = cv2.solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs)
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

A = np.matmul(cameraMatrix, r_t_max)
print A
A = np.delete(A,2,1)
print A

#compute inverse of r_t
r_t_inv = np.linalg.inv(A)
print "inverse r|t:"
print r_t_inv


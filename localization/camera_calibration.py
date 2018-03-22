import numpy as np
import cv2, sys, time
import glob

def capture_image():
   #capture images from camera
   # termination criteria
   criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

   # prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
   objp = np.zeros((6*7,3), np.float32)
   objp[:,:2] = np.mgrid[0:7,0:6].T.reshape(-1,2)

   # Arrays to store object points and image points from all the images.
   objpoints = [] # 3d point in real world space
   imgpoints = [] # 2d points in image plane.

   vidcap = cv2.VideoCapture(sys.argv[1])

   print vidcap
   vidcap.set(3,1920) #capture width
   vidcap.set(4,1080) #capture height
   vidcap.set(cv2.CAP_PROP_FPS,30) #capture FPS

   counter = 0
   for i in range(3000):
      success,image = vidcap.read()

      print success
      if success:
         fps = vidcap.get(cv2.CAP_PROP_FPS)
         height,width,channels = image.shape
         print fps
         print width
         print height

         gray = cv2.cvtColor(image,cv2.COLOR_BGR2GRAY)

         # Find the chess board corners
         ret, corners = cv2.findChessboardCorners(gray, (7,6),None)

         # If found, add object points, image points (after refining them)
         if ret == True:
            objpoints.append(objp)

            cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
            imgpoints.append(corners)

            # Draw and display the corners
            cv2.drawChessboardCorners(image, (7,6), corners,ret)
            cv2.imshow('img',image)
            cv2.waitKey(500)
            counter += 1
            print counter

            if counter == 10:
               break
         else:
            cv2.destroyAllWindows()
   ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, gray.shape[::-1],None,None)
   print mtx

   tot_error = 0
   for i in xrange(len(objpoints)):
      imgpoints2, _ = cv2.projectPoints(objpoints[i], rvecs[i], tvecs[i], mtx, dist)
      error = cv2.norm(imgpoints[i],imgpoints2, cv2.NORM_L2)/len(imgpoints2)
      tot_error += error

   print "total error: ", tot_error/len(objpoints)

capture_image()


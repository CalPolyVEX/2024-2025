#!/usr/bin/python

#this file computes the matrix to transform coordinates in the image plane
#to coordinates in 3-d space

import cv2, numpy as np, sys

class camera_transform:
   def __init__(self):
      #image scaling factor
      #if the image is 960x540, then set to 1.0
      #if the image is 480x270, then set to 2.0
      self.image_scaling = 1.00000000 #if the image is 960x540, then set to 1.0

      #The objectpoints array has the coordinates that are measured on the ground (inches)
      #
      #The order of the points is X,Y,Z where +X is towards the left of the robot
      #+Y is down and +Z is towards the front of the robot
      self.objectPoints = np.array([[
        [0,0,12],
        [0,0,24],
        [0,0,36],
        [0,0,48],
        [0,0,60],
        [12,0,12],
        [12,0,24],
        [12,0,36],
        [12,0,48],
        [12,0,60],
        [-12,0,12],
        [-12,0,24],
        [-12,0,36],
        [-12,0,48],
        [-12,0,60],


        [24,0,12],
        [24,0,24],
        [24,0,36],
        [24,0,48],
        [24,0,60],
        [24,0,72],
        [-24,0,12],
        [-24,0,24],
        [-24,0,36],
        [-24,0,48],
        [-24,0,60],
        [-24,0,72],


        [-36,0,12],
        [-36,0,24],
        [-36,0,36],
        [-36,0,48],
        [-36,0,60],
        [36,0,12],
        [36,0,24],
        [36,0,36],
        [36,0,48],
        [36,0,60]
        ]],
        dtype=np.float64)
      self.objectPoints = .0254*self.objectPoints #convert to meters

      print (self.objectPoints.shape)

      #The imagepoints array has the coordinates of the corresponding points as they
      #appear in the image.  The order of the points is col,row where the origin is
      #the upper left
      self.imagePoints = np.array([[
        [310,354],
        [310,266],
        [310,216],
        [310,187],
        [310,168],
        [177,333],
        [212,260],
        [234,213],
        [249,185],
        [261,166],
        [441,336],
        [406,260],
        [382,215],
        [368,187],
        [357,168],


        [91,310],
        [136,254],
        [172,213],
        [199,187],
        [217,169],
        [231,156],
        [526,311],
        [484,254],
        [450,215],
        [424,189],
        [405,171],
        [391,156],


        [568,288],
        [529,246],
        [492,213],
        [463,188],
        [440,171],
        [47,283],
        [87,241],
        [123,210],
        [152,186],
        [175,169]
        ]],
        dtype=np.float64)
      self.imagePoints = self.imagePoints / self.image_scaling #scale to 480x270

      print (self.imagePoints.shape)

      assert len(self.imagePoints) == len(self.objectPoints)

      #intrinsic matrix
      #2.8mm lens
      self.cameraMatrix = np.array([[336.57498971 , 0.000000, 312.10160575],
                            [0.000000,   335.25594161, 181.58579683],
                            [0.000000,   0.000000,   1.000000]])

      #scale the camera matrix to 480x270, so divide by 4
      #self.cameraMatrix = self.cameraMatrix / self.image_scaling

      #set the 2,2 element back to 1 in the camera matrix
      #self.cameraMatrix[2,2] = 1
      print ("Camera Matrix:")
      print (self.cameraMatrix)

      #distortion coefficients
      #2.8mm lens
      #self.distCoeffs = np.array([[-0.360448], [0.088215], [0.000181], [-0.001589], [0.000000]])
      self.distCoeffs = np.array([[-0.35182242], [0.10972241], [0.00142039], [-0.00209606], [-0.01479889]])

      self.retval, self.rvec, self.tvec = cv2.solvePnP(self.objectPoints, self.imagePoints, self.cameraMatrix, self.distCoeffs, flags=cv2.SOLVEPNP_ITERATIVE)
      #self.retval, self.rvec, self.tvec, self.inliers = cv2.solvePnPRansac(self.objectPoints, self.imagePoints, self.cameraMatrix, self.distCoeffs, flags=cv2.SOLVEPNP_EPNP)

      self.rmat, self.rmat_jacobian = cv2.Rodrigues(self.rvec)

      #concatenate r and t matrix
      self.r_t_max = np.concatenate((self.rmat,self.tvec), axis=1)

      #multiply intrinsic matrix with R|t matrix
      self.A = np.matmul(self.cameraMatrix, self.r_t_max)

      #remove the second column because it is a plane
      self.A = np.delete(self.A,1,1)
      #self.A = np.delete(self.A,2,1)

      #compute the inverse of the A matrix
      self.r_t_inv = np.linalg.inv(self.A)
      print ("r_t_inv Matrix:")
      print ("------------copy from here-----------------")
      counter = 0;
      for x in np.nditer(self.r_t_inv, order = 'C'):
          if counter == 0:
              print ("{", end="")
          counter += 1
          if counter != 3:
              print (x, end="")
              print (",", end="")
          else:
              print (x, end="")
              print ("},")
              counter = 0
      print ("------------end copy from here-----------------")

   def compute(self,x,y):
      imagepoint = np.array([x,y,1],dtype=np.float64)
      ans = np.matmul(self.r_t_inv, imagepoint)
      # print ans
      w = ans.item(2)
      ans = ans / w
      #ans = ans / .0254 #convert back to inches
      return ans.tolist()[0:2] #answer is in meters

if __name__ == '__main__':
   c = camera_transform()
   print ("---testing---")
   print ("0,12")
   ans = c.compute(310,354)
   print ([x/.0254 for x in ans])

   print ("24,72")
   ans = c.compute(231,156)
   print ([x/.0254 for x in ans])

   print ("-24,72")
   ans = c.compute(391,156)
   print ([x/.0254 for x in ans])

   print ("0,24")
   ans = c.compute(310,266)
   print ([x/.0254 for x in ans])

   print ("12,60")
   ans = c.compute(261,166)
   print ([x/.0254 for x in ans])

   print ("-12,12")
   ans = c.compute(441,336)
   print ([x/.0254 for x in ans])

   print ("-36,48")
   ans = c.compute(463,188)
   print ([x/.0254 for x in ans])

   print ("36,48")
   ans = c.compute(152,186)
   print ([x/.0254 for x in ans])
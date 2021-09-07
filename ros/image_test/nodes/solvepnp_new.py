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
      #The order of the points is X,Y,Z where +X is towards the right of the robot
      #+Y is down and +Z is towards the front of the robot
      self.objectPoints = np.array([[
        [0,0,15],
        [0,0,18],
        [0,0,24],
        [0,0,36],
        [0,0,60],
        [0,0,96],
        [0,0,120],
        [0,0,168],
        [46.5,0,49],
        [46.5,0,84],
        [29,0,20],
        [18,0,18],
        [47,0,141],
        [-45,0,36],
        [-18,0,18],
        [-6,0,18],
        [-45,0,48],
        [-45,0,78],
        [-37.75,0,115]]],
        dtype=np.float64)
      self.objectPoints = .0254*self.objectPoints #convert to meters

      print (self.objectPoints.shape)

      #The imagepoints array has the coordinates of the corresponding points as they
      #appear in the image.  The order of the points is col,row where the origin is
      #the upper left
      self.imagePoints = np.array([[
        [470,532],
        [470,488],
        [471,417],
        [470,330],
        [472,251],
        [472,207],
        [471,190],
        [472,174],
        [873,280],
        [746,220],
        [926,401],
        [839,442],
        [649,184],
        [19,315],
        [108,446],
        [325,477],
        [76,282],
        [184,229],
        [297,196]]],
        dtype=np.float64)
      self.imagePoints = self.imagePoints / self.image_scaling #scale to 480x270

      print (self.imagePoints.shape)

      #self.new_imagePoints = np.ascontiguousarray(self.imagePoints[:,:2]).reshape((N,1,2))

      #assert len(self.imagePoints) == len(self.objectPoints)

      #intrinsic matrix
      #3.6mm lens
      # self.cameraMatrix = np.array([[1258.513767, 0.000000, 949.143263],
      #                         [0.000000, 1260.515476, 587.553871],
      #                         [0.000000, 0.000000, 1.000000]])

      #1.8mm lens
      # self.cameraMatrix = np.array([[2733.727422, 0.000000, 995.635599],
      #                               [0.000000, 2653.464732, 563.684444],
      #                               [0.000000, 0.000000, 1.000000]])

      #2.5mm lens
      # self.cameraMatrix = np.array([[2643.752237, 0.000000, 966.715934],
      #                               [0.000000, 2589.388168, 549.378039],
      #                               [0.000000, 0.000000, 1.000000]])

      #2.8mm lens
      self.cameraMatrix = np.array([[625.988136, 0.000000,   487.889539],
                            [0.000000,   869.697237, 247.887212],
                            [0.000000,   0.000000,   1.000000]])

      #scale the camera matrix to 480x270, so divide by 4
      self.cameraMatrix = self.cameraMatrix / self.image_scaling

      #set the 2,2 element back to 1 in the camera matrix
      self.cameraMatrix[2,2] = 1
      print ("Camera Matrix:")
      print (self.cameraMatrix)

      #distortion coefficients
      #3.6mm lens
      #self.distCoeffs = np.array([[-0.350545], [0.098685], [-0.004605], [-0.001945], [0.000000]])

      #2.5mm lens
      #self.distCoeffs = np.array([[-1.131540], [1.561072], [0.021116], [-0.088527], [0.000000]])

      #2.8mm lens
      self.distCoeffs = np.array([[-0.360448], [0.088215], [0.000181], [-0.001589], [0.000000]])


      #self.distCoeffs = np.array([[0], [0], [0], [0], [0.000000]])

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
   print ("0,15")
   ans = c.compute(470,532)
   print ([x/.0254 for x in ans])

   print ("-45,36")
   ans = c.compute(19,315)
   print ([x/.0254 for x in ans])

   print ("0,18")
   ans = c.compute(470,488)
   print ([x/.0254 for x in ans])

   print ("0,72")
   ans = c.compute(467,245)
   print ([x/.0254 for x in ans])

   print ("-37.75,115")
   ans = c.compute(297,196)
   print ([x/.0254 for x in ans])

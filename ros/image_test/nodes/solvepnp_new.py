#!/usr/bin/python

#max resolution: 740x415
#(18,45) = (527,339)
#(27, 27) = (673,409)
#(9,63) = (424,285)
#(54,54) = (708,283)
#(0,36) = (357,394)

import cv2, numpy as np, sys

class camera_transform:
   def __init__(self):
      self.objectPoints = np.array([
        [9,  0,  18],
        [18, 0,  18],
        [18, 0,  27],
        [0,  0,  18],
        [0,  0,  15],
        [-18,0,  18],
        [-27,0,  27],
        [0,  0,  54],
        [0,  0,  72],
        [-36,0,  45]],
                        dtype=np.float32)
      self.objectPoints = .0254*self.objectPoints #convert to meters

      self.imagePoints = np.array([
        [624,476],
        [765,461],
        [728,353],
        [458,485],
        [456,532],
        [151,463],
        [85, 347],
        [457,146],
        [457,76],
        [88 ,208]],
                        dtype=np.float32)
      self.imagePoints = self.imagePoints / 2.0000 #scale to 480x270

      #intrinsic matrix
      self.cameraMatrix = np.array([[1258.513767, 0.000000, 949.143263],
                              [0.000000, 1260.515476, 587.553871],
                              [0.000000, 0.000000, 1.000000]])

      #scale to 480x270, so divide by 4
      self.cameraMatrix = self.cameraMatrix / 4.000

      #set the 2,2 element back to 1 in the camera matrix
      self.cameraMatrix[2,2] = 1
      print "Camera Matrix:"
      print self.cameraMatrix

      #distortion coefficients
      self.distCoeffs = np.array([[-0.350545], [0.098685], [-0.004605], [-0.001945], [0.000000]])
      #self.distCoeffs = np.array([[0], [0], [0], [0], [0.000000]])

      self.retval, self.rvec, self.tvec = cv2.solvePnP(self.objectPoints, self.imagePoints, self.cameraMatrix, self.distCoeffs, flags=cv2.SOLVEPNP_ITERATIVE)

      self.rmat, self.rmat_jacobian = cv2.Rodrigues(self.rvec)

      #concatenate r and t matrix
      self.r_t_max = np.concatenate((self.rmat,self.tvec), axis=1)

      #multiply intrinsic matrix with R|t matrix
      self.A = np.matmul(self.cameraMatrix, self.r_t_max)

      #remove the second column because it is a plane
      self.A = np.delete(self.A,1,1)

      #compute the inverse of the A matrix
      self.r_t_inv = np.linalg.inv(self.A)
      print "r_t_inv Matrix:"
      #print self.r_t_inv
      counter = 0;
      for x in np.nditer(self.r_t_inv, order = 'C'):
          if counter == 0:
              print "{",
          counter += 1
          if counter != 3:
              print x,
              print ",",
          else:
              print x,
              print "},"
              counter = 0

   def compute(self,x,y):
      imagepoint = np.array([x,y,1],dtype=np.float32)
      ans = np.matmul(self.r_t_inv, imagepoint)
      # print ans
      w = ans.item(2)
      ans = ans / w
      #ans = ans / .0254 #convert back to inches
      return ans.tolist()[0:2] #answer is in meters

if __name__ == '__main__':
   c = camera_transform()
   print "---testing---"
   print "9,18"
   ans = c.compute(624/2,476/2)
   print [x/.0254 for x in ans]

   print "0,72"
   ans = c.compute(457/2,76/2)
   print [x/.0254 for x in ans]

   print "-36,120"
   ans = c.compute(213/1.54,209/1.54)
   print [x/.0254 for x in ans]

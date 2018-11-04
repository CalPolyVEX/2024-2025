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
      self.objectPoints = np.array([[18,0,45], #in inches 
                        [27,0,27], 
                        [9,0,63], 
                        [54,0,54], 
                        [0,0,36],
                        [-18,0,36],
                        [-18,0,45],
                        [-18,0,63],
                        [0,0,90],
                        [-18,0,90],
                        [54,0,81], 
                        #data after this line gathered on 11/3/18
                        [0,0,120],
                        [-36,0,120],
                        [48,0,120],
                        [0,0,180],
                        [48,0,180],
                        [48,0,41],
                        [0,0,240]],
                        dtype=np.float32)
      self.objectPoints = .0254*self.objectPoints #convert to meters

      self.imagePoints = np.array([[527,339], #740x415 calibrate script 
                        [673,409], 
                        [424,285], 
                        [708,283], 
                        [357,394],
                        [164,380],
                        [192,339],
                        [228,284],
                        [360,232],
                        [264,235],
                        [631,236],
                        #data after this line gathered on 11/3/18
                        [356,207],
                        [213,209],
                        [538,203],
                        [360,175],
                        [487,173],
                        [733,315],
                        [358,161]],
                        dtype=np.float32)
      self.imagePoints = self.imagePoints / 1.54 #scale to 480x270

      #intrinsic matrix
      self.cameraMatrix = np.array([[1258.513767, 0.000000, 949.143263],
                              [0.000000, 1260.515476, 587.553871],
                              [0.000000, 0.000000, 1.000000]])

      #scale to 480x270, so divide by 4
      self.cameraMatrix = self.cameraMatrix / 4.000

      #set the 2,2 element back to 1 in the camera matrix
      self.cameraMatrix[2,2] = 1
      print self.cameraMatrix

      #distortion coefficients
      self.distCoeffs = np.array([[-0.350545], [0.098685], [-0.004605], [-0.001945], [0.000000]])

      self.retval, self.rvec, self.tvec = cv2.solvePnP(self.objectPoints, self.imagePoints, self.cameraMatrix, self.distCoeffs)

      self.rmat, self.rmat_jacobian = cv2.Rodrigues(self.rvec)

      #concatenate r and t matrix
      self.r_t_max = np.concatenate((self.rmat,self.tvec), axis=1)

      #multiply intrinsic matrix with R|t matrix
      self.A = np.matmul(self.cameraMatrix, self.r_t_max)

      #remove the second column because it is a plane
      self.A = np.delete(self.A,1,1)

      #compute the inverse of the A matrix
      self.r_t_inv = np.linalg.inv(self.A)

   def compute(self,x,y):
      imagepoint = np.array([x,y,1],dtype=np.float32)
      ans = np.matmul(self.r_t_inv, imagepoint)
      w = ans.item(2)
      ans = ans / w
      #ans = ans / .0254 #convert back to inches
      return ans.tolist()[0:2] #answer is in meters

if __name__ == '__main__':
   c = camera_transform()
   print "---testing---"
   print "-18,90"
   ans = c.compute(264/1.54,235/1.54)
   print [x/.0254 for x in ans]
   print "18,45"
   ans = c.compute(527/1.54,339/1.54)
   print [x/.0254 for x in ans]

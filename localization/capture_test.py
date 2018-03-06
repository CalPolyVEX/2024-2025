import cv2
import sys

vidcap = cv2.VideoCapture('/dev/video1')

print vidcap
vidcap.set(3,1280) #capture width
vidcap.set(4,720) #capture height
vidcap.set(cv2.CAP_PROP_FPS,30) #capture FPS

#vidcap.set(cv2.CAP_PROP_POS_MSEC,20000)      # just cue to 20 sec. position
for i in range(100):
   success,image = vidcap.read()
   print success
   if success:
      fps = vidcap.get(cv2.CAP_PROP_FPS)
      height,width,channels = image.shape
      print fps
      print width
      print height
      res = cv2.resize(image,(width/3, height/3), interpolation = cv2.INTER_AREA)
      cv2.imwrite("./test_video/frame_ " + str(i) + ".jpg", res)     # save frame as JPEG file
      #cv2.imshow("20sec",image)
      #cv2.waitKey()                    
      #cv2.destroyAllWindows()

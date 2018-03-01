import cv2
import sys

vidcap = cv2.VideoCapture('/home/jseng/www/robot.mpg')

print vidcap
#vidcap.set(cv2.CAP_PROP_POS_MSEC,20000)      # just cue to 20 sec. position
success,image = vidcap.read()
print success
if success:
   fps = vidcap.get(cv2.CAP_PROP_FPS)
   height,width,channels = image.shape
   print fps
   print width
   print height
   cv2.imwrite("./test_video/frame20sec.jpg", image)     # save frame as JPEG file
    #cv2.imshow("20sec",image)
    #cv2.waitKey()                    

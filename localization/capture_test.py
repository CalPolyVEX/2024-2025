import cv2
import sys, time, datetime

vidcap = cv2.VideoCapture(sys.argv[1])

print vidcap
vidcap.set(3,1920) #capture width
vidcap.set(4,1080) #capture height
vidcap.set(cv2.CAP_PROP_FPS,30) #capture FPS

#vidcap.set(cv2.CAP_PROP_POS_MSEC,20000)      # just cue to 20 sec. position
for i in range(3000):
   success,image = vidcap.read()
   #print success
   if success:
      fps = vidcap.get(cv2.CAP_PROP_FPS)
      height,width,channels = image.shape
      print fps
      print width
      print height
      start = time.time()
      #res = cv2.resize(image,(width/4, height/4), interpolation = cv2.INTER_AREA)
      #res = cv2.resize(image,(width/4, height/4))
      res = cv2.resize(image,(width, height), interpolation = cv2.INTER_AREA)
      end = time.time()
      print (end-start)
      cv2.imwrite("./test_video/frame_" + str(i) + ".jpg", res)     # save frame as JPEG file
      #cv2.imshow("20sec",image)
      #cv2.waitKey()                    
      #cv2.destroyAllWindows()
      time.sleep(1)

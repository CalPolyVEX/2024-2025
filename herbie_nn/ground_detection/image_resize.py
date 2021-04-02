import cv2
import numpy as np

img = cv2.imread('beach.jpg')
resized = cv2.resize(img, (640, 360), 0, 0, interpolation = cv2.INTER_NEAREST)
# resized = cv2.resize(img, (640, 360), 0, 0, interpolation = cv2.INTER_LINEAR)
#cv2.imwrite('test1.jpg', resized)

blank_image = np.zeros((360,640,3), np.uint8)

for h in range(3):
    for i in range(360):
        for j in range(640):
            blank_image[i][j][h] = img[i*3,j*3][h]

cv2.imwrite('test1.jpg', blank_image)

#file to test code for speeding up conversion from UYVY to BGR

import cv2
import numpy as np

def test_convert():
    imghd = cv2.imread('beach.jpg')
    imghd_i420 = cv2.cvtColor(imghd, cv2.COLOR_BGR2YUV_I420)
    resized = cv2.resize(imghd, (640, 360), 0, 0, interpolation = cv2.INTER_NEAREST)
    # resized = cv2.resize(img, (640, 360), 0, 0, interpolation = cv2.INTER_LINEAR)
    #cv2.imwrite('test1.jpg', resized)

    blank_image = np.zeros((360,640,3), np.uint8)

    for h in range(3):
        for i in range(360):
            for j in range(640):
                blank_image[i][j][h] = imghd[i*3,j*3][h]

    cv2.imwrite('test1.jpg', blank_image)

    print ('size of imghd: ' + str(imghd.shape))
    print ('size of imghd_i420: ' + str(imghd_i420.shape))
    print (imghd_i420[0])
    print (format(imghd_i420[0][0], '08b'))
    print (format(imghd_i420[0][1], '08b'))
    print (format(imghd_i420[0][2], '08b'))

def process_uyvy():
    #https://www.fourcc.org/pixel-format/yuv-uyvy/

    fh = open('./beach.uyvy', 'rb')
    ba = bytearray(fh.read())
    fh.close()
    print (ba[0])
    print (len(ba))

    counter = 0
    b = bytearray()
    for i in range(1080):
        for j in range(int(1920/2)):
            u = ba[counter]
            counter += 1

            y1 = ba[counter]
            counter += 1

            v = ba[counter]
            counter += 1

            y2 = ba[counter]
            counter += 1

            y1 -= 16;
            y2 -= 16;
            u -= 128;
            v -= 128;

            r1 = 1.164 * y1             + 1.596 * v;
            g1 = 1.164 * y1 - 0.392 * u - 0.813 * v;
            b1 = 1.164 * y1 + 2.017 * u;

            r2 = 1.164 * y2             + 1.596 * v;
            g2 = 1.164 * y2 - 0.392 * u - 0.813 * v;
            b2 = 1.164 * y2 + 2.017 * u;

            if b1 > 255:
                b1 = 255
            if g1 > 255:
                g1 = 255
            if r1 > 255:
                r1 = 255

            if b2 > 255:
                b2 = 255
            if g2 > 255:
                g2 = 255
            if r2 > 255:
                r2 = 255

            if b1 < 0:
                b1 = 0
            if g1 < 0:
                g1 = 0
            if r1 < 0:
                r1 = 0
            if b2 < 0:
                b2 = 0
            if g2 < 0:
                g2 = 0
            if r2 < 0:
                r2 = 0

            # print (r1,g1,b1)

            b.append(int(b1))
            b.append(int(g1))
            b.append(int(r1))
            b.append(int(b2))
            b.append(int(g2))
            b.append(int(r2))

    print (counter)
    print (len(b))


    blank_image = np.zeros((1080,1920,3), np.uint8)
    counter = 0

    for z in range(1080):
        for y in range(1920):
            for x in range(3):
                blank_image[z][y][x] = b[counter]
                counter += 1

    cv2.imwrite('test1.jpg', blank_image)

    # Y -= 16;
    # U -= 128;
    # V -= 128;
    # const auto r = 1.164 * Y             + 1.596 * V;
    # const auto g = 1.164 * Y - 0.392 * U - 0.813 * V;
    # const auto b = 1.164 * Y + 2.017 * U;

process_uyvy()


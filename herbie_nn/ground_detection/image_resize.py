#file to test code for speeding up conversion from UYVY to BGR

import cv2
import numpy as np
import time

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

def process_uyvy():
    #https://www.fourcc.org/pixel-format/yuv-uyvy/

    fh = open('./beach.uyvy', 'rb')
    ba = bytearray(fh.read())
    fh.close()
    print (ba[0])
    print (len(ba))

    counter = 0
    b = bytearray()
    row_size = 3840 #number byte to access next row
    for i in range(1080):
        for j in range(int(1920/2)):
            u = ba[counter]
            y1 = ba[counter+1]
            v = ba[counter+2]
            y2 = ba[counter+3]
            counter += 4

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

def process_uyvy2():
    #https://www.fourcc.org/pixel-format/yuv-uyvy/

    fh = open('./beach.uyvy', 'rb')
    ba = bytearray(fh.read())
    fh.close()
    print (ba[0])
    print (len(ba))

    counter = 0
    next_counter = 0
    ba_new=bytearray()
    row_size = 3840 #number of bytes per row
    start_ms = time.time()*1000.0
    for i in range(360):
        for j in range(320):
            for k in range(2): #alternating pixels within macropixel sets of 3
                u0 = ba[counter]
                u2 = ba[counter+4]

                v0 = ba[counter+2]
                v2 = ba[counter+6]

                if k == 0:
                    y0 = ba[counter+1]
                    y2 = ba[counter+5]
                else:
                    y0 = ba[counter+3]
                    y2 = ba[counter+7]

                #next row
                next_counter = counter + row_size*2
                u0 += ba[next_counter]
                u2 += ba[next_counter+4]

                v0 += ba[next_counter+2]
                v2 += ba[next_counter+6]

                if k == 0:
                    y0 += ba[next_counter+1]
                    y2 += ba[next_counter+5]
                else:
                    y0 += ba[next_counter+3]
                    y2 += ba[next_counter+7]

                if k == 0:
                    counter += 4
                else:
                    counter += 8

                #compute totals and average
                avg_u = (u0 + u2) / 4.0
                avg_v = (v0 + v2) / 4.0
                avg_y = (y0 + y2) / 4.0

                avg_y -= 16;
                avg_u -= 128;
                avg_v -= 128;

                r = 1.164 * avg_y                 + 1.596 * avg_v;
                g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
                b = 1.164 * avg_y + 2.017 * avg_u;

                if b > 255:
                    b = 255
                if g > 255:
                    g = 255
                if r > 255:
                    r = 255

                if b < 0:
                    b = 0
                if g < 0:
                    g = 0
                if r < 0:
                    r = 0

                # print (r1,g1,b1)

                ba_new.append(int(b))
                ba_new.append(int(g))
                ba_new.append(int(r))

        counter += row_size*2

    end_ms = time.time()*1000.0
    print ('time in ms:  ' + str(end_ms-start_ms))

    #print (counter)
    #print (len(ba_new))

    blank_image = np.zeros((360,640,3), np.uint8)
    counter = 0

    for z in range(360):
        for y in range(640):
            for x in range(3):
                blank_image[z][y][x] = ba_new[counter]
                counter += 1

    cv2.imwrite('test1.jpg', blank_image)

process_uyvy2()


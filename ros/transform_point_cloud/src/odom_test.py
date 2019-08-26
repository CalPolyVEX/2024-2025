#!/usr/bin/python

import math

old_x = 0
old_y = 0
old_theta = 0
new_x = 0
new_y = 0
new_theta = 0
base_width = .5
ticks_per_meter = 5000.0

def normalize_theta(angle):
    pi = 3.141592654
    while angle > pi:
        angle -= 2.0 * pi
    while angle < -pi:
        angle += 2.0 * pi

    return angle

def old_odom(left_diff_count, right_diff_count):
    global old_x, old_y, old_theta

    #distance each wheel traveled
    dist_left = left_diff_count / ticks_per_meter
    dist_right = right_diff_count / ticks_per_meter

    #distance the center point traveled
    dist = -(dist_right + dist_left) / 2.0
    d_theta = (dist_right - dist_left) / base_width
    r = dist / d_theta
    old_x += r * (math.sin(d_theta + old_theta) - math.sin(old_theta))
    old_y += r * (math.cos(d_theta + old_theta) - math.cos(old_theta))
    old_theta = normalize_theta(old_theta - d_theta)

def new_odom(left_diff_count, right_diff_count):
    global new_x, new_y, new_theta

    #distance each wheel traveled
    dist_left = left_diff_count / ticks_per_meter
    dist_right = right_diff_count / ticks_per_meter

    #distance the center point traveled
    dist = -(dist_right + dist_left) / 2.0

    new_x += dist * math.cos(new_theta + ((dist_right - dist_left) / (2*base_width)))
    new_y -= dist * math.sin(new_theta + ((dist_right - dist_left) / (2*base_width)))
    new_theta -= (dist_right - dist_left) / base_width
    new_theta = normalize_theta(new_theta)

def main():
    global old_x, old_y, old_theta
    global new_x, new_y, new_theta
    l = 0
    r = 1
    left_encoder = range(0,100,5)
    right_encoder = range(0,200,10)
    assert len(left_encoder) == len(right_encoder)

    for i in range(100):
        old_odom(100,200)
        new_odom(100,200)
        print ("%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f" % (l,r, old_x, new_x, old_y, new_y, old_theta, new_theta))
        l += 100
        r += 200

main()


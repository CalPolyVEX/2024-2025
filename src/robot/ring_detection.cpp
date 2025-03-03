#include "pros/distance.hpp"
#include "pros/optical.h"
#include "pros/optical.hpp"
#include "robot/ring_detection.hpp"

#define PROX_THRESH 130

Ring_Detector::Ring_Detector(int detector_port, int proximity_port):
    detector(detector_port),
    proximity(proximity_port) {}

bool Ring_Detector::has_ring() {
    return proximity.get_distance() > PROX_THRESH;
}

bool Ring_Detector::has_red_ring() {
    pros::c::optical_rgb_s_t rgb = detector.get_rgb();
    return has_ring() and rgb.red > rgb.blue and rgb.red > rgb.green;
}

bool Ring_Detector::has_blue_ring() {
    pros::c::optical_rgb_s_t rgb = detector.get_rgb();
    return has_ring() and rgb.blue > rgb.red and rgb.blue > rgb.green;
}
#ifndef GLOBALS_H
#define GLOBALS_H

#include "api.h"
#include "robot/fish_mech.hpp"
#include "robot/mogo_grabber.hpp"
#include "robot/doinker.hpp"
#include "robot/conveyor.hpp"
#include "robot/ring_detection.hpp"

// state variables
extern bool alliance_color; // true if red

// motors
extern pros::MotorGroup leftMG;
extern pros::MotorGroup rightMG;
//extern pros::Motor conveyor;
//extern pros::Motor intake;

// sensors
extern pros::Optical conveyor_color_detector;

// pneumatics
extern pros::adi::Pneumatics rejector;

// mechs
extern FishMech fish_mech;
extern MogoGrabber mogo_grabber;
extern Doinker doinker;
extern Conveyor conveyor;
extern Ring_Detector ring_detector;
#endif
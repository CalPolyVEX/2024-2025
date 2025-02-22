#ifndef GLOBALS_H
#define GLOBALS_H

#include "api.h"
#include "robot/fish_mech.h"
#include "robot/mogo_grabber.h"
#include "robot/doinker.h"

// state variables
extern bool alliance_color; // true if red

// motors
extern pros::MotorGroup leftMG;
extern pros::MotorGroup rightMG;
extern pros::Motor conveyor;
extern pros::Motor intake;

// sensors
extern pros::Optical conveyor_color_detector;

// pneumatics
extern pros::adi::Pneumatics rejector;

// mechs
extern FishMech fish_mech;
extern MogoGrabber mogo_grabber;
extern Doinker doinker;

#endif
#ifndef GLOBALS_H
#define GLOBALS_H

#include "api.h"

// state variables
extern bool alliance_color; // true if red

// motors
extern pros::MotorGroup leftMG;
extern pros::MotorGroup rightMG;
extern pros::Motor fish_mech;
extern pros::Motor conveyor;
extern pros::Motor intake;

// sensors
extern pros::Optical conveyor_color_detector;

// pneumatics
extern pros::adi::Pneumatics mogo_grabber;
extern pros::adi::Pneumatics rejector;
extern pros::adi::Pneumatics doinker;

#endif
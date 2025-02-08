#ifndef HARDWARE_H
#define HARDWARE_H

#include "api.h"

// TODO move this somewhere else to avoid setting global state in the main initialize function.
extern bool alliance_color;

// ----------------------------- drivetrain -----------------------------
extern pros::MotorGroup leftMG;
extern pros::MotorGroup rightMG;

// ----------------------------- motors -----------------------------
extern pros::Motor fish_mech;
extern pros::Motor conveyor;
extern pros::Motor intake;

//----------------------------- sensors -----------------------------
extern pros::Optical conveyor_color_detector;

//----------------------------- pneumatics -----------------------------
extern pros::adi::Pneumatics mogo_grabber;
extern pros::adi::Pneumatics rejector;
extern pros::adi::Pneumatics doinker;

#endif
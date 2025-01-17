#pragma once

#include "lemlib/pose.hpp"
#include "config.h"
#include "api.h"
// this abstracts all the devices that are used by the v5 brain so that we can change ports and parameters quickly and
// debug easily.

inline bool alliance_color = true; // true is red, false is blue

//----------------------------- motors & motor groups -----------------------------

// note: there's no difference in MotorGearset and MotorCartridge

/*
1800 ticks/rev with 36:1 gears, 600 rpm //blue
900 ticks/rev with 18:1 gears, 200 rpm //green
300 ticks/rev with 6:1 gears, 100 rpm //red
*/
#ifdef PROTOTYPE_BOT
// drivetrain
inline pros::MotorGroup leftMG({15, -9, -18}, pros::MotorGearset::blue);
inline pros::MotorGroup rightMG({-19, 4, 5}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
inline pros::Motor fish_mech(-3, pros::MotorCartridge::red);

inline pros::Motor conveyor(-2, pros::MotorGearset::blue);
inline pros::Motor roller_intake(-6, pros::MotorCartridge::blue);

//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

inline pros::Optical conveyor_color_detector(13);

int musty_port = 12;
//===================================================================

//----------------------------- pneumatics -----------------------------
inline pros::adi::Pneumatics mogo_grabber('C', false);

inline pros::adi::Pneumatics rejector('B', false);

inline pros::adi::Pneumatics doinker('D', false);
//===================================================================

inline lemlib::Pose start_red_pose = lemlib::Pose(0, 0, 0);
inline lemlib::Pose start_blue_pose = lemlib::Pose(0, 0, 180);

#endif

#ifdef GOLD_BOT
// drivetrain
inline pros::MotorGroup leftMG({-6, 7, -8}, pros::MotorGearset::blue);
inline pros::MotorGroup rightMG({1, -2, 3}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
inline pros::Motor fish_mech(9, pros::MotorCartridge::red);

inline pros::Motor conveyor(-4, pros::MotorGearset::blue);
inline pros::Motor roller_intake(-5, pros::MotorCartridge::blue);

//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

inline pros::Optical conveyor_color_detector(21);

inline int musty_port = 12;
//===================================================================

//----------------------------- pneumatics -----------------------------
inline pros::adi::Pneumatics mogo_grabber('A', false);

inline pros::adi::Pneumatics rejector('C', false);

inline pros::adi::Pneumatics doinker('B', false);
//===================================================================

inline lemlib::Pose start_red_pose = lemlib::Pose(0, 0, 0);
inline lemlib::Pose start_blue_pose = lemlib::Pose(0, 0, 180);

#endif

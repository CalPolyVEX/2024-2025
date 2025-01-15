#include "main.h"
#include "config.h"
#include "lemlib/api.hpp"
#include "pros/adi.hpp"
// this abstracts all the devices that are used by the v5 brain so that we can change ports and parameters quickly and
// debug easily.

bool alliance_color = true; // true is red, false is blue

//----------------------------- motors & motor groups -----------------------------

// note: there's no difference in MotorGearset and MotorCartridge

/*
1800 ticks/rev with 36:1 gears, 600 rpm //blue
900 ticks/rev with 18:1 gears, 200 rpm //green
300 ticks/rev with 6:1 gears, 100 rpm //red
*/
#ifdef PROTOTYPE_BOT
// drivetrain
pros::MotorGroup leftMG({15, -9, -18}, pros::MotorGearset::blue);
pros::MotorGroup rightMG({-19, 4, 5}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
pros::Motor fish_mech(-3, pros::MotorCartridge::red);

pros::Motor conveyor(-2, pros::MotorGearset::blue);
pros::Motor roller_intake(-6, pros::MotorCartridge::blue);

//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(13);
//===================================================================

//----------------------------- pneumatics -----------------------------
pros::adi::Pneumatics mogo_grabber('C', false);

pros::adi::Pneumatics rejector('B', false);

pros::adi::Pneumatics doinker('D', false);
//===================================================================
#endif

#ifdef GOLD_BOT
// drivetrain
pros::MotorGroup leftMG({-6, 7, -8}, pros::MotorGearset::blue);
pros::MotorGroup rightMG({1, -2, 3}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
pros::Motor fish_mech(9, pros::MotorCartridge::red);

pros::Motor conveyor(-4, pros::MotorGearset::blue);
pros::Motor roller_intake(-5, pros::MotorCartridge::blue);

//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(21);
//===================================================================

//----------------------------- pneumatics -----------------------------
pros::adi::Pneumatics mogo_grabber('A', false);

pros::adi::Pneumatics rejector('C', false);

pros::adi::Pneumatics doinker('B', false);
//===================================================================
#endif

#ifdef GREEN_BOT
// drivetrain
pros::MotorGroup leftMG({1, -2, 3}, pros::MotorGearset::blue);
pros::MotorGroup rightMG({-6, 7, -8}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
pros::Motor fish_mech(-9, pros::MotorCartridge::red);

pros::Motor conveyor(-4, pros::MotorGearset::blue);
pros::Motor roller_intake(5, pros::MotorCartridge::blue);

//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(21);
//===================================================================

//----------------------------- pneumatics -----------------------------
pros::adi::Pneumatics mogo_grabber('D', false);

pros::adi::Pneumatics rejector('C', false);

pros::adi::Pneumatics doinker('B', false);
//===================================================================

#endif
#include "main.h"
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

// drivetrain
pros::MotorGroup leftMG({15, -9, -18}, pros::MotorGearset::blue);
pros::MotorGroup rightMG({-19, 4, 5}, pros::MotorGearset::blue);
//----------

// TODO rename this when the other mechanism is on the bot
pros::Motor fish_mech(7, pros::MotorCartridge::red);

pros::Motor conveyor(2, pros::MotorGearset::blue);
pros::Motor roller_intake(-6, pros::MotorCartridge::blue);


//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(3);
//===================================================================

//----------------------------- pneumatics -----------------------------
pros::adi::Pneumatics mogoGrabber('A', false);

pros::adi::Pneumatics rejector('B', false);

pros::adi::Pneumatics doinker('H', false);
//===================================================================
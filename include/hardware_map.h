#include "main.h"
#include "lemlib/api.hpp"
// this abstracts all the devices that are used by the v5 brain so that we can change ports and parameters quickly and
// debug easily.

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



// drivetrain, chassis and PID controllers definitions===================================
lemlib::ControllerSettings lateralPIDController(10, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              3, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              20 // maximum acceleration (slew)
);
lemlib::ControllerSettings angularPIDController(2, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              10, // derivative gain (kD)
                                              3, // anti windup
                                              1, // small error range, in inches
                                              100, // small error range timeout, in milliseconds
                                              3, // large error range, in inches
                                              500, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);

lemlib::Drivetrain drivetrain(&leftMG, &rightMG, 12.25, lemlib::Omniwheel::NEW_325, 450, 0);

lemlib::Chassis chassis(drivetrain, lateralPIDController, angularPIDController);
//===================================================================

//----------------------------- sensors -----------------------------
// pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(3);
//===================================================================

//----------------------------- pneumatics -----------------------------
pros::adi::DigitalOut mogoGrabber('A', false);

pros::adi::DigitalOut rejector('B', false);

pros::adi::DigitalOut doinker('H', false);
//===================================================================
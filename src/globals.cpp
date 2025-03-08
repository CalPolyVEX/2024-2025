#include "globals.h"
#include "ports.h"
#include "api.h"
// #include "robot/fish_mech.hpp"
// #include "robot/mogo_grabber.hpp"
// #include "robot/doinker.hpp"

bool alliance_color = true;

// note: there's no difference in MotorGearset and MotorCartridge

/*
1800 ticks/rev with 36:1 gears, 600 rpm //blue
900 ticks/rev with 18:1 gears, 200 rpm //green
300 ticks/rev with 6:1 gears, 100 rpm //red
*/

// motors
pros::MotorGroup leftMG({PORT_LEFT_MOTOR_1, PORT_LEFT_MOTOR_2, PORT_LEFT_MOTOR_3}, pros::MotorGearset::blue);
pros::MotorGroup rightMG({PORT_RIGHT_MOTOR_1, PORT_RIGHT_MOTOR_2, PORT_RIGHT_MOTOR_3}, pros::MotorGearset::blue);

//pros::Motor intake(PORT_INTAKE_MOTOR, pros::MotorCartridge::blue);

// sensors
pros::Optical conveyor_color_detector(PORT_CONVEYOR_COLOR_SENSOR);

// pneumatics
pros::adi::Pneumatics rejector(PORT_REJECTOR, false);

// mechs
FishMech fish_mech(PORT_FISH_MOTOR);
MogoGrabber mogo_grabber(PORT_MOGO_GRABBER);
Doinker doinker(PORT_DOINKER);
Conveyor conveyor(PORT_CONVEYOR_MOTOR, PORT_INTAKE_MOTOR);
Ring_Detector ring_detector(PORT_CONVEYOR_COLOR_SENSOR, PORT_DISTANCE_SENSOR);
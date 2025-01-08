#include "main.h"

//this abstracts all the devices that are used by the v5 brain so that we can change ports and parameters quickly and debug easily.



//----------------------------- motors & motor groups -----------------------------

//note: there's no difference in MotorGearset and MotorCartridge

/*
1800 ticks/rev with 36:1 gears, 600 rpm //blue
900 ticks/rev with 18:1 gears, 200 rpm //green
300 ticks/rev with 6:1 gears, 100 rpm //red
*/



//drivetrain
pros::MotorGroup rightMG({-8, 14, 17}, pros::MotorGearset::blue);
pros::MotorGroup leftMG({7, -4, -18}, pros::MotorGearset::blue);
//----------

//TODO rename this when the other mechanism is on the bot
pros::Motor fish_mech(3, pros::MotorCartridge::red); 


pros::Motor conveyor(12, pros::MotorGearset::blue);
pros::Motor roller_intake(15, pros::MotorCartridge::blue);
//===================================================================





//----------------------------- sensors -----------------------------
//pros::Distance ring_dist(6);

pros::Optical conveyor_color_detector(5);
//===================================================================






//----------------------------- pneumatics -----------------------------
pros::adi::DigitalOut mogoGrabber('A', false);

pros::adi::DigitalOut rejector('B', false);

pros::adi::DigitalOut doinker('H', false);
//===================================================================
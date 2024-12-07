#include "main.h"
#include "display.h"
//#include "pros/llemu.hpp"

#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/odom.hpp" // IWYU pragma: keep

//#include "lemlib/chassis/chassis.cpp"

pros::MotorGroup rightMG({-8, 14, 17}, pros::MotorGearset::blue);
pros::MotorGroup leftMG({7, -4, -18}, pros::MotorGearset::blue);

pros::Optical colorSensor(9);

//pros::adi::DigitalOut mogoGrabber('A', false);

//pros::Motor wallStakeArm(3, pros::MotorCartridge::red);

//note: there's no difference in MotorGearset and MotorCartridge
//pros::Motor conveyor(12, pros::MotorGearset::blue);
//pros::Motor roller(15, pros::MotorCartridge::blue);

//pros::MotorGroup conveyor({3, 4}, pros::MotorGearset::blue);

lemlib::ControllerSettings lateralPIDController(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
lemlib::ControllerSettings angularPIDController(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

lemlib::Drivetrain drivetrain(&leftMG, &rightMG, 12.25, lemlib::Omniwheel::NEW_325, 450, 0);

lemlib::Chassis chassis(drivetrain, lateralPIDController, angularPIDController);

pros::Controller controller(CONTROLLER_MASTER);


#define IS_DEBUGGING_OTOS false

//int loop_counter = 0;
//int last_time = 0;



/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
    //pros::lcd::initialize(); // initialize brain screen

    lemlib::init(); // initialize lemlib
    
    //pros::screen::draw_line(0, 0, 200, 200);
    initializeScreen();

    colorSensor.disable_gesture();
    pros::c::optical_rgb_s_t color = colorSensor.get_rgb();

    if (color.red > color.blue){
        lemlib::calibrate_otos(true);
    } else {
        lemlib::calibrate_otos(false);
    }
    
    
    //pros::Controller controller (pros::E_CONTROLLER_MASTER);

    while (IS_DEBUGGING_OTOS)
    {
        //pros::lcd::print(1, "Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

        //pros::screen::draw_line(0, 0, 200, 200);

        //pros::screen::draw_circle(lemlib::getPose().x + 72, lemlib::getPose().y + 72, 4);

        //controller.print(2, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
        //if (IS_DEBUGGING_OTOS){
        printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    
        pros::delay(10);
    }
    
        
        
        //pros::lcd::print(0, "Pose: (%f, %f, %f)", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    
    //pros::lcd::print(0, "Pose: (%f, %f, %f)", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
}

/**
 * Runs while the robot is disabled
 */
void disabled() {}

/**
 * runs after initialize if the robot is connected to field control
 */
void competition_initialize() {}

/**
 * Runs during auto
 *
 * 
 * */
void autonomous() {
    
}

/**
 * Runs in driver control
 */
void opcontrol() {
    int scale = 4;
    //chassis.follow()
    while (1) {
        updateRobotPositionOnScreen(lemlib::getPose());
        /*
        pros::screen::erase();
        //printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
        pros::screen::set_pen(pros::Color::red);
        pros::screen::draw_circle((240 + lemlib::getPose().x * scale) + 18*cos(lemlib::degToRad(lemlib::getPose().theta)) , (136 + lemlib::getPose().y * scale) + 18*sin(lemlib::degToRad(lemlib::getPose().theta)), 2);
        pros::screen::set_pen(pros::Color::blue);
        pros::screen::draw_circle((240 + lemlib::getPose().x * scale) , (136 + lemlib::getPose().y * scale) , 12);

        //pros::lcd::print(1, "Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
        */
        chassis.arcade(
            controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y), 
            controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X)
        );
        controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
        pros::delay(10);
       //controller.print(2, 0, "Pose:(%1.1f, %1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    }
}

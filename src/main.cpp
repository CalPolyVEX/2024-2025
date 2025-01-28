#include "main.h"
#include "display.h"
// #include "config.h"
//  #include "pros/llemu.hpp"
//  #ifndef HARDWARE_MAP_H
#include "auto_state_machine.cpp"

#include "fmt/format.h"
#include "hardware_map.h"
// #endif
#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/chassis.hpp"
#include "lemlib/chassis/odom.hpp" // IWYU pragma: keep
#include "conveyor_ctrls.hpp"
#include "pros/misc.h"
#include "pros/misc.hpp"
#include "pros/rtos.hpp"
#include "globals.h"
#include <cstdlib>
ASSET(pathTest_txt);

// this is .0408 inches accuracy
#define CONVEYOR_TARGET_THRESH 30 // (in ticks)

//======================================================================================

pros::Controller controller(CONTROLLER_MASTER);

pros::Task* fish_mech_task = nullptr;

pros::Task* telemetry_task = nullptr;


#define IS_DEBUGGING_OTOS false

// drivetrain, chassis and PID controllers definitions===================================
lemlib::ControllerSettings lateralPIDController(9, // proportional gain (kP)
                                                0.5, // integral gain (kI)
                                                0.9, // derivative gain (kD)
                                                3, // anti windup
                                                0.25, // small error range, in inches
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in inches
                                                500, // large error range timeout, in milliseconds
                                                16 // maximum acceleration (slew)
);
lemlib::ControllerSettings angularPIDController(2, // pr7oportional gain (kP)
                                                0, // integral gain (kI)
                                                10, // derivative gain (kD)
                                                3, // anti windup
                                                0.5, // small error range, in inches
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in inches
                                                500, // large error range timeout, in milliseconds
                                                0 // maximum acceleration (slew)
);

lemlib::Drivetrain drivetrain(&leftMG, &rightMG, 12.25, lemlib::Omniwheel::NEW_325, 450, 0);

lemlib::Chassis chassis(drivetrain, lateralPIDController, angularPIDController);

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
  initialize_screen();

  chassis.setBrakeMode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_BRAKE);
  conveyor_color_detector.set_led_pwm(100);

  lemlib::init(); // initialize lemlib

  fish_mech.set_brake_mode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_HOLD);
  fish_mech.set_encoder_units(pros::MotorEncoderUnits::degrees);

  // the conveyor is already tensioned and frictioned.
  // it seems to stop abruptly as hard braking anyway
  conveyor.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

  conveyor.set_encoder_units(pros::motor_encoder_units_e_t::E_MOTOR_ENCODER_COUNTS);

  // this coast behavior makes sense though, the roller doesnt need to brake hard.
  intake.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

  conveyor_color_detector.disable_gesture();
  pros::c::optical_rgb_s_t color = conveyor_color_detector.get_rgb();
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  if (has_red_ring()) {
    // lemlib::calibrate_otos(true);
    // lemlib::calibrate_otos(true);
    alliance_color = true;
    print_text_at(5, "color sensor sees red");
  } else if (has_blue_ring()) {
    alliance_color = false;
    // lemlib::calibrate_otos(false);
    // lemlib::calibrate_otos(false);
    print_text_at(5, "color sensor sees blue");
  } else {
    print_text_at(5, "color sensor sees nothing");
  }

  lemlib::calibrate_otos(alliance_color);

  //}

  // lemlib::calibrate_otos(true);
  pros::delay(600); // dont do anything for half a sec so we can init the otos

  print_text_at(8, "done calibrating");
  /*
  telemetry_task = new pros::Task {[=] {
                                     while (true) {
                                       update_robot_position_on_screen(lemlib::getPose(true));
                                       pros::delay(100);
                                     }
                                   },
                                   "telemetry task"};
  */

  while (IS_DEBUGGING_OTOS) {
    update_robot_position_on_screen(lemlib::getPose(true));
    // printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

    pros::delay(10);
  }
  conveyor_color_detector.set_led_pwm(0);
}

/**
 * Runs while the robot is disabled
 */
void disabled() {}

/**
 * runs after initialize if the robot is connected to field control
 */
void competition_initialize() {
  // TODO do otos offset with lidar here??????
}

/**
 * Runs during auto
 *
 *
 * */
void autonomous() {
  print_text_at(7, "am moving rn");
  // chassis.turnToHeading(0, 3000);
  // set_conveyor_target_in_inches(5, 600);
  // while (true) { pros::delay(100); }

  lemlib::MoveToPointParams params = {.forwards = false};
  // chassis.moveToPoint(-8, 0, 2000, params);

  // chassis.moveToPose(-8, 0, 90, 1200);
  // chassis.turnToHeading(0, 500);
  // lemlib::MoveToPointParams params = {.forwards = false};
  // chassis.turnToHeading(0, 500);
  // lemlib::MoveToPointParams params = {.forwards = false};
  // chassis.follow(pathTest_txt, 3, 3000);

  // chassis.moveToPoint(-8, 0, 2000, params);
  // chassis.moveToPose(-8, 0, 90, 1200);
  // chassis.moveToPoint(-8, 0, 2000, params);
  // chassis.moveToPose(-8, 0, 90, 1200);

  chassis.waitUntilDone();

  print_text_at(7, "done moving");
}

void deadband(int& val, int deadband) {
  if (std::abs(val) < deadband) { val = 0; }
}


bool conveyor_is_enabled = false;


void op_init(){
  

  fish_mech_task = new pros::Task {[=] {
    bool fish_mech_override_flag = true;

    while (true){
      
      #ifdef GOLD_BOT

      int fish_axis = controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_UP) ? 90 : 0;
      fish_axis -= controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_DOWN) ? 90 : 0;

      #endif


      #ifdef GREEN_BOT  
      // FISH MECH
      int fish_axis = controller.get_analog(FISH_MANUAL_AXIS);
      deadband(fish_axis, 30);

      #endif


      #ifdef PROTOTYPE_BOT  
      // FISH MECH
      int fish_axis = controller.get_analog(FISH_MANUAL_AXIS);
      deadband(fish_axis, 30);

      #endif

      fish_mech_override_flag = fish_axis != 0;

      //print_text_at(10, fmt::format("fish mech pos = {}", fish_mech.get_position()).c_str());

      

      //print_text_at(9, fmt::format("fish mech flag is {}", fish_mech_override_flag).c_str());

      //print_text_at(8, fmt::format("fish velo = {}", fish_mech.get_actual_velocity()).c_str());

      if (controller.get_digital_new_press(FISH_SCORE_BUTTON) and not fish_mech_override_flag) { 
        //if we arent inputting fish manual controls and we press (Y)
        
        score_with_fish_mech(); // score
        while (std::abs(fish_mech.get_position() - fish_mech.get_target_position()) >= 4 ){  // if we have not met/exceeded target
          pros::delay(200); //wait
          //print_text_at(5, fmt::format("misllis = {}", pros::millis()).c_str());
        }

        // if we are within/past the target then
          
        // end fishing.  wait to hold at pos 
        pros::delay(FISH_DELAY);
      }

      if (fish_mech_override_flag){
        fish_mech.move_velocity(fish_axis);

      } else {
        if (fish_mech.get_position() > 0 and fish_mech.get_position() < 161){
          fish_mech.move_velocity(-600);
        } else {
          if (fish_mech.get_actual_velocity() == 0){
            fish_mech.move_velocity(0);
          }else {
            fish_mech.move_velocity(-1);

          }
          //fish_mech.move_absolute(0, 600);
          
        }
      }
      //print_text_at(11, fmt::format("fish axis val = {}", fish_axis).c_str());


      

      //print_text_at(7, fmt::format("is_fishing = {}", is_fishing).c_str());

      
      
      //print_text_at(5, fmt::to_string(pros::millis()).c_str());
    }
  }, "fish mech task"};
    
}

/**
 * Runs in driver control
 */

void opcontrol() {
  op_init();

  bool is_loading = false;

  while (1) {
    update_robot_position_on_screen(lemlib::getPose(true));

    if (controller.get_digital_new_press(CONVEYOR_ENABLE)) {
      conveyor_is_enabled = !conveyor_is_enabled;
    } else if (controller.get_digital(CONVEYOR_REVERSE)){
      conveyor_deposit_and_intake(-600);
      conveyor_is_enabled = false;
      
    } else if (!conveyor_is_enabled){
      conveyor.move_velocity(0);
      intake.move_velocity(100);
    }
    if (controller.get_digital_new_press(LOAD_NEXT_RING)){
      is_loading = !is_loading;
      if (is_loading){
        controller.rumble("....");
      } else {
        controller.rumble("..");
      }  
      
    }


    if (is_loading){
      conveyor_color_detector.set_led_pwm(100);
      if (not fish_mech_is_loaded()){
        conveyor_deposit_and_intake();

      } else {
        is_loading = false;
        //set_conveyor_target_in_inches(float inches)
        controller.rumble(".-");
      }
    } else if (conveyor_is_enabled){
      conveyor_color_detector.set_led_pwm(100);

      conveyor_deposit_and_intake();
    } else {
      conveyor_color_detector.set_led_pwm(0);
    }

    

    if (controller.get_digital_new_press(SCORING_OPPOSITE_BUTTON)) {
      scoring_opposite = !scoring_opposite;
    }

    if (scoring_opposite){
      controller.print(2, 0, "chucking color");
    } else {
      controller.clear_line(2);
    }

    #ifndef MOGO_DROP // JOSEPH
      if (controller.get_digital_new_press(MOGO_GRAB)){ 
        mogo_grabber.toggle(); 
        if (mogo_grabber.is_extended()){
          conveyor.move_velocity(-600);
          pros::delay(100);
        }
      }
    #else // TIM
      if (controller.get_digital_new_press(MOGO_GRAB)){
        mogo_grabber.extend();
        conveyor.move_velocity(-600);
        pros::delay(100);
      } else if (controller.get_digital_new_press(MOGO_DROP)) {
        mogo_grabber.retract();
      }
    #endif

    

    if (controller.get_digital_new_press(DOINKER_BUTTON)) { doinker.toggle(); } // doinker is a newpress toggle

    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   -controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    pros::delay(50);
  }
}

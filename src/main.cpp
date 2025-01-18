#include "main.h"
#include "display.h"
#include "config.h"
// #include "pros/llemu.hpp"
// #ifndef HARDWARE_MAP_H
#include "auto_state_machine.cpp"
#include "fmt/core.h"
#include "hardware_map.h"
// #endif
#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/chassis.hpp"
#include "lemlib/chassis/odom.hpp" // IWYU pragma: keep
#include "conveyor_ctrls.hpp"
#include "button_helper_class.h"
#include "pros/misc.h"
#include "pros/motors.h"
#include "pros/rtos.hpp"

ASSET(pathTest_txt);

// this is .0408 inches accuracy
#define CONVEYOR_TARGET_THRESH 30 // (in ticks)

//======================================================================================

pros::Controller controller(CONTROLLER_MASTER);

pros::Task* intake_task = nullptr;
pros::Task* button_update_task = nullptr;
pros::Task* telemetry_task = nullptr;
pros::Task* mogo_grabber_task = nullptr;
pros::Task* fish_mech_task = nullptr;

#define IS_DEBUGGING_OTOS false

button_expanded fish_mech_loading_conveyor_button;
button_expanded conveyor_enabled_button;
button_expanded mogo_grabber_button;
button_expanded conveyor_reverse_button;
button_expanded fish_score_button;
button_expanded color_rejection_enable;
button_expanded doinker_button;

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
  // TODO initialize the otos using lemlib::setPose() and the color sensor
  initialize_screen();
  chassis.setBrakeMode(pros::E_MOTOR_BRAKE_BRAKE);

  fish_mech.set_brake_mode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_HOLD);
  fish_mech.set_encoder_units(pros::MotorEncoderUnits::degrees);

  // TODO: see if we can use hard braking instead of coasting,
  // it should be more accurate and make little-to-no-diff because
  // the conveyor is already tensioned and frictioned.
  // it seems to stop abruptly as hard braking anyway
  conveyor.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

  conveyor.set_encoder_units(pros::motor_encoder_units_e_t::E_MOTOR_ENCODER_COUNTS);

  // this coast behavior makes sense though, the roller doesnt need to brake hard.
  roller_intake.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

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

  lemlib::calibrate_otos(true);

  //}

  // lemlib::calibrate_otos(true);

  print_text_at(8, "done calibrating");
  telemetry_task = new pros::Task {[=] {
                                     while (true) {
                                       update_robot_position_on_screen(lemlib::getPose(true));
                                       pros::delay(100);
                                     }
                                   },
                                   "telemetry task"};

  while (IS_DEBUGGING_OTOS) {
    update_robot_position_on_screen(lemlib::getPose(true));
    // printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

    pros::delay(10);
  }

  lemlib::init(); // initialize lemlib
  pros::delay(600); // dont do anything for half a sec so we can init the otos
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
  lemlib::MoveToPointParams params = {.forwards = false};
  lemlib::MoveToPointParams speedParams = {.maxSpeed=127};

  // Drive straight, grab mogo, and back up
  // Note: starting angled toward mogo, initial heading is still set to 90 internally
  // doinker.extend();
  // chassis.moveToPoint(45, 0, 8000, speedParams); //13.5 extra for 50, reading 51.33, -5 for 50, reading 49.37, 
  // chassis.waitUntilDone();
  // chassis.moveToPoint(20, 0, 2000, params);
  // doinker.retract();

  // Test path without LemLib
  // chassis.moveToPose(-8, 0, 90, 1200);
  // chassis.turnToHeading(45, 2000);
  // chassis.waitUntilDone();

  // chassis.moveToPoint(20, 0, 2000, params);
  // chassis.waitUntilDone();

  // chassis.turnToHeading(90, 2000);
  // chassis.waitUntilDone();

  chassis.follow(pathTest_txt, 7, 2000);
  chassis.waitUntilDone();
  // print_text_at(9, pros::Task::current().get_name());
}

void update_buttons() {
  // put all button_expanded updates here
  fish_mech_loading_conveyor_button.update(
      controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_A));
  conveyor_enabled_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_L1));
  mogo_grabber_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_R1));
  conveyor_reverse_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_L2));
  fish_score_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_Y));
  color_rejection_enable.update(controller.get_digital(pros::E_CONTROLLER_DIGITAL_LEFT));
  doinker_button.update(controller.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT));
}

void deadband(int& val, int deadband) {
  if (std::abs(val) < deadband) { val = 0; }
}

bool fish_mech_override_flag = true;

// initializes opmode tasks and mechanisms for opmode(), before main loop runs.
void op_init() {
  mogo_grabber_task =
      new pros::Task {[=] {
                        while (true) {
#ifdef PROTOTYPE_BOT
                          if (mogo_grabber_button.is_toggled()) {
                            mogo_grabber.extend();
                          } else {
                            mogo_grabber.retract();
                            if (mogo_grabber_button.just_pressed()) { conveyor.move_relative(-900, 600); }
                          }
#endif

#ifdef GOLD_BOT
                          if (controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_R1)) {
                            mogo_grabber.retract();
                          } else if (controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_R2)) {
                            mogo_grabber.extend();
                          }
#endif
                          pros::delay(20);
                        }
                      },
                      "mogo grabber task"};

  button_update_task = new pros::Task {[=] {
                                         while (true) {
                                           update_buttons();
                                           pros::delay(10);
                                         }
                                       },
                                       "button update task"};

  intake_task = new pros::Task {
      [=] {
        while (true) {
          // print_text_at(5, "intaking");
          if (fish_mech_loading_conveyor_button.is_toggled() and conveyor_enabled_button.is_toggled()) {
            if (fish_mech_loading_conveyor_button.just_pressed()) {
              print_text_at(9, "fishin");
              // if button has just been pressed (not held) (and we have toggled the
              // button):
              controller.rumble("....");

              while (!fish_mech_is_loaded() and fish_mech_loading_conveyor_button.is_toggled() and
                     conveyor_enabled_button.is_toggled()) {
                pros::delay(20);
              }
              // sets target for the conveyor to reach
              // the conveyor moves about 4 in for the fish mech to load the ring.
              set_conveyor_target_in_inches(.75);
            }
            if (std::abs(conveyor.get_position() - conveyor.get_target_position()) <= CONVEYOR_TARGET_THRESH) {
              conveyor.brake();
            }
            // move the conveyor to the fish mech position
          } else {
            print_text_at(9, "");
            if (conveyor_enabled_button.is_toggled() and not conveyor_reverse_button.is_pressed()) {
              // conveyor_reverse_button.update(false);
              conveyor_deposit_and_intake();
            } else if (conveyor_reverse_button.is_pressed()) {
              conveyor_enabled_button.toggled = false;

              conveyor.move_velocity(-600);
              roller_intake.move_velocity(-600);
            } else {
              conveyor.move_velocity(0);
              roller_intake.move_velocity(120);
            }
          }
          pros::delay(60);
        }
      },
      "intake task"};

  /*
    fish_mech_task = new pros::Task {
        [=] {
          while (true) {
            print_text_at(7, fmt::format("fish_mech_pos = {}", fish_mech.get_position()).c_str());
            print_text_at(6, fmt::format("fish_mech_target = {}", fish_mech.get_target_position()).c_str());
            print_text_at(9, fmt::format("fish override = {}", fish_mech_override_flag).c_str());
            print_text_at(10, fmt::format("fish current milliamps {}", fish_mech.get_current_draw()).c_str());
  #ifdef GREEN_BOT
            int fish_axis = controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_Y);
            deadband(fish_axis, 38); // 30% deadband of 127
  #endif
  #ifdef GOLD_BOT
            int fish_axis = controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_UP) ? 127 : 0;
            fish_axis -= controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_DOWN) ? 127 : 0;
  #endif
            print_text_at(11, fmt::format("fish axis val = {}", fish_axis).c_str());
            if (fish_axis > 0 or fish_axis < 0) {
              fish_mech_override_flag = true;
              fish_mech.move_velocity(fish_axis);
            } else if (fish_mech.get_position() < 160) {
              if (fish_mech.get_position() != 0) { zero_fish_mech(); }
            } else {
              fish_mech.move_velocity(0);
            }

            if (fish_score_button.just_pressed()) { fish_mech_override_flag = false; }

            if (not fish_mech_override_flag) {
              score_with_fish_mech();

              if ((std::abs(fish_mech.get_position() - fish_mech.get_target_position()) > 3) and
                  not fish_mech_override_flag) {
                // thresh of 3 degrees
                pros::delay(20);
              }

              pros::delay(1000);
              fish_mech.move_absolute(0, -600);
            }
            pros::delay(20);
          }
        },
        "fish mech task"};
  */
}

/**
 * Runs in driver control
 */
void opcontrol() {
  op_init();
  
  chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);

  while (1) {
    update_robot_position_on_screen(lemlib::getPose(true));

    if (not color_rejection_enable.is_toggled()) {
      scoring_opposite = false;
      controller.clear();
    } else {
      controller.print(0, 0, "chucking color");
      scoring_opposite = true;
    }

    if (doinker_button.is_toggled()) {
      doinker.extend();
    } else {
      doinker.retract();
    }

    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    pros::delay(50);
  }
}

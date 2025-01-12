#include "main.h"
#include "display.h"
// #include "pros/llemu.hpp"
// #ifndef HARDWARE_MAP_H
#include "hardware_map.h"
// #endif
#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/chassis.hpp"
#include "lemlib/chassis/odom.hpp" // IWYU pragma: keep
#include "conveyor_ctrls.hpp"
#include "button_helper_class.h"

ASSET(pathTest_txt);

// this is .0408 inches accuracy
#define CONVEYOR_TARGET_THRESH 30 // (in ticks)

//======================================================================================

pros::Controller controller(CONTROLLER_MASTER);

pros::Task* intake_task = nullptr;
pros::Task* button_update_task = nullptr;
pros::Task* telemetry_in_auto_task = nullptr;
pros::Task* mogo_grabber_task = nullptr;

#define IS_DEBUGGING_OTOS false

button_expanded fish_mech_loading_conveyor_button;
button_expanded conveyor_enabled_button;
button_expanded mogo_grabber_button;
button_expanded conveyor_reverse_button;

// drivetrain, chassis and PID controllers definitions===================================
lemlib::ControllerSettings lateralPIDController(9, // proportional gain (kP)
                                                .1, // integral gain (kI)
                                                3, // derivative gain (kD)
                                                3, // anti windup
                                                0.25, // small error range, in inches
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in inches
                                                750, // large error range timeout, in milliseconds
                                                16 // maximum acceleration (slew)
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

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
  // TODO initialize the otos using lemlib::setPose() and the color sensor
  initialize_screen();

  lemlib::init(); // initialize lemlib

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
  if (has_red_ring()) {
    // lemlib::calibrate_otos(true);
    alliance_color = true;
    print_text_at(5, "color sensor sees red");
  } else if (has_blue_ring()) {
    alliance_color = false;
    // lemlib::calibrate_otos(false);
    print_text_at(5, "color sensor sees blue");
  } else {
    print_text_at(5, "color sensor sees nothing");
  }

  lemlib::calibrate_otos(true);
  pros::delay(100);
  //}

  // lemlib::calibrate_otos(true);
  pros::delay(500); // dont do anything for half a sec so we can init the otos

  print_text_at(8, "done calibrating");

  while (IS_DEBUGGING_OTOS) {
    update_robot_position_on_screen(lemlib::getPose(true));
    // printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

    pros::delay(10);
  }
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
  telemetry_in_auto_task = new pros::Task {[=] {
                                             while (true) {
                                               update_robot_position_on_screen(lemlib::getPose(true));
                                               pros::delay(100);
                                             }
                                           },
                                           "telemetry auton task"};
  print_text_at(7, "am moving rn");
  // chassis.turnToHeading(0, 3000);
  lemlib::MoveToPointParams params = {.forwards = false};
  chassis.moveToPoint(-8, 0, 2000, params);
  // chassis.moveToPose(-8, 0, 90, 1200);
  // chassis.turnToHeading(0, 500);
  // lemlib::MoveToPointParams params = {.forwards = false};
  chassis.follow(pathTest_txt, 3, 3000);
  // chassis.moveToPoint(-8, 0, 2000, params);
  // chassis.moveToPose(-8, 0, 90, 1200);
  chassis.waitUntilDone();
  print_text_at(7, "done moving");
}

void update_buttons() {
  // put all button_expanded updates here
  fish_mech_loading_conveyor_button.update(
      controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_A));
  conveyor_enabled_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_L1));
  mogo_grabber_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_R1));
  conveyor_reverse_button.update(controller.get_digital(pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_R2));
}

// initializes opmode tasks and mechanisms for opmode(), before main loop runs.
void op_init() {
  mogo_grabber_task = new pros::Task {[=] {
                                        while (true) {
                                          if (mogo_grabber_button.is_toggled()) {
                                            mogo_grabber.extend();
                                          } else {
                                            if (mogo_grabber_button.just_pressed()) {
                                              conveyor.move_velocity(-600);
                                              pros::delay(200);
                                            }
                                            mogo_grabber.retract();
                                          }
                                          pros::delay(20);
                                        }
                                      },
                                      "mogo grabber task"};

  button_update_task = new pros::Task {[=] {
                                         while (true) {
                                           update_buttons();
                                           pros::delay(20);
                                         }
                                       },
                                       "button update task"};

  intake_task = new pros::Task {
      [=] {
        while (true) {
          // print_text_at(5, "intaking");
          if (fish_mech_loading_conveyor_button.is_toggled() and conveyor_enabled_button.is_toggled()) {
            if (fish_mech_loading_conveyor_button.just_pressed()) {
              // if button has just been pressed (not held) (and we have toggled the
              // button):
              controller.rumble("....");
              while (!fish_mech_is_loaded() and fish_mech_loading_conveyor_button.is_toggled()) { pros::delay(20); }
              // sets target for the conveyor to reach
              // the conveyor moves about 4 in for the fish mech to load the ring.
              set_conveyor_target_in_inches(.75);
            }
            if (std::abs(conveyor.get_position() - conveyor.get_target_position()) <= CONVEYOR_TARGET_THRESH) {
              conveyor.brake();
            }
            // move the conveyor to the fish mech position
          } else {
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
}

/**
 * Runs in driver control
 */
void opcontrol() {
  op_init();

  while (1) {
    update_robot_position_on_screen(lemlib::getPose(true));

    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    pros::delay(50);
  }
}

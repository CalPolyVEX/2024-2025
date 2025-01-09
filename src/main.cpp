#include "main.h"
#include "display.h"
// #include "pros/llemu.hpp"
// #ifndef HARDWARE_MAP_H
#include "hardware_map.h"
// #endif
#include "lemlib/api.hpp" // IWYU pragma: keep
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

#define IS_DEBUGGING_OTOS false
#define LIDAR false

button_expanded fish_mech_loading_conveyor_button;

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
  // pros::lcd::initialize(); // initialize brain screen

  // printf("in init");
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

  if (color.red > color.blue and LIDAR) {
    lemlib::calibrate_otos(true);
  } else {
    lemlib::calibrate_otos(false);
  }
  pros::delay(500); // dont do anything for half a sec so we can init the otos

  // pros::screen::draw_line(0, 0, 200, 200);
  initialize_screen();

  // printf("in init");

  // pros::Controller controller (pros::E_CONTROLLER_MASTER);

  while (IS_DEBUGGING_OTOS) {
    // pros::lcd::print(1, "Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y,
    // lemlib::getPose().theta);

    // pros::screen::draw_line(0, 0, 200, 200);

    // pros::screen::draw_circle(lemlib::getPose().x + 72, lemlib::getPose().y + 72, 4);

    // controller.print(2, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    // if (IS_DEBUGGING_OTOS){
    printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

    pros::delay(10);
  }

  // pros::lcd::print(0, "Pose: (%f, %f, %f)", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

  // pros::lcd::print(0, "Pose: (%f, %f, %f)", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
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
  // chassis.moveToPoint(0, 18, 3000, lemlib::MoveToPointParams());
}

// initializes opmode tasks and mechanisms for opmode(), before main loop runs.
void op_init() {
  // printf("i am hererereererere");
  button_update_task = new pros::Task {[=] {
                                         while (true) {
                                           // put all button_expanded updates here
                                           fish_mech_loading_conveyor_button.update(controller.get_digital(
                                               pros::controller_digital_e_t::E_CONTROLLER_DIGITAL_A));
                                           pros::delay(20);
                                         }
                                       },
                                       "button update task"};

  // sets priority to max-2. the odom task is priority 16.

  intake_task = new pros::Task {[=] {
                                  bool DEBUG = false;
                                  while (true) {
                                    // print_text_at(5, "intaking");
                                    if (fish_mech_loading_conveyor_button.is_toggled() or DEBUG) {
                                      if (fish_mech_loading_conveyor_button.just_pressed() or DEBUG) {
                                        // if button has just been pressed (not held) (and we have toggled the
                                        // button):
                                        controller.rumble("....");

                                        while (!load_for_fish_mech()) {
                                          // print_text_at(5, "aaaaaa");
                                          pros::delay(20);
                                          if (!fish_mech_loading_conveyor_button.is_toggled()) { break; }

                                          // sets target for the conveyor to reach
                                        }

                                        // the conveyor moves about 4 in for the fish mech to load the ring.
                                        set_conveyor_target_in_inches(3.676);
                                      }

                                      if (std::abs(conveyor.get_position() - conveyor.get_target_position()) <=
                                          CONVEYOR_TARGET_THRESH) {
                                        conveyor.brake();
                                      }
                                      // move the conveyor to the fish mech position

                                    } else {
                                      conveyor_deposit_and_intake();
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

  // print_text_at(4, "fish");

  // chassis.moveToPoint(0, 18, 3000, lemlib::MoveToPointParams());
  while (1) {
    /*
    uint8_t prox = conveyor_color_detector.get_proximity();
    print_text_at(7, fmt::format("prox is {}", prox).c_str());
    pros::c::optical_rgb_s_t color = conveyor_color_detector.get_rgb();
    print_text_at(8, fmt::format("color is: {:.1f}, {:.1f}, {:.1f}", color.red, color.green, color.blue).c_str());
    */

    // update_robot_position_on_screen(lemlib::Pose(30.0, 20.0, 3.141592/4.0));
    update_robot_position_on_screen(lemlib::getPose(true));

    // print_text_at(6, "27");

    /*
    pros::screen::erase();
    //printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    pros::screen::set_pen(pros::Color::red);
    pros::screen::draw_circle((240 + lemlib::getPose().x * scale) +
    18*cos(lemlib::degToRad(lemlib::getPose().theta)) , (136 + lemlib::getPose().y * scale) +
    18*sin(lemlib::degToRad(lemlib::getPose().theta)), 2); pros::screen::set_pen(pros::Color::blue);
    pros::screen::draw_circle((240 + lemlib::getPose().x * scale) , (136 + lemlib::getPose().y * scale) , 12);

    //pros::lcd::print(1, "Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y,
    lemlib::getPose().theta);
    */

    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    pros::delay(50);
    // controller.print(2, 0, "Pose:(%1.1f, %1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y,
    // lemlib::getPose().theta);
  }
}

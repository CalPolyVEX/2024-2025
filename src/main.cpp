#include "main.h"
#include "display.h"
#include "config.h"
//  #include "pros/llemu.hpp"
#include "auto_state_machine.cpp"

#include "fmt/format.h"
#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/chassis.hpp"
#include "lemlib/chassis/odom.hpp" // IWYU pragma: keep
#include "conveyor_ctrls.hpp"
#include "lemlib/pose.hpp"
#include "pros/misc.h"
#include "pros/misc.hpp"
#include "pros/motors.h"
#include "pros/rtos.hpp"
#include "globals.h"
#include "controls.h"
#include <cstdlib>
ASSET(pathTest_txt);

// this is .0408 inches accuracy
#define CONVEYOR_TARGET_THRESH 30 // (in ticks)

//======================================================================================

pros::Controller controller(CONTROLLER_MASTER);

pros::Task* fish_mech_task = nullptr;

pros::Task* telemetry_task = nullptr;

#define IS_DEBUGGING_OTOS false

#define ENABLE_SCREEN_FOR_DEBUG false

// drivetrain, chassis and PID controllers definitions===================================
#ifdef GREEN_BOT
lemlib::ControllerSettings lateralPIDController(18, // proportional gain (kP)
                                                0.10, // integral gain (kI)
                                                180, // derivative gain (kD)
                                                3, // anti windup
                                                0.25, // small error range, in inches
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in inches
                                                500, // large error range timeout, in milliseconds
                                                16 // maximum acceleration (slew)
);
lemlib::ControllerSettings angularPIDController(2.8, // proportional gain (kP)
                                                0.06, // integral gain (kI)
                                                18, // derivative gain (kD)
                                                3, // anti windup
                                                0.5, // small error range, in degrees
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in degrees
                                                500, // large error range timeout, in milliseconds
                                                0 // maximum acceleration (slew)
);
#endif

#ifdef GOLD_BOT
lemlib::ControllerSettings lateralPIDController(7, // proportional gain (kP)
                                                0.07, // integral gain (kI)
                                                20, // derivative gain (kD)
                                                3, // anti windup
                                                0.25, // small error range, in inches
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in inches
                                                500, // large error range timeout, in milliseconds
                                                16 // maximum acceleration (slew)
);
lemlib::ControllerSettings angularPIDController(2.8, // proportional gain (kP)
                                                0.06, // integral gain (kI)
                                                18, // derivative gain (kD)
                                                3, // anti windup
                                                0.5, // small error range, in degrees
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in degrees
                                                500, // large error range timeout, in milliseconds
                                                0 // maximum acceleration (slew)
);
#endif
lemlib::Drivetrain drivetrain(&leftMG, &rightMG, 12.25, lemlib::Omniwheel::NEW_325, 450, 0);

lemlib::Chassis chassis(drivetrain, lateralPIDController, angularPIDController);

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
  fish_mech.initialize();
  pros::delay(200);
  // initialize_screen();

  // pros::delay(300);
  // printf("%f, %f, %f", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
  ring_detector.initialize();
  // pros::Serial* s = lemlib::get_serial_ptr();

  chassis.setBrakeMode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_BRAKE);
  conveyor_color_detector.set_led_pwm(100);
  conveyor.initialize();
  conveyor_color_detector.disable_gesture();
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  if (ring_detector.has_red_ring()) {
    alliance_color = true;
    print_text_at(5, "color sensor sees red");
  } else if (ring_detector.has_blue_ring()) {
    alliance_color = false;

    print_text_at(5, "color sensor sees blue");
  } else {
    print_text_at(5, "color sensor sees nothing");
  }
  pros::delay(100);
  lemlib::init(); // initialize lemlib
  pros::delay(200);
  lemlib::calibrate_otos(true);

  //}

  // lemlib::calibrate_otos(true);
  print_text_at(8, "done calibrating");

  if (ENABLE_SCREEN_FOR_DEBUG) {
    initialize_screen();
    telemetry_task =
        new pros::Task {[=] {
                          while (true) {
                            update_robot_position_on_screen(lemlib::getPose(true)); // this also updates screen
                            pros::delay(200);
                          }
                        },
                        "telemetry task"};
  }

  while (IS_DEBUGGING_OTOS) {
    printf("\n");
    lemlib::Pose odomPose = lemlib::getPose(true);
    printf("Pose: (%f, %f, %f) \n", odomPose.x, odomPose.y, odomPose.theta);
    printf("\n");

    // update_robot_position_on_screen(lemlib::getPose(true)); // this also updates screen
    //  printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    //  print_text_at(3, fmt::format("millis = {}", pros::millis()).c_str());

    pros::delay(10);
  }
  conveyor_color_detector.set_led_pwm(0);
  conveyor_color_detector.set_integration_time(5);
  // initialize the fish mech to a VERY tight zero.
}

/**
 * Runs while the robot is disabled
 */
void disabled() {}

int selected_auton = 0;

/**
 * runs after initialize if the robot is connected to field control
 */
void competition_initialize() {
  // TODO do otos offset with lidar here??????

  // Autonomous selector

  const char* auton_names[] = {"Auton 1", "Auton 2", "Auton 3"};
  const int num_autons = sizeof(auton_names) / sizeof(auton_names[0]); // TODO make this just a hardcoded int

#define AUTON_CONTROLLER_LINE 2

  while (true) {
    if (controller.get_digital_new_press(AUTO_PREV)) {
      selected_auton = (selected_auton - 1 + num_autons) % num_autons;
    } else if (controller.get_digital_new_press(AUTO_NEXT)) {
      selected_auton = (selected_auton + 1) % num_autons;
    }

    controller.clear_line(AUTON_CONTROLLER_LINE);
    controller.print(AUTON_CONTROLLER_LINE, 0, "Auton: %s", auton_names[selected_auton]);

    if (controller.get_digital_new_press(AUTO_CONFIRM)) { break; }

    pros::delay(100);
  }

  controller.clear_line(AUTON_CONTROLLER_LINE);
  controller.print(AUTON_CONTROLLER_LINE, 0, "Selected: %s", auton_names[selected_auton]);
}

/**
 * Runs during auto
 *
 *
 * */
#ifdef GOLD_BOT
void autonomous() {
  lemlib::MoveToPointParams params = {.forwards = true};
  lemlib::MoveToPointParams speedParams = {.maxSpeed = 127};
  /*
  doinker.extend();
  chassis.moveToPoint(45, 0, 8000, speedParams); // 13.5 extra for 50, reading 51.33, -5 for 50, reading 49.37,
  chassis.waitUntilDone();
  chassis.moveToPoint(20, 0, 2000, params);
  doinker.retract();
  */
  // chassis.moveToPoint(-40, 24, 3000);
  // chassis.moveToPoint(-28, 24, 3000, params);
  chassis.turnToHeading(0, 3000);
  // chassis.waitUntilDone();
  // pros::delay(1000);
  // chassis.turnToHeading(-90, 3000);
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
  chassis.setBrakeMode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);
  print_text_at(7, "done moving");
}
#endif

#ifdef GREEN_BOT
void autonomous() {
  // chassis.moveToPose(-34.75, -24, 90, 3000);
  chassis.moveToPoint(-32.75, -24, 3000); // 12 in
  chassis.waitUntilDone();
  chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
}
#endif

void deadband(int& val, int deadband) {
  if (std::abs(val) < deadband) { val = 0; }
}

bool conveyor_is_enabled = false;

/**
 * Runs in driver control
 */

void opcontrol() {
  // op_init();

  bool is_loading = false;

  bool fishing = false;
  uint32_t last_time = pros::millis();

  while (1) {
    printf("\n");
    lemlib::Pose odomPose = lemlib::getPose(true);
    printf("Pose: (%f, %f, %f) \n", odomPose.x, odomPose.y, odomPose.theta);
    printf("\n");

    pros::delay(50);

    // FISH MECH
    int fish_axis = controller.get_analog(FISH_MANUAL_AXIS);
    deadband(fish_axis, 30);
    fish_mech.manual(fish_axis);

    if (controller.get_digital_new_press(FISH_SCORE_BUTTON)) { fish_mech.score(); }

    fish_mech.update();

    // end fish mech section

    // CONVEYOR SECTION BEGIN
    // TODO COLOR REJECTION
    if (controller.get_digital_new_press(CONVEYOR_ENABLE)) { // enable conveyor
      conveyor_is_enabled = !conveyor_is_enabled;
    } else if (controller.get_digital(CONVEYOR_REVERSE)) { // reverse conveyor
      conveyor.move_at_speed(-600);

      conveyor_is_enabled = false; // disable conveyor

    } else if (!conveyor_is_enabled) { // disabled conveyor
      conveyor.move_at_speed(0);
    }
    if (controller.get_digital_new_press(LOAD_NEXT_RING)) { // load next ring
      is_loading = !is_loading; // toggle loading
      if (is_loading) { // rumble on enable
        controller.rumble("....");
      } else { // rumble diff on cancel
        controller.rumble("..");
      }
    }

    if (is_loading) {
      ring_detector.enable_led(); // enable led on color sensor
      if (not ring_detector.has_ring()) { // if we have neither color ring
        conveyor.move_at_speed(600); // color sort and intake

      } else {
        is_loading = false; // we have finished loading bc there is a ring
        // set_conveyor_target_in_inches(float inches)
        controller.rumble(".-"); // rumble on complete
      }
    } else if (conveyor_is_enabled) { // if conveyor is enabled
      conveyor_color_detector.set_led_pwm(100);

      conveyor.move_at_speed(600); // move conveyor fast
    } else {
      conveyor_color_detector.set_led_pwm(0); // disable led on color sensor
    }

    if (controller.get_digital_new_press(SCORING_OPPOSITE_BUTTON)) { scoring_opposite = !scoring_opposite; }

    if (not scoring_opposite) { // controller telemetry
      controller.print(2, 0, "chucking color");
    } else {
      controller.clear_line(2);
    }
    // END CONVEYOR SECTION

    // BEGIN MOGO SECTION
#ifdef GREEN_BOT // JOSEPH
    if (controller.get_digital_new_press(MOGO_GRAB)) {
      mogo_grabber.toggle();
      if (mogo_grabber.is_grabbed()) {
        conveyor.move_at_speed(-600);
        pros::delay(100);
      }
    }
#else // TIM
    if (controller.get_digital_new_press(MOGO_GRAB)) {
      mogo_grabber.grab();
      conveyor.move_at_speed(-600);
      pros::delay(100);
    } else if (controller.get_digital_new_press(MOGO_DROP)) {
      mogo_grabber.release();
    }
#endif
    // END MOGO SECTION

    // BEGIN DOINKER SECTION
    if (controller.get_digital_new_press(DOINKER_BUTTON)) { doinker.toggle(); } // doinker is a newpress toggle
    // END DOINKER SECTION

    // CHASSIS
    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // END CHASSIS

    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
  }
}

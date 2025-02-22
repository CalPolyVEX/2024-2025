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
/*
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
lemlib::ControllerSettings angularPIDController(2, // proportional gain (kP)
                                                0, // integral gain (kI)
                                                10, // derivative gain (kD)
                                                3, // anti windup
                                                0.5, // small error range, in degrees
                                                100, // small error range timeout, in milliseconds
                                                1, // large error range, in degrees
                                                500, // large error range timeout, in milliseconds
                                                0 // maximum acceleration (slew)
);
*/

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

lemlib::Drivetrain drivetrain(&leftMG, &rightMG, 12.25, lemlib::Omniwheel::NEW_325, 450, 0);

lemlib::Chassis chassis(drivetrain, lateralPIDController, angularPIDController);

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */

void initialize() {
  fish_mech.move_velocity(-600);
  pros::delay(200);
  fish_mech.tare_position();
  fish_mech.brake();
  pros::delay(200);
  // initialize_screen();
  lemlib::init(); // initialize lemlib
  // pros::delay(300);
  // printf("%f, %f, %f", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);

  // pros::Serial* s = lemlib::get_serial_ptr();

  chassis.setBrakeMode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_BRAKE);
  conveyor_color_detector.set_led_pwm(100);

  fish_mech.set_brake_mode(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_HOLD);
  fish_mech.set_encoder_units(pros::MotorEncoderUnits::degrees);

  // the conveyor is already tensioned and frictioned.
  // it seems to stop abruptly as hard braking anyway
  conveyor.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

  conveyor.set_encoder_units(pros::motor_encoder_units_e_t::E_MOTOR_ENCODER_COUNTS);

  // this coast behavior makes sense though, the roller doesnt need to brake hard.
  intake.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

  conveyor_color_detector.disable_gesture();
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  // while (true) {
  // print_text_at(7, fmt::format("color sensor sees: ({}, {}, {})", color.red, color.green, color.blue).c_str());
  // print_text_at(8, fmt::format("prox sensor sees: {}", conveyor_color_detector.get_proximity()).c_str());
  if (has_red_ring()) {
    alliance_color = true;
    print_text_at(5, "color sensor sees red");
  } else if (has_blue_ring()) {
    alliance_color = false;

    print_text_at(5, "color sensor sees blue");
  } else {
    print_text_at(5, "color sensor sees nothing");
  }

  lemlib::calibrate_otos(true);

  //}

  // lemlib::calibrate_otos(true);
  pros::delay(600); // dont do anything for half a sec so we can init the otos

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
    update_robot_position_on_screen(lemlib::getPose(true)); // this also updates screen
    // printf("Pose: (%f, %f, %f) \n", lemlib::getPose().x, lemlib::getPose().y, lemlib::getPose().theta);
    // print_text_at(3, fmt::format("millis = {}", pros::millis()).c_str());

    pros::delay(10);
  }
  conveyor_color_detector.set_led_pwm(0);

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
  chassis.moveToPoint(-28, 24, 3000, params);
  // chassis.turnToHeading(0, 3000);
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
void autonomous() {}
#endif

/*
void op_init(){


  fish_mech_task = new pros::Task {[=] {
    bool fish_mech_override_flag = true;

    while (true){





      // FISH MECH
      int fish_axis = controller.get_analog(FISH_MANUAL_AXIS);
      deadband(fish_axis, 30);



      fish_mech_override_flag = fish_axis != 0;

      //print_text_at(10, fmt::format("fish mech pos = {}", fish_mech.get_position()).c_str());



      //print_text_at(9, fmt::format("fish mech flag is {}", fish_mech_override_flag).c_str());

      //print_text_at(8, fmt::format("fish velo = {}", fish_mech.get_actual_velocity()).c_str());

      if (controller.get_digital_new_press(FISH_SCORE_BUTTON) and not fish_mech_override_flag) {
        //if we arent inputting fish manual controls and we press (Y)

        score_with_fish_mech(); // score
        while (std::abs(fish_mech.get_position() - fish_mech.get_target_position()) >= 4 ){  // if we have not
met/exceeded target pros::delay(200); //wait
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
          } else {
            fish_mech.move_velocity(-1);

          }
          //fish_mech.move_absolute(0, 600);

        }
      }
      pros::delay(80);
      //print_text_at(11, fmt::format("fish axis val = {}", fish_axis).c_str());




      //print_text_at(7, fmt::format("is_fishing = {}", is_fishing).c_str());



      //print_text_at(5, fmt::to_string(pros::millis()).c_str());
    }
  }, "fish mech task"};

}
*/

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
    print_text_at(5, fmt::format("fish pos = {}", fish_mech.get_position()).c_str());
    // FISH MECH
    int fish_axis = controller.get_analog(FISH_MANUAL_AXIS);
    deadband(fish_axis, 30);

    if (controller.get_digital_new_press(FISH_SCORE_BUTTON)) {
      fishing = true; // start fishing
      score_with_fish_mech(); // set target on a newpress
    }

    printf("TARGET POS = %f\n", fish_mech.get_target_position());
    printf("CUR POS = %f\n", fish_mech.get_position());
    if (fishing) { // if currently fishing
      if (fish_mech.get_flags() &
          pros::E_MOTOR_FLAGS_ZERO_VELOCITY) { // check if we are stopped (the method DNE) BRUUUUUUUHHHHH

        // if we at target, zero.
        printf("TRAPPED IN TARGET CALL\n");
        if ((pros::millis() - last_time) > FISH_SCORE_DELAY) { // if we zeroing and past delay, complete zero
          last_time = pros::millis();
          fish_mech.move_absolute(0, 600);
          fishing = false;
        }

      } else {
        printf("TRAPPED IN LAST ELSE\n");
        last_time = pros::millis();
      }

    } else {
      if (fish_axis != 0) { // manual override
        printf("fish axis is %d\n", fish_axis);
        fish_mech.move_velocity(fish_axis); // move by stick
      } else if (fish_mech.get_position() <= 175 and (fish_mech.get_position() != 0) and
                 not(fish_mech.get_flags() & pros::E_MOTOR_FLAGS_ZERO_VELOCITY)) {
        // if we below 160, above 0, and we ARE moving, zero
        printf("TRAPPED IN ZERO FOR MANL\n");
        fish_mech.move_absolute(0, 600);
      } else {
        printf("TRAPPED IN BRAKE\n");
        fish_mech.brake();
      }
    }

    // end fish mech section

    lemlib::Pose pose = lemlib::getPose(false); // get pose
    // printf("%f, %f, %f\n", pose.x, pose.y, pose.theta); // print pos to terminal

    // update_robot_position_on_screen(lemlib::getPose(true));
    // print_text_at(4, fmt::format("millis = {}", pros::millis()).c_str());

    if (controller.get_digital_new_press(CONVEYOR_ENABLE)) { // enable conveyor
      conveyor_is_enabled = !conveyor_is_enabled;
    } else if (controller.get_digital(CONVEYOR_REVERSE)) { // reverse conveyor
      conveyor_deposit_and_intake(-600);
      conveyor_is_enabled = false; // disable conveyor

    } else if (!conveyor_is_enabled) { // disabled conveyor
      conveyor.move_velocity(0);
      intake.move_velocity(100); // keep a little bit of intake to hold rings
    }
    if (controller.get_digital_new_press(LOAD_NEXT_RING)) { // load next ring
      // TODO CHECK THIS TO SEE IF IT WORKS (MIGHT BE BUGGED)
      is_loading = !is_loading; // toggle loading
      if (is_loading) { // rumble on enable
        controller.rumble("....");
      } else { // rumble on cancel
        controller.rumble("..");
      }
    }

    if (is_loading) {
      conveyor_color_detector.set_led_pwm(100);
      if (not fish_mech_is_loaded()) { // if we have neither color ring
        conveyor_deposit_and_intake(); // color sort and intake

      } else {
        is_loading = false; // we have finished loading bc there is a ring
        // set_conveyor_target_in_inches(float inches)
        controller.rumble(".-"); // rumble on complete
      }
    } else if (conveyor_is_enabled) { // if conveyor is enabled
      conveyor_color_detector.set_led_pwm(100);

      conveyor_deposit_and_intake(); // color sort and intake
    } else {
      conveyor_color_detector.set_led_pwm(0); // disable led on color sensor
    }

    if (controller.get_digital_new_press(SCORING_OPPOSITE_BUTTON)) { scoring_opposite = !scoring_opposite; }

    if (not scoring_opposite) { // controller telemetry
      controller.print(2, 0, "chucking color");
    } else {
      controller.clear_line(2);
    }

#ifdef GREEN_BOT // JOSEPH
    if (controller.get_digital_new_press(MOGO_GRAB)) {
      mogo_grabber.toggle();
      if (mogo_grabber.is_extended()) {
        conveyor.move_velocity(-600);
        pros::delay(100);
      }
    }
#else // TIM
    if (controller.get_digital_new_press(MOGO_GRAB)) {
      mogo_grabber.extend();
      conveyor.move_velocity(-600);
      pros::delay(100);
    } else if (controller.get_digital_new_press(MOGO_DROP)) {
      mogo_grabber.retract();
    }
#endif

    if (controller.get_digital_new_press(DOINKER_BUTTON)) { doinker.toggle(); } // doinker is a newpress toggle

    chassis.arcade(controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_LEFT_Y),
                   controller.get_analog(pros::controller_analog_e_t::E_CONTROLLER_ANALOG_RIGHT_X));
    // controller.print(1, 0, "Pose:(%1.1f, %1.1f)", lemlib::getPose().x, lemlib::getPose().y);
    pros::delay(50);
  }
}

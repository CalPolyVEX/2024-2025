#include "robot/fish_mech.hpp"

#define SCORE_POS 175.0
#define FISH_SCORE_DELAY 400
#define DEPLOYING_THRESHOLD 30.0
#define MANUAL_RESET_THRESHOLD 175.0
#define MOTOR_VELOCITY 600

FishMech::FishMech(int motor_port)
  : motor(motor_port, pros::MotorCartridge::red),
    state(State::HOME),
    should_score(false),
    manual_velocity(0),
    holding_finish_time(0) {}

void FishMech::initialize() {
  motor.move_velocity(-MOTOR_VELOCITY);
  pros::delay(200);
  motor.tare_position();
  motor.brake();

  motor.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
  motor.set_encoder_units(pros::MotorEncoderUnits::degrees);
}

void FishMech::score() {
  if (state == State::HOME) { should_score = true; }
}

void FishMech::manual(int velocity) { manual_velocity = velocity; }

void FishMech::update() {
  if (manual_velocity != 0) { state = State::MANUAL; }

  switch (state) {
    // In home position and braked.
    case State::HOME:
      if (should_score) {
        should_score = false;

        motor.move_absolute(SCORE_POS, MOTOR_VELOCITY);
        state = State::DEPLOYING;
      }
      break;

    // Moving the fish mech to the score position.
    case State::DEPLOYING:
      if (motor.get_position() > DEPLOYING_THRESHOLD && motor_stopped()) {
        holding_finish_time = pros::millis() + FISH_SCORE_DELAY;
        state = State::HOLDING;
      }
      break;

    // Holding the fish mech in the score position for a short time.
    case State::HOLDING:
      if (pros::millis() >= holding_finish_time) {
        motor.move_absolute(0, MOTOR_VELOCITY);
        state = State::RETRACTING;
      }
      break;

    // Moving the fish mech back to the home position.
    case State::RETRACTING:
      if (motor_stopped()) {
        motor.brake();
        state = State::HOME;
      }
      break;

    // Manual control of the fish mech.
    case State::MANUAL:
      motor.move_velocity(manual_velocity);

      if (manual_velocity == 0 && (motor.get_position() <= MANUAL_RESET_THRESHOLD)) {
        motor.move_absolute(0, MOTOR_VELOCITY);
        state = State::RETRACTING;
      }
      break;
  }
}

bool FishMech::motor_stopped() { return motor.get_flags() & pros::E_MOTOR_FLAGS_ZERO_VELOCITY; }

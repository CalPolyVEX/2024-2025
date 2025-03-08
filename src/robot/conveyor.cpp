#include "robot/conveyor.hpp"
#include "pros/motors.hpp"

#define TPI 122.323 // ticks per inch (300.0 * (60.0 / 36.0)) / (1.3 * M_PI)

Conveyor::Conveyor(int conveyor_port, int intake_port)
  : conveyor(conveyor_port, pros::MotorCartridge::green),
    intake(intake_port, pros::MotorCartridge::green) {}
    void Conveyor::initialize() {
        
        // the conveyor is already tensioned and frictioned.
        // it seems to stop abruptly as hard braking anyway
        conveyor.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

        conveyor.set_encoder_units(pros::motor_encoder_units_e_t::E_MOTOR_ENCODER_COUNTS);

        // this coast behavior makes sense though, the roller doesnt need to brake hard.
        intake.set_brake_mode_all(pros::motor_brake_mode_e_t::E_MOTOR_BRAKE_COAST);

    }
    void Conveyor::move_at_speed(int speed) {
        intake.move_velocity(speed);
        conveyor.move_velocity(speed);
    }
    void Conveyor::move_distance(double inches, int speed) {
        intake.move_velocity(speed);
        conveyor.move_relative(inches/TPI, speed);
    }
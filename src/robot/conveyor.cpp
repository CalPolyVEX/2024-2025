#include "robot/conveyor.hpp"
#include "pros/motors.hpp"

#define TPI 122.323

Conveyor::Conveyor(int conveyor_port, int intake_port):
    conveyor(conveyor_port, pros::MotorCartridge::green),
    intake(intake_port, pros::MotorCartridge::green) {}
    
    void Conveyor::move_at_speed(int speed) {
        intake.move_velocity(speed);
        conveyor.move_velocity(speed);
    }
    void Conveyor::move_distance(double inches, int speed) {
        intake.move_velocity(speed);
        conveyor.move_relative(inches/TPI, speed);
    }
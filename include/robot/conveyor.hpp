

#include "pros/motors.hpp"

class Conveyor {
  public:
    Conveyor(int conveyor_port, int intake_port);
    void initialize();
    void move_at_speed(int speed);
    void move_distance(double inches, int speed);
  private:
    pros::Motor conveyor;
    pros::Motor intake;
};
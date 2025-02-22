#ifndef FISH_MECH_H
#define FISH_MECH_H

#include "pros/motors.hpp"

class FishMech {
public:
    FishMech(int motor_port);
    void initialize();
    void score();
    void manual(int velocity);
    void update();

private:
    bool motor_stopped();

    enum State {
        HOME,
        DEPLOYING,
        HOLDING,
        RETRACTING,
        MANUAL,
    };

    pros::Motor motor;
    State state;
    bool should_score;
    int manual_velocity;
    uint32_t holding_finish_time;
};

#endif
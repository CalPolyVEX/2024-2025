#ifndef DOINKER_H
#define DOINKER_H

#include "pros/adi.hpp"

class Doinker {
public:
    Doinker(char port);
    void toggle();
    void extend();
    void retract();
    bool is_extended();

private:
    pros::adi::DigitalOut piston;
    bool extended;
};

#endif
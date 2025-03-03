#ifndef MOGO_GRABBER_H
#define MOGO_GRABBER_H

#include "pros/adi.hpp"

class MogoGrabber {
  public:
    MogoGrabber(char port);
    void toggle();
    void grab();
    void release();
    bool is_grabbed();
  private:
    pros::adi::DigitalOut piston;
    bool grabbed;
};

#endif
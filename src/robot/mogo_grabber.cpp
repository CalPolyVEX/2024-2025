#include "robot/mogo_grabber.hpp"

MogoGrabber::MogoGrabber(char port)
  : piston(port, false),
    grabbed(false) {}

void MogoGrabber::toggle() {
  if (grabbed) {
    release();
  } else {
    grab();
  }
}

void MogoGrabber::grab() {
  piston.set_value(true);
  grabbed = true;
}

void MogoGrabber::release() {
  piston.set_value(false);
  grabbed = false;
}

bool MogoGrabber::is_grabbed() { return grabbed; }

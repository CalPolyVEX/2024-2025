#include "doinker.h"

Doinker::Doinker(char port)
  : piston(port, false),
    extended(false) {}

void Doinker::toggle() {
  if (extended) {
    retract();
  } else {
    extend();
  }
}

void Doinker::extend() {
  piston.set_value(1);
  extended = true;
}

void Doinker::retract() {
  piston.set_value(0);
  extended = false;
}

bool Doinker::is_extended() { return extended; }
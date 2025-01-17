#include "button_manager.hpp"

#include "config.h"

void ButtonManager::update(pros::Controller& controller) {
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L1))
    conveyor = ConveyorState::Forward;
  else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_L2))
    conveyor = ConveyorState::Reverse;
  else if (conveyor == ConveyorState::Reverse)
    conveyor = ConveyorState::Off;
  
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1))
    mogo_grabber = true;
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2))
    mogo_grabber = false;

  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_UP))
    fish = FishState::OverrideUp;
  else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN))
    fish = FishState::OverrideDown;
  
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_LEFT))
    color_rejection = !color_rejection;
  
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT))
    doinker = !doinker;
  
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_A))
    conveyor = ConveyorState::WaitingForRing;
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_Y))
    fish = FishState::SequenceStart;
}

ConveyorState ButtonManager::getConveyorState() const {
  return conveyor;
}

FishState ButtonManager::getFishState() const {
  return fish;
}

bool ButtonManager::isColorRejectionEnabled() const {
  return color_rejection;
}

bool ButtonManager::isDoinkerDown() const {
  return doinker;
}

bool ButtonManager::isMogoGrabberOpen() const {
  return mogo_grabber;
}


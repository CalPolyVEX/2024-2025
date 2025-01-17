#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "pros/misc.hpp"

enum class ConveyorState {
  Idle,
  Off,
  Forward,
  Reverse,
  WaitingForRing,
  HoldingRing,
};

enum class FishState {
  Idle,
  SequenceStart,
  OverrideUp,
  OverrideDown,
};

class ButtonManager {
  private:
    ConveyorState conveyor = ConveyorState::Idle;
    FishState fish = FishState::Idle;
    bool color_rejection = false;
    bool doinker = false;
    bool mogo_grabber = false;

  public:

    void update(pros::Controller& controller);

    ConveyorState getConveyorState() const;
    FishState getFishState() const;
    bool isColorRejectionEnabled() const;
    bool isDoinkerDown() const;
    bool isMogoGrabberOpen() const;    
};

#endif

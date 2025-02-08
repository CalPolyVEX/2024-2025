#include "config.h"

#ifndef GLOBALS_H
#define GLOBALS_H

// controller
#define DOINKER_BUTTON pros::E_CONTROLLER_DIGITAL_RIGHT
#define SCORING_OPPOSITE_BUTTON pros::E_CONTROLLER_DIGITAL_LEFT
#define CONVEYOR_ENABLE pros::E_CONTROLLER_DIGITAL_L1
#define CONVEYOR_REVERSE pros::E_CONTROLLER_DIGITAL_L2
#define FISH_SCORE_BUTTON pros::E_CONTROLLER_DIGITAL_Y
#define LOAD_NEXT_RING pros::E_CONTROLLER_DIGITAL_A
#define FISH_DELAY 400

#define FISH_MANUAL_AXIS pros::E_CONTROLLER_ANALOG_RIGHT_Y

#define AUTO_CONFIRM pros::E_CONTROLLER_DIGITAL_A
#define AUTO_CANCEL pros::E_CONTROLLER_DIGITAL_B
#define AUTO_NEXT pros::E_CONTROLLER_DIGITAL_RIGHT
#define AUTO_PREV pros::E_CONTROLLER_DIGITAL_LEFT

#ifdef GOLD_BOT

// controller
#define MOGO_GRAB pros::E_CONTROLLER_DIGITAL_R1
#define MOGO_DROP pros::E_CONTROLLER_DIGITAL_R2

#endif

#ifdef GREEN_BOT

// controller
#define MOGO_GRAB pros::E_CONTROLLER_DIGITAL_R1

#endif

#ifdef PROTOTYPE_BOT

// controller
#define MOGO_GRAB pros::E_CONTROLLER_DIGITAL_R1

#endif

#endif
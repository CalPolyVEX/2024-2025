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


#ifdef GOLD_BOT

// controller
#define MOGO_GRAB pros::E_CONTROLLER_DIGITAL_R1
#define MOGO_DROP pros::E_CONTROLLER_DIGITAL_R2

#define FISH_SCORE_BUTTON pros::E_CONTROLLER_DIGITAL_Y

#endif

#ifdef GREEN_BOT

// controller
#define MOGO_GRAB pros::E_CONTROLLER_DIGITAL_R1

#define FISH_MANUAL_AXIS pros::E_CONTROLLER_ANALOG_RIGHT_Y





#endif


#endif
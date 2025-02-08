#ifndef PORTS_H
#define PORTS_H

#include "config.h"

#ifdef GOLD_BOT
#define PORT_LEFT_MOTOR_1 -6
#define PORT_LEFT_MOTOR_2 7
#define PORT_LEFT_MOTOR_3 -8
#define PORT_RIGHT_MOTOR_1 1
#define PORT_RIGHT_MOTOR_2 -2
#define PORT_RIGHT_MOTOR_3 3
#define PORT_FISH_MOTOR 9
#define PORT_CONVEYOR_MOTOR -4
#define PORT_INTAKE_MOTOR -5
#define PORT_CONVEYOR_COLOR_SENSOR 21
#define PORT_MOGO_GRABBER 'A'
#define PORT_REJECTOR 'C'
#define PORT_DOINKER 'B'
#endif

#ifdef GREEN_BOT
// TODO: Fill this in please
#endif

#ifdef PROTOTYPE_BOT
#define PORT_LEFT_MOTOR_1 15
#define PORT_LEFT_MOTOR_2 -9
#define PORT_LEFT_MOTOR_3 -18
#define PORT_RIGHT_MOTOR_1 -19
#define PORT_RIGHT_MOTOR_2 4
#define PORT_RIGHT_MOTOR_3 5
#define PORT_FISH_MOTOR 3
#define PORT_CONVEYOR_MOTOR -2
#define PORT_INTAKE_MOTOR -6
#define PORT_CONVEYOR_COLOR_SENSOR 13
#define PORT_MOGO_GRABBER 'C'
#define PORT_REJECTOR 'B'
#define PORT_DOINKER 'D'
#endif

#endif
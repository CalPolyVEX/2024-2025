#ifndef MOTOR_SERVO_CONTROL_H
#define MOTOR_SERVO_CONTROL_H

void sync_servos(unsigned short servo_number);
void motor_setup();
void change_motor_speed(unsigned short motor_num, int speed);
void set_servo_angle(unsigned short servo_num, unsigned position);

#endif

#ifndef MOTOR_SERVO_CONTROL_H
#define MOTOR_SERVO_CONTROL_H

// Waits for the Servos to be Synced
void sync_servos(uint8_t servo_number);

// Intialize Moter and Servo Timers Through This Function
void motor_setup();

// These Functions Set the Speed of the Specified Moter/Servo
// The Speed is Clamped to Values Between -100 and 100
void change_motor_speed(uint8_t motor_num, byte speed);
void set_servo_angle(uint8_t servo_num, byte speed);

// Transform a Signed Byte Between -100 and 100 to an Unsigned Byte Centered Around 127
// Also Performs Clamping of Speed Between -100 and 100
uint8_t transformSpeed(byte speed);

#endif

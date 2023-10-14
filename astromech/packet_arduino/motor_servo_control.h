#ifndef MOTOR_SERVO_CONTROL_H
#define MOTOR_SERVO_CONTROL_H

// Waits for the Servos to be Synced
void sync_servos(uint8_t servo_number);

// Intialize Motor and Servo Timers Through This Function
void motor_setup();

// These Functions Set the Speed of the Specified Motor/Servo
// The Speed is Clamped to Values Between -100 and 100
void control_motors_joystick(uint16_t ver_val, uint16_t hor_val);
void change_motor_speed(uint8_t motor_num, int16_t speed);
void set_servo_angle(uint8_t servo_num, int16_t speed);

// Transform a Signed Byte Between -100 and 100 to an Unsigned Byte Centered Around 127
// Also Performs Clamping of Speed Between -100 and 100
uint8_t transformSpeed(int16_t speed, int min_speed, int max_speed, int scalar);

// Sets the Max Motor Speed from Debugger
void set_max_motor_speed(int new_max);

// Sets the Motor Speed Scalar
void set_motor_speed_scalar(int new_scalar);

// Sets the Bias of the Motor Speed
void set_motor_speed_bias(int new_bias);

// Sets the Acceleration for the Motors
void set_motor_acceleration(int new_value);

// Sets the Max Servo Speed from Debugger
void set_max_servo_speed(int new_max);

// Sets the Servo Speed Scalar
void set_servo_speed_scalar(int new_scalar);

#endif

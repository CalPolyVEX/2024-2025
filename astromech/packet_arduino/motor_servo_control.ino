#include "motor_servo_control.h"
// C++ code
// The TCC0 (servos) and TCC1 (motors) timers run at 48MHz.  The period of the 
// PWM period of the left/right motors and servos is 48000000/960000 = 50Hz.
const uint32_t MOTOR_WAVELENGTH = 960000;

// The MOTOR_BASE_TIME is the default on time for a servo/motor signal.  
// The computation for the signal 'on' time is:  
// MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_value) / 48000000, so the
// default 'on' time for a servo in neutral position is:
//
// 54220 + (140 * 127) = 72000, and 72000/48000000 = 1.5ms (neutral servo position)
//
const float MOTOR_BASE_TIME = 54220;

// multiplied by the motor_value to get change in range of wavelength needed (For
// Full Range Value is 189, Lowered for Safety)
float MOTOR_CHANGE_CONST = 140;

// range is 27 (100% backward) to 127 (0% speed) to 227 (100% forward) for left
// motor, initialize to the stopped value of 127
unsigned int motor_value_left = 127;

// range is 27 (100% backward) to 127 (0% speed) to 227 (100% forward) for right
// motor, initialize to the stopped value of 127
unsigned int motor_value_right = 127;

// Same as Above but for Servos
unsigned int servo_value[4] = {127, 127, 127, 127};

// The current angle of the servos
// unsigned int current_servo_position[4] = {0, 0, 0, 0};
// The servos will move until their current angle matches these values
// unsigned int target_servo_position[4] = {0, 0, 0, 0};
// unsigned int waveform_map[4] = {0, 2, 1, 3};
// The Servo Waveform Output indexed by servo number
unsigned int waveform_map[4] = {3, 1, 2, 0};
// WARNING: Only Use Values Between 27 and 227 For Motor Value To Keep Motor
// in Bounds

// Left Motor:  On PCB: D9 (PA07)
// Right Motor: On PCB: D8 (PA06) 
// Servo 0:     On PCB: D7 (PA21) WO[7]
// Servo 1:     On PCB: D5 (PA15) WO[5]
// Servo 2:     On PCB: D6 (PA20) WO[6]
// Servo 3:     On PCB: A3 (PA04) WO[0]

// Min/Max Motor Speed for Debugger
int max_motor_speed = 100;
int min_motor_speed = -100;

// Motor Speed Scalar
int motor_speed_scalar = 0;

// Min/Max Servo Speed for Debugger
int max_servo_speed = 100;
int min_servo_speed = -100;

// Servo Speed Scalar
int servo_speed_scalar = 0;

// Wait for Servo Wavelengths to Sync
void sync_servos(uint8_t servo_number) {
    switch (servo_number) {
    case 0: {
        while (TCC0->SYNCBUSY.bit.CC0) { // Connecting to servo 0's counter
        };
        break;
    }
    case 1: {
        while (TCC0->SYNCBUSY.bit.CC2) { // Connecting to servo 1's counter
        };
        break;
    }
    case 2: {
        while (TCC0->SYNCBUSY.bit.CC1) { // Connecting to servo 2's counter
        };
        break;
    }
    case 3: {
        while (TCC0->SYNCBUSY.bit.CC3) { // Connecting to servo 3's counter
        };
        break;
    }
    }
}

void motor_setup() {

    // Set GCLK5's prescaler
    GCLK->GENDIV.reg =
        GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
        GCLK_GENDIV_ID(5);   // Select Generic Clock (GCLK) 5
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // Configure GCLK5 to use DFLL48M
    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |         // Set the duty cycle to 50/50 HIGH/LOW
                        GCLK_GENCTRL_GENEN |       // Enable GCLK5
                        GCLK_GENCTRL_SRC_DFLL48M | // Set the 48MHz clock source
                        GCLK_GENCTRL_ID(5);        // Select GCLK5
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // Connect GCLK5 to TCC0, TCC1
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |       // Enable GCLK5 to TCC0 and TCC1
                        GCLK_CLKCTRL_GEN_GCLK5 |   // Select GCLK5
                        GCLK_CLKCTRL_ID_TCC0_TCC1; // Feed the GCLK5 to TCC0 and TCC1
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // Set prescaler TCCDiv for TCC1
    TCC1->CTRLA.reg |=
        TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); // set prescalar to divide by 1

    // Use Normal PWM
    TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC1->SYNCBUSY.bit.WAVE) {
    };

    // The PER register determines the period of the PWM
    // uint32_t period = 960000; //48000000/960000 = 50Hz
    TCC1->PER.reg = MOTOR_WAVELENGTH; // this is a 24-bit register
    while (TCC1->SYNCBUSY.bit.PER) {
    };

    ///////////////////////////////
    // Left Motor

    // Set the duty cycle to controller speeds
    // this set the on time of the PWM cycle based on the motor value set by
    // the controller
    TCC1->CC[1].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_value_left);
    while (TCC1->SYNCBUSY.bit.CC1) {
    };

    // set PA07 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA07; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA07; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[7].reg |= PORT_PINCFG_PMUXEN;

    // Set PA07's function to function E. Function E is TCC1/WO[1] for PA07.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 3.
    PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXO_E;

    ///////////////////////////////
    // Right Motor

    // Set the duty cycle to controller speeds
    // this set the on time of the PWM cycle based on the motor value set by
    // the controller
    TCC1->CC[0].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_value_right);
    while (TCC1->SYNCBUSY.bit.CC0) {
    };

    // set PA06 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA06; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA06; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[6].reg |= PORT_PINCFG_PMUXEN;

    // Set PA06's function to function E. Function E is TCC1/WO[0] for PA06.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 3.
    PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXE_E;

    ///////////////////////////////
    // Enable TCC1
    TCC1->CTRLA.reg |= (TCC_CTRLA_ENABLE);
    while (TCC1->SYNCBUSY.bit.ENABLE) {
    };

    //////////////////////
    // Set Up Servos Timer

    // Set prescaler TCCDiv for TCC0
    TCC0->CTRLA.reg |=
        TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); // set prescalar to divide by 1

    // Use Normal PWM
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC0->SYNCBUSY.bit.WAVE) {
    };

    // The PER register determines the period of the PWM
    // uint32_t period = 960000; //48000000/960000 = 50Hz

    TCC0->PER.reg = MOTOR_WAVELENGTH; // this is a 24-bit register
    while (TCC0->SYNCBUSY.bit.PER) {
    };

    // Servo 0 (D7 PA21 WO[7])

    // set PA21 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA21; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA21; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[21].reg |= PORT_PINCFG_PMUXEN;

    // Set PA21's function to function F. Function F is TCC0/WO[7] for PA21.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 10
    PORT->Group[0].PMUX[10].reg |= PORT_PMUX_PMUXO_F;

    // Servo 1 (D5 PA15 WO[5])

    // set PA15 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA15; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA15; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[15].reg |= PORT_PINCFG_PMUXEN;

    // Set PA15's function to function F. Function F is TCC0/WO[5] for PA15.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 7.
    PORT->Group[0].PMUX[7].reg |= PORT_PMUX_PMUXO_F;

    // Servo 2 (D6 PA20 WO[6])

    // Set PA20 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA20; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA20; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[20].reg |= PORT_PINCFG_PMUXEN;

    // Set PA20's function to function F. Function F is TCC0/WO[6] for PA20.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 10.
    PORT->Group[0].PMUX[10].reg |= PORT_PMUX_PMUXE_F;

    // Servo 3 (A3 PA04 WO[0])

    // Set PA04 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA04; // set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA04; // set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[4].reg |= PORT_PINCFG_PMUXEN;

    // Set PA04's function to function E. Function E is TCC0/WO[6] for PA04.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 2.
    PORT->Group[0].PMUX[2].reg |= PORT_PMUX_PMUXE_E;

    // Set Waveform Output for All Servos
    for (int i = 0; i < 4; i++) {
        // Set the duty cycle to controller speeds
        TCC0->CC[waveform_map[i]].reg =
            MOTOR_BASE_TIME +
            (MOTOR_CHANGE_CONST *
             servo_value[i]); // this set the on time of the PWM cycle based
                                    // on the motor_value set by the controller
        sync_servos(i);
    }

    // Enable TCC0
    TCC0->CTRLA.reg |= (TCC_CTRLA_ENABLE);
    while (TCC0->SYNCBUSY.bit.ENABLE) {
    };
}

/** Control motors using joystick
 * currently, the range of ver_val and hor_val is between 0 and 255
 */
void control_motors_joystick(uint16_t ver_val, uint16_t hor_val) {
    // These calculations might need to be
    // changed based on calibrations!!
    // center both values at 0
    int16_t hor_val_origin = hor_val - 124;
    int16_t ver_val_origin = ver_val - 124;
    int16_t left_motor = ver_val_origin + hor_val_origin;
    int16_t right_motor = ver_val_origin - hor_val_origin;
    // The Clamping of Values is Now Done in the Motor Servo Control
    // ver_val represents throttle, hor_val represents steering

    int16_t left_speed = ((left_motor * 100) >> 9);
    int16_t right_speed = ((right_motor * 100) >> 9);

    // Clamp Speed if Less Than 3
    if (abs(left_speed) < 3)
      left_speed = 0;
    if (abs(right_speed) < 3)
      right_speed = 0;

    // SerialUSB.print(left_speed);
    // SerialUSB.print(" ");
    // SerialUSB.print(right_speed);
    // SerialUSB.print("\n");

    // set left motor: throttle + steering
    change_motor_speed(0, -left_speed);
    // set right motor: throttle - steering
    change_motor_speed(1, -right_speed);
}

// Sets Motor Speed
void change_motor_speed(uint8_t motor_num, int16_t speed) {
    // Left Motor
    if (motor_num) {
        // Calculate speed value
        motor_value_left = transformSpeed(speed, min_motor_speed, max_motor_speed, motor_speed_scalar);

        // Update timer values
        TCC1->CC[0].reg =
            MOTOR_BASE_TIME +
            (MOTOR_CHANGE_CONST * motor_value_left); // this sets the on time of the PWM cycle
        while (TCC1->SYNCBUSY.bit.CC0) {
        };
    }

    // Right Motor
    else {
        // Calculate speed value
        motor_value_right = 255 - transformSpeed(speed, min_motor_speed, max_motor_speed, motor_speed_scalar);

        // Update timer values
        TCC1->CC[1].reg =
            MOTOR_BASE_TIME +
            (MOTOR_CHANGE_CONST * motor_value_right); // this set the on time of the PWM cycle
        while (TCC1->SYNCBUSY.bit.CC1) {
        };
    }
}

// Sets Servo Angle
void set_servo_angle(uint8_t servo_num, int16_t speed) {
    // Change Value
    servo_value[servo_num] = transformSpeed(speed, min_servo_speed, max_servo_speed, servo_speed_scalar);

    // Update Servo
    TCC0->CC[waveform_map[servo_num]].reg =
        MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST *
                           servo_value[servo_num]); // this sets the on time of the PWM cycle
    sync_servos(servo_num);
}

// Transform a Signed Byte Between -100 and 100 to an Unsigned Byte Centered Around 127
// Also Performs Clamping of Speed Between -100 and 100
uint8_t transformSpeed(int16_t speed, int min_speed, int max_speed, int scalar) {

    // Apply the Scalar
    if (speed > 0) {
        speed += scalar;
        if (speed < 0)
            speed = 0;
    }
    else {
        speed -= scalar;
        if (speed > 0)
            speed = 0;
    }

    // Clamp Speed Between Min and Max Speed
    if (speed > max_speed)
        speed = max_speed;
    else if (speed < min_speed)
        speed = min_speed;

    // Transform Value into an Unsigned Byte
    return 127 + speed;
}

// Sets the Max Motor Speed from Debugger
void set_max_motor_speed(int new_max)
{
    int absolute_max = abs(new_max);
    max_motor_speed = absolute_max;
    min_motor_speed = -absolute_max;
}

// Sets the Motor Speed Scalar
void set_motor_speed_scalar(int new_scalar)
{
    motor_speed_scalar = abs(new_scalar);
}
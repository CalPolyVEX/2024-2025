// C++ code
const uint32_t MOTOR_WAVELENGTH = 960000; //wavelength from peak to peak in Hz
const float MOTOR_BASE_TIME = 47997; //value in Hz of high length when speed = 100% backwards
float MOTOR_CHANGE_CONST = 189; //multiplied by percentage to get change in range of wavelength needed
unsigned int motor_percentage_1 = 127; //range is 0 (100% backward) to 127 (0% speed) to 254 (100% forward) for left motor
unsigned int motor_percentage_2 = 127; //range is 0 (100% backward) to 127 (0% speed) to 254 (100% forward) for right motor
unsigned int servo_percentages[4] = {127, 127, 217, 127}; // Same as Above but for Servos 
unsigned int current_servo_position[4] = {0, 0, 0, 0}; // The current angle of the servos
unsigned int target_servo_position[4] = {0, 0, 0, 0}; // The survos will move until their current angle matches these values

// WARNING: Only Use Values Between 1 and 253 For Motor Percentage To Keep Motor in Bounds

// Left Motor: Pin 9 (PA07)
// Right Motor: Pin 8 (PA06)
// Servo 1: Pin 4 (PA08)
// Servo 2: Pin 3 (PA09)
// Servo 3: Pin 10 (PA18)
// Servo 4: Pin 12 (PA19)

float theta = 0;

void setup() {

    // Set GCLK5's prescaler
    GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                       GCLK_GENDIV_ID(5);   // Select Generic Clock (GCLK) 5
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Configure GCLK5 to use DFLL48M
    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |         // Set the duty cycle to 50/50 HIGH/LOW
                        GCLK_GENCTRL_GENEN |       // Enable GCLK5
                        GCLK_GENCTRL_SRC_DFLL48M | // Set the 48MHz clock source
                        GCLK_GENCTRL_ID(5);        // Select GCLK5
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Connect GCLK5 to TCC0, TCC1
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |     // Enable GCLK5 to TCC0 and TCC1
                        GCLK_CLKCTRL_GEN_GCLK5 | // Select GCLK5
                        GCLK_CLKCTRL_ID_TCC0_TCC1; // Feed the GCLK5 to TCC0 and TCC1
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Set prescaler TCCDiv for TCC1
    TCC1->CTRLA.reg |= TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); //set prescalar to divide by 1

    //Use Normal PWM
    TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC1->SYNCBUSY.bit.WAVE) {};

    //The PER register determines the period of the PWM
    //uint32_t period = 960000; //48000000/2400 = 20KHz

    TCC1->PER.reg = MOTOR_WAVELENGTH;  //this is a 24-bit register
    while (TCC1->SYNCBUSY.bit.PER) {};

    ///////////////////////////////
    //Left Motor

    //Set the duty cycle to controller speeds
    TCC1->CC[1].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_1); //this set the on time of the PWM cycle based on the motor_percentage set by the controller
    while (TCC1->SYNCBUSY.bit.CC1) {};

    //set PA07 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA07; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA07; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[7].reg |= PORT_PINCFG_PMUXEN;

    // Set PA07's function to function E. Function E is TCC1/WO[1] for PA07.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 3.
    PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXO_E;

    //set motor dir PA16 to output
    //PORT->Group[0].DIRSET.reg = PORT_PA16; //set the direction to output
    //PORT->Group[0].OUTCLR.reg = PORT_PA16; //set the value to output LOW

    ///////////////////////////////
    //Right Motor
    //Set the duty cycle to controller speeds
    TCC1->CC[0].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_2); //this set the on time of the PWM cycle based on the motor_percentage set by the controller
    while (TCC1->SYNCBUSY.bit.CC0) {};

    //set PA06 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA06; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA06; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[6].reg |= PORT_PINCFG_PMUXEN;

    // Set PA06's function to function E. Function E is TCC1/WO[0] for PA06.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 3.
    PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXE_E;

    //set motor dir PA18 to output
    //PORT->Group[0].DIRSET.reg = PORT_PA18; //set the direction to output
    //PORT->Group[0].OUTCLR.reg = PORT_PA18; //set the value to output LOW

    ///////////////////////////////
    //Enable TCC1
    TCC1->CTRLA.reg |= (TCC_CTRLA_ENABLE);
    while (TCC1->SYNCBUSY.bit.ENABLE) {};


    //////////////////////  
    // Set Up Servos Timer

    // Set prescaler TCCDiv for TCC0
    TCC0->CTRLA.reg |= TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); //set prescalar to divide by 1

    //Use Normal PWM
    TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC1->SYNCBUSY.bit.WAVE) {};

    //The PER register determines the period of the PWM
    //uint32_t period = 960000; //48000000/2400 = 20KHz

    TCC0->PER.reg = MOTOR_WAVELENGTH;  //this is a 24-bit register
    while (TCC1->SYNCBUSY.bit.PER) {};

    // Servo 1

    //set PA08 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA08; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA08; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[8].reg |= PORT_PINCFG_PMUXEN;

    // Set PA08's function to function E. Function E is TCC0/WO[0] for PA08.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 4.
    PORT->Group[0].PMUX[4].reg |= PORT_PMUX_PMUXE_E;

    // Servo 2

    //set PA09 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA09; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA09; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[9].reg |= PORT_PINCFG_PMUXEN;

    // Set PA09's function to function E. Function E is TCC0/WO[1] for PA09.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 4.
    PORT->Group[0].PMUX[4].reg |= PORT_PMUX_PMUXO_E;

    // Servo 3

    // Set PA18 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA18; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA18; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[18].reg |= PORT_PINCFG_PMUXEN;

    // Set PA18's function to function F. Function F is TCC0/WO[2] for PA18.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 9.
    PORT->Group[0].PMUX[9].reg |= PORT_PMUX_PMUXE_F;

    // Servo 4

    // Set PA19 to output
    PORT->Group[0].DIRSET.reg |= PORT_PA19; //set the direction to output
    PORT->Group[0].OUTCLR.reg |= PORT_PA19; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[19].reg |= PORT_PINCFG_PMUXEN;

    // Set PA19's function to function F. Function F is TCC0/WO[3] for PA19.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 9.
    PORT->Group[0].PMUX[9].reg |= PORT_PMUX_PMUXO_F;

    // Set Waveform Output for All Servos
    for (int i = 0; i < 4; i++)
    {
        //Set the duty cycle to controller speeds
        TCC0->CC[i].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * servo_percentages[i]); //this set the on time of the PWM cycle based on the motor_percentage set by the controller
        sync_servos(i);
    }

    //Enable TCC0
    TCC0->CTRLA.reg |= (TCC_CTRLA_ENABLE);
    while (TCC0->SYNCBUSY.bit.ENABLE) {}; 
}

// Wait for Servo Wavelengths to Sync
void sync_servos(unsigned short servo_number)
{
    switch (servo_number)
    {
    case 0:
    {
        while (TCC0->SYNCBUSY.bit.CC0) {};
        break;
    }
    case 1:
    {
        while (TCC0->SYNCBUSY.bit.CC1) {};
        break;
    }
    case 2:
    {
        while (TCC0->SYNCBUSY.bit.CC2) {};
        break;
    }
    case 3:
    {
        while (TCC0->SYNCBUSY.bit.CC3) {};
        break;
    }
    }
}

void loop() 
{
    // put your main code here, to run repeatedly:
    theta += 0.0002f;
    motor_percentage_1 = 127 + int(127 * sin(theta));
    motor_percentage_2 = 127 + int(127 * sin(theta + 3.14));

    TCC1->CC[0].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_2); //this sets the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC0) {};

    TCC1->CC[1].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_1); //this set the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC1) {};

    for (int i = 0; i < 4; i++)
    {
        // Change Percentage
        servo_percentages[i] = 127 + int(127 * sin(theta));

        // Update Motor
        TCC0->CC[i].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * servo_percentages[i]); //this sets the on time of the PWM cycle
        sync_servos(i);
    }
}

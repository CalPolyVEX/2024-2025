const uint32_t MOTOR_WAVELENGTH = 960000; // wavelength from peak to peak in Hz
const float MOTOR_BASE_TIME = 48000;      // value in Hz of high length when speed = 100% backwards
const float MOTOR_CHANGE_CONST = 189;     // multiplied by percentage to get change in range of wavelength needed
unsigned int motor_percentage_1 = 127;    // range is 0 (100% backward) to 127 (0% speed) to 254 (100% forward) for left motor
unsigned int motor_percentage_2 = 127;    // range is 0 (100% backward) to 127 (0% speed) to 254 (100% forward) for right motor

// WARNING: Don't go close to 100% only made it 1 to 253 to create something that works

float theta = 0;

void motor_setup()
{
  // Set GCLK5's prescaler
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
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
  TCC1->CTRLA.reg |= TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); // set prescalar to divide by 1

  // Use Normal PWM
  TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC1->SYNCBUSY.bit.WAVE)
  {
  };

  // The PER register determines the period of the PWM
  // uint32_t period = 960000; //48000000/2400 = 20KHz

  TCC1->PER.reg = MOTOR_WAVELENGTH; // this is a 24-bit register
  while (TCC1->SYNCBUSY.bit.PER)
  {
  };

  ///////////////////////////////
  // Left Motor

  // Set the duty cycle to controller speeds
  TCC1->CC[1].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_1); // this set the on time of the PWM cycle based on the motor_percentage set by the controller
  while (TCC1->SYNCBUSY.bit.CC1)
  {
  };

  // set PA07 to output
  PORT->Group[0].DIRSET.reg = PORT_PA07; // set the direction to output
  PORT->Group[0].OUTCLR.reg = PORT_PA07; // set the value to output LOW

  /* Enable the peripheral multiplexer for the pins. */
  PORT->Group[0].PINCFG[7].reg |= PORT_PINCFG_PMUXEN;

  // Set PA07's function to function E. Function E is TCC1/WO[1] for PA07.
  // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
  // index is pin number - 1 / 2, so 3.
  PORT->Group[0].PMUX[3].reg = PORT_PMUX_PMUXO_E;

  // set motor dir PA16 to output
  PORT->Group[0].DIRSET.reg = PORT_PA16; // set the direction to output
  PORT->Group[0].OUTCLR.reg = PORT_PA16; // set the value to output LOW

  ///////////////////////////////
  // Right Motor
  // Set the duty cycle to controller speeds
  TCC1->CC[0].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * motor_percentage_2); // this set the on time of the PWM cycle based on the motor_percentage set by the controller
  while (TCC1->SYNCBUSY.bit.CC0)
  {
  };

  // set PA06 to output
  PORT->Group[0].DIRSET.reg = PORT_PA06; // set the direction to output
  PORT->Group[0].OUTCLR.reg = PORT_PA06; // set the value to output LOW

  /* Enable the peripheral multiplexer for the pins. */
  PORT->Group[0].PINCFG[6].reg |= PORT_PINCFG_PMUXEN;

  // Set PA06's function to function E. Function E is TCC1/WO[0] for PA06.
  // Because this is an even numbered pin the PMUX is E (even) and the PMUX
  // index is pin number / 2, so 3.
  PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXE_E;

  // set motor dir PA18 to output
  PORT->Group[0].DIRSET.reg = PORT_PA18; // set the direction to output
  PORT->Group[0].OUTCLR.reg = PORT_PA18; // set the value to output LOW

  ///////////////////////////////
  // Enable TCC1
  TCC1->CTRLA.reg |= (TCC_CTRLA_ENABLE);
  while (TCC1->SYNCBUSY.bit.ENABLE)
  {
  };
}

// Changes one of the motors based on if it is left or right motor and the input speed
void change_motor_speed(unsigned short motor_num, int speed)
{
  if (motor_num == 0)
  {
    TCC1->CC[0].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * speed); // this set the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC0)
    {
    };
  }
  else
  {
    TCC1->CC[1].reg = MOTOR_BASE_TIME + (MOTOR_CHANGE_CONST * speed); // this set the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC1)
    {
    };
  }
}

void set_servo_angle(unsigned short servo_num, int position)
{
}

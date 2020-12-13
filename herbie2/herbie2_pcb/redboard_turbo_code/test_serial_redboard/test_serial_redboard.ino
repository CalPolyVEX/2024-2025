#include <Servo.h>
#include <Wire.h>
#include <TCA9534.h>

TCA9534 ioex;
const uint8_t IOEX_ADDR = 0x38; // A0 = A1 = A2 = 0

//#include "SparkFun_TCA9534.h"
// #include <SAMD21turboPWM.h>

Servo myservo;
//TCA9534 myGPIO;
// TurboPWM pwm; //FIXME:  the turboPWM library uses GCLK4 which is used by Servo.h

void setup_interrupt() 
{
  myservo.attach(9);  //the servo library already initializes TC5 to use the 48MHz clock

  // Set up the generic clock (GCLK4) used to clock timers
  // GCLK->GENDIV.reg = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
  //                    GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  // while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization

  // GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
  //                     GCLK_GENCTRL_GENEN |         // Enable GCLK4
  //                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
  //                     GCLK_GENCTRL_ID(4);          // Select GCLK4
  // while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization

  // Feed GCLK4 to TC4 and TC5
  // GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
  //                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
  //                     GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK4 to TC4 and TC5
  // while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization
 
  REG_TC5_CTRLA |= TC_CTRLA_MODE_COUNT16;           // Set the counter to 16-bit mode
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  REG_TC5_COUNT16_CC0 = 25000;                      // Set the TC5 CC0 register to some arbitary value
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  //NVIC_DisableIRQ(TC4_IRQn);
  //NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);                    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC5 to 0 (highest)
  NVIC_EnableIRQ(TC5_IRQn);                         // Connect TC5 to Nested Vector Interrupt Controller (NVIC)

  REG_TC5_INTFLAG |= TC_INTFLAG_MC0;                // Clear the interrupt flags
  REG_TC5_INTENSET = TC_INTENSET_MC0;               // Enable TC5 interrupts
 
  REG_TC5_CTRLA |= TC_CTRLA_PRESCALER_DIV64 |     // Set prescaler to 64, 48MHz/64 = 750kHz
                   TC_CTRLA_ENABLE;               // Enable TC5
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);        // Wait for synchronization
}

unsigned long c = 0;
int f = 0;

void setup_i2c() 
{
  Wire.begin();
  ioex.attach(Wire);
  ioex.setDeviceAddress(IOEX_ADDR); // A0 = A1 = A2 = 0
  ioex.config(TCA9534::Config::IN); // set all port to input
}

void setup() 
{
  delay(4000); //wait 4 seconds

  setup_i2c();
  setup_interrupt();
  // init_encoders();

  SerialUSB.begin(115200); // Initialize Serial Monitor USB

  //while (!SerialUSB); // Wait for Serial monitor to open

  SerialUSB.println("Send character(s) to relay it over Serial1");


  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  char buf[64];
  int packet_length;

  packet_length = SerialUSB.available();
  if (packet_length >= 5) {
    SerialUSB.readBytes(buf, packet_length);

    // Print a message stating what we're sending:
    SerialUSB.print("Remaining bytes: ");
    SerialUSB.print(SerialUSB.available());
    SerialUSB.print(" ");
    SerialUSB.print(c);
    SerialUSB.print(" ");
    SerialUSB.print(f);
    SerialUSB.println(" ");

    // uint8_t raw = ioex.input();
    // SerialUSB.println(raw, BIN);
  }

  while(SerialUSB.available()){SerialUSB.read();}  //clear out the serial input buffer
  delay(1); //wait 1ms, otherwise the serial USB port will lock up
}

void TC5_Handler()  // Interrupt Service Routine (ISR) for timer TC5
{     
  static int count = 0;

  // Check for match counter 0 (MC0) interrupt
  if (TC5->COUNT16.INTFLAG.bit.MC0)             
  {
    // Read the encoders here
    // ...

    c++;
   
    REG_TC5_INTFLAG = TC_INTFLAG_MC0;         // Clear the MC0 interrupt flag
    TC5->COUNT16.COUNT.reg = 0; 
  }

  if (c % 50 == 0) {
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  } else {
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  }
}

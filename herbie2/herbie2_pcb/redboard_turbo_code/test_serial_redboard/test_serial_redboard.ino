#include <Servo.h>
#include <SPI.h>

Servo myservo;

void setup_interrupt() 
{
  myservo.attach(9);  //the servo library already initialize TC5 to use the 48MHz clock

   // Set up the generic clock (GCLK4) used to clock timers
  // GCLK->GENDIV.reg = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
  //                    GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  // while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
  //                     GCLK_GENCTRL_GENEN |         // Enable GCLK4
  //                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
  //                     GCLK_GENCTRL_ID(4);          // Select GCLK4
  // while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Feed GCLK4 to TC4 and TC5
  // GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
  //                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
  //                     GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK4 to TC4 and TC5
  // while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization
 
  REG_TC5_CTRLA |= TC_CTRLA_MODE_COUNT16;           // Set the counter to 8-bit mode
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);        // Wait for synchronization

  REG_TC5_COUNT16_CC0 = 25000;                      // Set the TC4 CC0 register to some arbitary value
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);        // Wait for synchronization

  //NVIC_DisableIRQ(TC4_IRQn);
  //NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_SetPriority(TC5_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC5 to 0 (highest)
  NVIC_EnableIRQ(TC5_IRQn);         // Connect TC5 to Nested Vector Interrupt Controller (NVIC)

  REG_TC5_INTFLAG |= TC_INTFLAG_MC0;        // Clear the interrupt flags
  REG_TC5_INTENSET = TC_INTENSET_MC0;     // Enable TC5 interrupts
 
  REG_TC5_CTRLA |= TC_CTRLA_PRESCALER_DIV64 |     // Set prescaler to 64, 48MHz/64 = 750kHz
                   TC_CTRLA_ENABLE;               // Enable TC5
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);        // Wait for synchronization
}

unsigned long c = 0;

void setup() 
{
  delay(4000); //wait 4 seconds

  setup_interrupt();

  SerialUSB.begin(115200); // Initialize Serial Monitor USB

  while (!SerialUSB) ; // Wait for Serial monitor to open

  SerialUSB.println("Send character(s) to relay it over Serial1");
}

void loop()
{
  if (c%30 == 0) 
  {
    // Print a message stating what we're sending:
    SerialUSB.print("Sending ");
    SerialUSB.print(c);
    SerialUSB.println(" to Serial1");
  }
}

void TC5_Handler()  // Interrupt Service Routine (ISR) for timer TC5
{     
  // Check for match counter 0 (MC0) interrupt
  if (TC5->COUNT16.INTFLAG.bit.MC0 && TC5->COUNT16.INTENSET.bit.MC0)             
  {
    // Put your counter compare 0 (CC0) code here:
    // ...
    c++;
   
    REG_TC5_INTFLAG = TC_INTFLAG_MC0;         // Clear the MC0 interrupt flag
    TC5->COUNT16.COUNT.reg = 0; 
  }
}

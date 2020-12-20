#include <Wire.h>
#include <TCA9534.h>

#define RCLK_SERVO 5
#define CLK_SERVO 6
#define DATA_SERVO 7

#define PIN_RCLK_SERVO PORT_PA15
#define PIN_CLK_SERVO PORT_PA20
#define PIN_DATA_SERVO PORT_PA21

TCA9534 ioex;
const uint8_t IOEX_ADDR = 0x20; // A0 = A1 = A2 = 0

#define NUM_SERVOS 4
unsigned short servo_on[NUM_SERVOS];

///////////////////////////////
//Timer usage:
//
//TC4 - servos
//TC5 - encoder reading (30Hz)
//TCC1 - motors
//
//GCLK5 - motors
//GCLK6 - servos and encoders

void init_servo() {
  //The servo system uses Timer 4
  /////////////////////////////////Timer 4
  TC4->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;     // Set the counter to 16-bit mode
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);            // Wait for synchronization

  REG_TC4_COUNT16_CC0 = 500;                           // Each timer tick is 1uS, set to 500uS
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);            // Wait for synchronization

  //NVIC_DisableIRQ(TC4_IRQn);
  //NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_SetPriority(TC4_IRQn, 0);                       // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
  NVIC_EnableIRQ(TC4_IRQn);                            // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

  REG_TC4_INTFLAG |= TC_INTFLAG_MC0;                   // Clear the interrupt flags
  TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;         // Enable TC4 interrupts
 
  TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV16 | // Set prescaler to 16, 16MHz/16 = 1MHz
                            TC_CTRLA_ENABLE;           // Enable TC5
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);            // Wait for synchronization

  //configure the pin directions
  pinMode(RCLK_SERVO,OUTPUT);
  pinMode(CLK_SERVO,OUTPUT);
  pinMode(DATA_SERVO,OUTPUT);

  digitalWrite(RCLK_SERVO,LOW);
  digitalWrite(CLK_SERVO,LOW);
  digitalWrite(DATA_SERVO,LOW);
}

void setup_interrupt() 
{
  // Set up the generic clock (GCLK6) used to clock timers
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
                     GCLK_GENDIV_ID(6);            // Select Generic Clock (GCLK) 6
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization

  GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN |         // Enable GCLK6
                      GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                      GCLK_GENCTRL_ID(6);          // Select GCLK6
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization

  // Feed GCLK4 to TC4 and TC5
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |         // Enable GCLK6 to TC4 and TC5
                      GCLK_CLKCTRL_GEN_GCLK6 |     // Select GCLK6
                      GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK6 to TC4 and TC5
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization
 
  /////////////////////////////////Timer 5
  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;  // Set the counter to 16-bit mode
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV64 | // Set prescaler to 64, 16MHz/64 = 250kHz
                            TC_CTRLA_ENABLE;           // Enable TC5
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  REG_TC5_COUNT16_CC0 = 8333;                       // 8333 * (1/250KHz) = .0333s  (30Hz)
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  //NVIC_DisableIRQ(TC4_IRQn);
  //NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_SetPriority(TC5_IRQn, 1);                    // Set the NVIC priority for TC5 to 1 (second highest priority)
  NVIC_EnableIRQ(TC5_IRQn);                         // Connect TC5 to Nested Vector Interrupt Controller (NVIC)

  REG_TC5_INTFLAG |= TC_INTFLAG_MC0;                // Clear the match compare interrupt flag
  TC5->COUNT16.INTENSET.reg = TC_INTENSET_MC0;      // Enable TC5 match compare interrupt
 
  init_servo();
  init_encoders();
}

unsigned long c = 0;
unsigned long d = 0;
int f = 0;
int ser_write = 0;

void setup_i2c() 
{
  Wire.begin();
  Wire.setClock(400000); //set clock to 400KHz
  ioex.attach(Wire);
  ioex.setDeviceAddress(IOEX_ADDR); // A0 = A1 = A2 = 0
  ioex.config(TCA9534::Config::IN); // set all port to input
  ioex.config(1, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(1, TCA9534::Level::H); // turn off LED3
}

void setup() 
{
  delay(4000); //wait 4 seconds
  SerialUSB.begin(115200); // Initialize Serial Monitor USB

  setup_interrupt();
  lcdInit();
  init_motors();
  setup_i2c();

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
    SerialUSB.print(" ");
    // SerialUSB.print(getChanEncoderValue(1));
    SerialUSB.print(readMDR1(1));
    SerialUSB.println(" ");

    if (buf[0] == 'a') {
      ser_write = 1;
      ioex.output(1, TCA9534::Level::L);
    } else {
      ser_write = 0;
      ioex.output(1, TCA9534::Level::H);
    }
    // uint8_t raw = ioex.input();
    // SerialUSB.println(raw, BIN);
    while (SerialUSB.available())
    {
      SerialUSB.read();
    } //clear out the serial input buffer
  }
  // SerialUSB.println(packet_length);
  while (SerialUSB.available())
  {
    SerialUSB.read();
  } //clear out the serial input buffer

  delay(1); //wait 1ms, otherwise the serial USB port will lock up
  // delayMicroseconds(250); //wait 1ms, otherwise the serial USB port will lock up
}

void TC5_Handler()  // Encoder (ISR) for timer TC5
{
  // Read the encoders here
  // ...

  c++;

  REG_TC5_INTFLAG = TC_INTFLAG_MC0; // Clear the MC0 interrupt flag
  TC5->COUNT16.COUNT.reg = 0;

  if (c % 50 == 0)
  {
    //digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  }
}

void clear_servo_register() 
{
  for (int i=0; i<3; i++)
  {
    REG_PORT_OUTCLR0 = PIN_DATA_SERVO; //set data pin low

    //toggle the clock
    REG_PORT_OUTSET0 = PIN_CLK_SERVO; //set data pin high
    delayMicroseconds(1);
    REG_PORT_OUTCLR0 = PIN_CLK_SERVO; //set data pin low
    delayMicroseconds(1);
  }
}

void TC4_Handler()  // Servo ISR for timer TC4
{
  static volatile int state = 0;
  // Adjust servos here
  // ...

  d++;

  char data = 0;
  char mask = 0;

  clear_servo_register();

  state++;

  REG_TC4_INTFLAG = TC_INTFLAG_MC0; // Clear the MC0 interrupt flag
  TC4->COUNT16.COUNT.reg = 0;

  if (d % 50 == 0)
  {
    if (ser_write == 1)
    {
      SerialUSB.println(d);
    }
  }
  else
  {
  }
}
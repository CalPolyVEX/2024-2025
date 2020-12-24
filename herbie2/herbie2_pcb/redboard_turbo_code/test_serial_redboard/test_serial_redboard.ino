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
extern const unsigned short crctable[256];

#define NUM_SERVOS 2
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
 
  TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV8 | // Set prescaler to 16, 16MHz/16 = 1MHz
                            TC_CTRLA_ENABLE;           // Enable TC4
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY);            // Wait for synchronization

  //configure the pin directions
  pinMode(RCLK_SERVO,OUTPUT);
  pinMode(CLK_SERVO,OUTPUT);
  pinMode(DATA_SERVO,OUTPUT);

  digitalWrite(RCLK_SERVO,LOW);
  digitalWrite(CLK_SERVO,LOW);
  digitalWrite(DATA_SERVO,LOW);

  for (int i=0; i<NUM_SERVOS; i++) {
    servo_on[i] = 3000;
  }

  TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;         // Enable TC4 interrupts
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

  NVIC_SetPriority(TC5_IRQn, 2);                    // Set the NVIC priority for TC5 to 1 (second highest priority)
  NVIC_EnableIRQ(TC5_IRQn);                         // Connect TC5 to Nested Vector Interrupt Controller (NVIC)

  REG_TC5_INTFLAG |= TC_INTFLAG_MC0;                // Clear the match compare interrupt flag
 
  // init_servo();
  init_encoders();

  //enable the encoder interrupt after initializing the 7366 chips
  TC5->COUNT16.INTENSET.reg = TC_INTENSET_MC0;      // Enable TC5 match compare interrupt
}

void setup_i2c() 
{
  Wire.begin();
  Wire.setClock(400000); //set clock to 400KHz
  ioex.attach(Wire);
  ioex.setDeviceAddress(IOEX_ADDR); // A0 = A1 = A2 = 0
  ioex.config(TCA9534::Config::IN); // set all pins to input

  ioex.config(1, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(1, TCA9534::Level::H); // turn off LED3

  ioex.config(0, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(0, TCA9534::Level::H); // turn off LED3

  ioex.config(7, TCA9534::Config::OUT); // set backlight control pin
}

void setup() 
{
  setup_i2c();
  setup_interrupt();
  lcdInit();
  init_motors();
  init_servo_test();

  set_motor_speed(0,0);
  set_motor_speed(1,0);

  backlight_on();

  delay(4000); //wait 4 seconds
  SerialUSB.begin(115200); // Initialize Serial Monitor USB

  SerialUSB.println("Send character(s) to relay it over Serial1");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

int motor_stop_timeout = 0;
char buf[64];

void loop()
{
  int packet_length;

  while(1) 
  {
    motor_stop_timeout++;
    packet_length = SerialUSB.available();
    if (packet_length >= 8) //the minimum packet length is 8 bytes
      break;

    if (motor_stop_timeout > 1000) { //if no command received in 1 second, then stop motors
      set_motor_speed(0, 0);
      set_motor_speed(1, 0);

      if (motor_stop_timeout == 1001) {
        lcdClear();
        lcdPrintf("timeout");
      }
    }

    delay(1); //delay 1 ms
  }

  if (packet_length >= 8) 
  {
    SerialUSB.readBytes(buf, packet_length);
    motor_stop_timeout = 0;

    if (buf[1] == 34) //motor command
    {
      //lcdSetCursor(0, 0);

      if (check_crc(buf, 8) == 1) //CRC is correct
      {
        short left_speed = (buf[2] << 8) | buf[3];  //speed is sent high byte first
        short right_speed = (buf[4] << 8) | buf[5];

        set_motor_speed(0, left_speed); //the speed should be between -2400 and +2400
        set_motor_speed(1, right_speed);

        //lcdPrintf("%d %d", left_speed, right_speed);
      }
      else
      { //bad CRC
        lcdClear();
        lcdSetCursor(0, 0);
        lcdPrintf("bad CRC");
        led_on(2);
      }
    }
    else if (buf[1] == 0)
    {
      //clear screen
      if (check_crc(buf, 8) == 1) //CRC is correct
        lcdClear();
    }
    else if (buf[1] == 1)
    {
      //set cursor
      //buf[2] = col (0-15)
      //buf[3] = row (0-1)
      if (check_crc(buf, 8) == 1) //CRC is correct
        lcdSetCursor(buf[2], buf[3]);
    }
    else if (buf[1] == 2)
    {
      //print string
      //buf[2] = the length of the string in bytes
      //buf[3-x] = the string data
      if (check_crc(buf, packet_length) == 1)
      { //CRC is correct
        char str[32];

        for (int i = 0; i < buf[2]; i++)
        {
          str[i] = buf[3 + i];
        }
        str[buf[2]] = 0; //set the end of the string to NULL

        lcdSetCursor(0, 1);
        lcdPrintf("%s", str);
      }
    }
    else if (buf[1] == 3)
    {
      //print int
      //buf[2-5] = the 4 bytes of the int, sent least significant byte first
      if (check_crc(buf, 8) == 1)  //CRC is correct
      { 
        int num = ((int)buf[5] << 24) | ((int)buf[4] << 16) | ((int)buf[3] << 8) | ((int)buf[2]);
        lcdSetCursor(0, 1);
        lcdPrintf("%d", num);
      }
    }
    else if (buf[1] == 4)
    {
      //backlight off
      if (check_crc(buf, 8) == 1)  //CRC is correct
        backlight_off();
    }
    else if (buf[1] == 5)
    {
      //backlight on
      if (check_crc(buf, 8) == 1)  //CRC is correct
        backlight_on();
    }
    else if (buf[1] == 6)
    {
      //set servo position
      //buf[2] = servo number (0-5)
      //buf[3-4] = position (0-255)
      set_servo(buf[2], ((unsigned short)buf[3] << 8) | (unsigned short)buf[4]);
    }
    else if (buf[1] == 7)
    {
      //LED on
      //buf[2] = led number 
      if (check_crc(buf, 8) == 1) {  //CRC is correct
        led_on(buf[2]);
      }
    }
    else if (buf[1] == 8)
    {
      //LED off
      //buf[2] = led number 
      if (check_crc(buf, 8) == 1) {  //CRC is correct
        led_off(buf[2]);
      }
    }
  }

  while (SerialUSB.available())  //clear out the serial input buffer
  {
    SerialUSB.read();
  } 

  // delay(1); //wait 1ms, otherwise the serial USB port will lock up
  delayMicroseconds(250); //wait 1ms, otherwise the serial USB port will lock up
}

void TC5_Handler()  // Encoder (ISR) for timer TC5
{
  REG_TC5_INTFLAG = TC_INTFLAG_MC0; // Clear the MC0 interrupt flag
  TC5->COUNT16.COUNT.reg = 0;

  unsigned char buf[13];
  unsigned short crc = 0;
  unsigned long left_enc_count = getChanEncoderValue(2); 
  unsigned long right_enc_count = getChanEncoderValue(1);

  buf[0] = 0xff;

  buf[1] = left_enc_count & 0xff;  //send least significant byte first
  left_enc_count = left_enc_count >> 8;
  buf[2] = left_enc_count & 0xff;
  left_enc_count = left_enc_count >> 8;
  buf[3] = left_enc_count & 0xff;
  left_enc_count = left_enc_count >> 8;
  buf[4] = left_enc_count & 0xff;

  buf[5] = right_enc_count & 0xff;
  right_enc_count = right_enc_count >> 8;
  buf[6] = right_enc_count & 0xff;
  right_enc_count = right_enc_count >> 8;
  buf[7] = right_enc_count & 0xff;
  right_enc_count = right_enc_count >> 8;
  buf[8] = right_enc_count & 0xff;

  buf[9] = 0xff; //send 2 extra bytes for future use
  buf[10] = 0xff;

  for (int byte = 0; byte < 11; byte++) //compute the CRC
  {
    crc = (crc << 8) ^ crctable[((crc >> 8) ^ buf[byte])];
  }

  buf[11] = (crc >> 8) & 0xFF; //send the high byte of the crc
  buf[12] = crc & 0xFF;        //send the low byte of the crc

  SerialUSB.write(buf,13);  //send the data
}

void TC4_Handler()  // Servo ISR for timer TC4
{
  static volatile int state = NUM_SERVOS;
  static volatile int total_on_time = 10000;

  REG_TC4_INTFLAG = TC_INTFLAG_MC0; // Clear the MC0 interrupt flag

  if (state == NUM_SERVOS) 
  {
    REG_PORT_OUTSET0 = PIN_CLK_SERVO;    //clock the shift register
    REG_PORT_OUTCLR0 = PIN_CLK_SERVO;    //clock the shift register

    REG_PORT_OUTSET0 = PIN_RCLK_SERVO;    //clock the output latch
    REG_PORT_OUTCLR0 = PIN_RCLK_SERVO;    //clock the output latch

    REG_TC4_COUNT16_CC0 += 40000 - total_on_time; // 20ms - total_on_time
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY); // Wait for synchronization

    state = 0;
    total_on_time = 0;
    REG_PORT_OUTSET0 = PIN_DATA_SERVO; //set the data input to high
  } 
  else 
  {
    REG_PORT_OUTSET0 = PIN_CLK_SERVO;    //clock the shift register
    REG_PORT_OUTCLR0 = PIN_CLK_SERVO;    //clock the shift register

    REG_PORT_OUTSET0 = PIN_RCLK_SERVO;    //clock the output latch
    REG_PORT_OUTCLR0 = PIN_RCLK_SERVO;    //clock the output latch

    REG_TC4_COUNT16_CC0 += servo_on[state]; // on time for the servo
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY); // Wait for synchronization

    total_on_time += servo_on[state];

    if (state == 0)
      REG_PORT_OUTCLR0 = PIN_DATA_SERVO; //set the data input to low

    state++;
  }
}

void set_servo_old(int num, int position) {
  TC4->COUNT16.INTENCLR.reg = TC_INTENSET_MC0;         // Disable TC4 interrupts
  while(TC4->COUNT16.STATUS.bit.SYNCBUSY);

  servo_on[num] = position;

  TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;         // Enable TC4 interrupts
  while(TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

void set_servo(int num, int position) {
  //the position can run from 0-2000
  if (num == 0)
  {
    TCC0->CC[3].reg = 48000 + 24*position; 
  }
  else if (num == 1)
  {
    TCC0->CC[1].reg = 48000 + 24*position;
  }
}
void backlight_on() {
  ioex.output(7, TCA9534::Level::H);
}

void backlight_off() {
  ioex.output(7, TCA9534::Level::L);
}

void led_on(int num) {
  if (num == 2)
    ioex.output(0, TCA9534::Level::L);
  else if (num == 3)
    ioex.output(1, TCA9534::Level::L);
}

void led_off(int num) {
  if (num == 2)
    ioex.output(0, TCA9534::Level::H);
  else if (num == 3)
    ioex.output(1, TCA9534::Level::H);
}
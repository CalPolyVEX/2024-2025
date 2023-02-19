#include <Wire.h>
#include <TCA9534.h>

TCA9534 ioex;
const uint8_t IOEX_ADDR = 0x20; // A0 = A1 = A2 = 0
extern const unsigned short crctable[256];

///////////////////////////////
//Timer usage:
//
//TC5 - encoder reading (30Hz)
//TCC0 - servos
//TCC1 - motors
//
//GCLK5 - motors and servos
//GCLK6 - encoders and SERCOM0 UART

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

  // Feed GCLK6 to TC4 and TC5
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

  //REG_TC5_COUNT16_CC0 = 8333;                       // 8333 * (1/250KHz) = .0333s  (30Hz)
  REG_TC5_COUNT16_CC0 = 5000;                       // 5000 * (1/250KHz) = .020s  (50Hz)
  
  while (TC5->COUNT16.STATUS.bit.SYNCBUSY);         // Wait for synchronization

  NVIC_SetPriority(TC5_IRQn, 2);                    // Set the NVIC priority for TC5 to 1 (second highest priority)
  NVIC_EnableIRQ(TC5_IRQn);                         // Connect TC5 to Nested Vector Interrupt Controller (NVIC)

  REG_TC5_INTFLAG |= TC_INTFLAG_MC0;                // Clear the match compare interrupt flag
 
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

  ioex.config(3, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(3, TCA9534::Level::H); // turn off LED1
  
  ioex.config(2, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(2, TCA9534::Level::H); // turn off LED2
  
  ioex.config(1, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(1, TCA9534::Level::H); // turn off LED3

  ioex.config(0, TCA9534::Config::OUT); // change port P1 to output
  ioex.output(0, TCA9534::Level::H); // turn off LED4

  ioex.config(7, TCA9534::Config::OUT); // set backlight control pin
}

void setup() 
{
  setup_i2c();
  setup_interrupt();
  lcdInit();
  init_motors();
  init_servo2();
  
  set_motor_speed(0,0);
  set_motor_speed(1,0);

  backlight_on();

  delay(200); //wait .2 seconds
  SerialUSB.begin(921600); // Initialize Serial Monitor USB
  SerialUSB.setTimeout(300); //timeout to 300ms

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  init_lb_uart(); //initialize the SmartPort TX UART on SERCOM0
}

char buf[512];
int motor_stop_timeout = 0;

void flush_serial() {
  while (SerialUSB.available())  //clear out the serial input buffer
  {
    SerialUSB.read();
  } 
}

void reset_timeout() {
  motor_stop_timeout = 0;
  led_off(3);
}

void loop()
{
  int packet_length;
  int num_bytes_read;
  int serial_bytes_available = 0;

  while(1) 
  {
    motor_stop_timeout++;
    serial_bytes_available = SerialUSB.available();

    if (serial_bytes_available != 0) {
      num_bytes_read = SerialUSB.readBytes((char*)buf, 1); //read address

      if (buf[0] == 128) { //valid address
        num_bytes_read = SerialUSB.readBytes((char*)(buf+1), 1); //read the command byte

        switch (buf[1]) {
          case 34:  //motor command
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6);

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
                short left_speed = (buf[2] << 8) | buf[3];  //speed is sent high byte first
                short right_speed = (buf[4] << 8) | buf[5];

                set_motor_speed(0, left_speed); //the speed should be between -2400 and +2400
                set_motor_speed(1, right_speed);
                reset_timeout();
                continue;
            } else {  //bad packet
              flush_serial(); 
            }
        
            break;
          case 0:  //clear screen
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              lcdClear();
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 1:  //set_cursor command
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              lcdSetCursor(buf[2], buf[3]);
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 2:  //print string
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 1);  //read string size
            num_bytes_read = SerialUSB.readBytes((char*)(buf+3), buf[2]+2); 

            if ((num_bytes_read == (buf[2]+2)) && (check_crc(buf, (buf[2] + 5)) == 1)) {  //CRC is correct
              buf[buf[2] + 3] = 0;  //set the end of the string to NULL

              lcdPrintf("%s", buf+3);
              led_on(2);
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 3:  //print integer
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              int* num_ptr = (int*) &(buf[2]);
              //int num = ((int)buf[5] << 24) | ((int)buf[4] << 16) | ((int)buf[3] << 8) | ((int)buf[2]);
              
              int num = __builtin_bswap32(*num_ptr);  //change byte order
              
              lcdPrintf("%d", num);
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }
            
            break;
          case 4:  //backlight off
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              backlight_off();
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 5:  //backlight on
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              backlight_off();
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 6:  //set servo
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              set_servo(buf[2], ((unsigned short)buf[3] << 8) | (unsigned short)buf[4]);
              //set_servo(buf[2], *((unsigned short*) &(buf[3])));

              reset_timeout();
              continue;
            } else {  // bad packet
              flush_serial();
            }

            break;
          case 7:  //LED on
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              led_on(buf[2]);
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          case 8:  //LED off
            num_bytes_read = SerialUSB.readBytes((char*)(buf+2), 6); 

            if ((num_bytes_read == 6) && (check_crc(buf, 8) == 1)) {
              led_off(buf[2]);
              reset_timeout();
              continue;
            } else {  //bad packet
              flush_serial();
            }

            break;
          default:
            flush_serial();
            break;
         }
      
      } else { //bad address received
        //flush serial buffer
        flush_serial();
      }
    } else {
      if (motor_stop_timeout == 4000) { //if no command received in 1 second, then stop motors
        set_motor_speed(0, 0);
        set_motor_speed(1, 0);
        
        lcdClear();
        lcdPrintf("--");
        led_on(3);
        led_off(1);
        led_off(2);
        led_off(4);
      }      
    }
    
    delayMicroseconds(250); //wait 250us, otherwise the serial USB port will lock up
  }
}

void TC5_Handler()  // Encoder (ISR) for timer TC5
{
  REG_TC5_INTFLAG = TC_INTFLAG_MC0; // Clear the MC0 interrupt flag
  TC5->COUNT16.COUNT.reg = 0;

  unsigned char buf[12];
  unsigned char temp_byte;
  unsigned short crc = 0;

  getChanEncoderValue(2,buf);  //fill the buffer with a 4-byte encoder reading
  
  buf[0] = 0xff;       //put the header in the packet
  temp_byte = buf[4];  //store the LSB byte which will be overwritten by getChanEncoderValue()

  getChanEncoderValue(1, buf+4);
  buf[4] = temp_byte;  //restore the LSB byte

  buf[9] = 0xff; //send 1 extra byte for future use

  for (int byte = 0; byte < 10; byte++) //compute the CRC
  {
    crc = (crc << 8) ^ crctable[((crc >> 8) ^ buf[byte])];
  }

  buf[10] = (crc >> 8) & 0xFF; //send the high byte of the crc
  buf[11] = crc & 0xFF;        //send the low byte of the crc

  //*((unsigned short*) &(buf[10])) = __builtin_bswap16(crc);

  SerialUSB.write(buf,12);  //send the data
}

void set_servo(int num, int position) {
  //the position can run from 0-2000
  if (num == 0) {
    TCC0->CC[3].reg = 48000 + 24*position; 
  } else if (num == 1) {
    TCC0->CC[1].reg = 48000 + 24*position;
  } else if (num == 2) {
    TCC0->CC[2].reg = 48000 + 24*position;
  } else if (num == 3) {
    TCC0->CC[0].reg = 48000 + 24*position;
  }
}

void backlight_on() {
  ioex.output(7, TCA9534::Level::H);
}

void backlight_off() {
  ioex.output(7, TCA9534::Level::L);
}

void led_on(int num) {
  if (num == 4)
    ioex.output(0, TCA9534::Level::L);
  else if (num == 3)
    ioex.output(1, TCA9534::Level::L);
  else if (num == 2)
    ioex.output(2, TCA9534::Level::L);
  else if (num == 1)
    ioex.output(3, TCA9534::Level::L);
}

void led_off(int num) {
  if (num == 4)
    ioex.output(0, TCA9534::Level::H);
  else if (num == 3)
    ioex.output(1, TCA9534::Level::H);
  else if (num == 2)
    ioex.output(2, TCA9534::Level::H);
  else if (num == 1)
    ioex.output(3, TCA9534::Level::H);
}

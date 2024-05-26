/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(115200);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(12, LOW);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(12, HIGH);    // turn the LED off by making the voltage LOW
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  Serial.write(97);
}

void init_vex_brain_serial() {
  // Enable the SERCOM5 clock in the PM // Enable the clock for SERCOM5
  MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM5;
  
  // Select the GCLK (Generic Clock) for SERCOM5
  GCLK->PCHCTRL[SERCOM5_GCLK_ID_CORE].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;

  // Configure the TX and RX pins
  // Assuming PB31 as TX and PB30 as RX (these can vary based on your setup)
  // Using PORTB so that is Group 1.
  PORT->Group[1].PINCFG[30].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[30 >> 1].reg |= 3; //PORT_PMUX_PMUXE_D
    
  PORT->Group[1].PINCFG[31].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[31 >> 1].reg |= (3 << 4); //PORT_PMUX_PMUXO_D = number 3 shifted to the left 4 bits
    
  // Reset the SERCOM5
  SERCOM5->USART.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
  while (SERCOM5->USART.CTRLA.bit.SWRST || SERCOM5->USART.SYNCBUSY.bit.SWRST);
    
  // Configure SERCOM5 for UART mode
  //SERCOM5->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE_USART_INT_CLK | // Internal clock
  SERCOM5->USART.CTRLA.reg = 4 | // Internal clock
                             SERCOM_USART_CTRLA_RXPO(1) |            // RX on PAD[1]
                             SERCOM_USART_CTRLA_TXPO(0) |            // TX on PAD[0]
                             SERCOM_USART_CTRLA_DORD;                // LSB first
    
  // Set the baud rate
  uint64_t baud = 115200;
  uint64_t baudValue = 65536 - ((65536 * 16 * baud) / 120000000); // Assuming 48MHz clock
  SERCOM5->USART.BAUD.reg = (uint16_t)baudValue;
    
  // Enable the receiver and transmitter
  SERCOM5->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
  while (SERCOM5->USART.SYNCBUSY.bit.CTRLB);
    
  // Enable SERCOM5
  SERCOM5->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
  while (SERCOM5->USART.SYNCBUSY.bit.ENABLE);
}

// Function to send a character
void UART_SERCOM5_write(uint8_t data) {
    while (!(SERCOM5->USART.INTFLAG.bit.DRE)); // Wait until Data Register Empty
    SERCOM5->USART.DATA.reg = data;
}

// Function to read a character
uint8_t UART_SERCOM5_read(void) {
    while (!(SERCOM5->USART.INTFLAG.bit.RXC)); // Wait until Receive Complete
    return SERCOM5->USART.DATA.reg;
}


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
  uint64_t baud = 460800; //or 115200
  uint64_t baudValue = 65536 - ((65536 * 16 * baud) / 120000000); // Assuming 120MHz clock
  SERCOM5->USART.BAUD.reg = (uint16_t)baudValue;
    
  // Enable the receiver and transmitter
  SERCOM5->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
  while (SERCOM5->USART.SYNCBUSY.bit.CTRLB);
    
  // Enable SERCOM5
  SERCOM5->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
  while (SERCOM5->USART.SYNCBUSY.bit.ENABLE);

  // Set the RS485 chip to receive
  PORT->Group[0].DIRSET.reg = PORT_PA27; //set PA27 to output
  REG_PORT_OUTCLR0 = PORT_PA27;          //output low for receive
}

// Function to send a character
void serial_write_to_brain(uint8_t data) {
  REG_PORT_OUTSET0 = PORT_PA27;              //output high for transmit
  delayMicroseconds(1);                      //allow the driver to enable
   
  while (!(SERCOM5->USART.INTFLAG.bit.DRE)); // Wait until Data Register Empty
  SERCOM5->USART.DATA.reg = data;            //write the data
  
  while (!(SERCOM5->USART.INTFLAG.bit.TXC)); // Wait until transmit is complete
  REG_PORT_OUTCLR0 = PORT_PA27;              //output low for receive
}

// Function to send multiple bytes
void serial_write_to_brain_buffer(uint8_t* data, int length) {  
  REG_PORT_OUTSET0 = PORT_PA27;              //output high for transmit
  delayMicroseconds(1);                      //allow the driver to enable

  for (int i=0; i<length; i++) {
    while (!(SERCOM5->USART.INTFLAG.bit.DRE)); // Wait until Data Register Empty
    SERCOM5->USART.DATA.reg = data[i];         //write the data
  }
  
  while (!(SERCOM5->USART.INTFLAG.bit.TXC)); // Wait until transmit is complete
  REG_PORT_OUTCLR0 = PORT_PA27;              //output low for receive
}

// Function to read a single byte
uint8_t serial_read_from_brain(void) {
  REG_PORT_OUTCLR0 = PORT_PA27;              //output low for receive
  while (!(SERCOM5->USART.INTFLAG.bit.RXC)); // Wait until Receive Complete
  return SERCOM5->USART.DATA.reg;
}

// Function to read n bytes from the VEX brain with a timeout (in microseconds)
// If the timeout is exceeded, the function returns
uint8_t serial_read_from_brain_delay(uint8_t* buffer, unsigned int timeout_us, int count) {
  unsigned int start_time = micros();  //get the current time in microseconds  
  int timeout_exceeded = 0;
  
  REG_PORT_OUTCLR0 = PORT_PA27;              //output low for receive
  
  for (int i=0; i<count; i++) {
    while (!(SERCOM5->USART.INTFLAG.bit.RXC)) {  //loop until the receive complete flag is set
      if (micros() > (start_time+timeout_us)) {
        return i; //timeout occurred and return the number of bytes received
      }
    }
  
    buffer[i] = SERCOM5->USART.DATA.reg;
  }

  return count; //success, there was no timeout
}

uint8_t serial_byte_available() {
  return SERCOM5->USART.INTFLAG.bit.RXC;
}

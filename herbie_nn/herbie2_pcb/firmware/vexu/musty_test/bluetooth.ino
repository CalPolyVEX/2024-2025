
void init_bluetooth_serial() {
  // Enable the SERCOM1 clock in the PM // Enable the clock for SERCOM1
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM1;
  
  // Select the GCLK (Generic Clock) for SERCOM1
  GCLK->PCHCTRL[SERCOM1_GCLK_ID_CORE].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;

  // Configure the TX and RX pins
  // Assuming PA16 as TX and PA17 as RX
  // Using PORTA so that is Group 0.
  PORT->Group[0].PINCFG[16].reg |= PORT_PINCFG_PMUXEN;  //PA16 for TX
  PORT->Group[0].PMUX[16 >> 1].reg |= 2; //PORT_PMUX_PMUXE_C - even number pin
    
  PORT->Group[0].PINCFG[17].reg |= PORT_PINCFG_PMUXEN;  //PA17 for TX
  PORT->Group[0].PMUX[17 >> 1].reg |= (2 << 4); //PORT_PMUX_PMUXO_C = number 2 shifted to the left 4 bits
    
  // Reset the SERCOM1
  SERCOM1->USART.CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
  while (SERCOM1->USART.CTRLA.bit.SWRST || SERCOM1->USART.SYNCBUSY.bit.SWRST);
    
  // Configure SERCOM1 for UART mode
  SERCOM1->USART.CTRLA.reg = 4 | // Internal clock
                             SERCOM_USART_CTRLA_RXPO(1) |            // RX on PAD[1]
                             SERCOM_USART_CTRLA_TXPO(0) |            // TX on PAD[0]
                             SERCOM_USART_CTRLA_DORD;                // LSB first
    
  // Set the baud rate
  uint64_t baud = 57600; //or 115200
  uint64_t baudValue = 65536 - ((65536 * 16 * baud) / 120000000); // Assuming 120MHz clock
  SERCOM1->USART.BAUD.reg = (uint16_t)baudValue;
    
  // Enable the receiver and transmitter
  SERCOM1->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN;
  while (SERCOM1->USART.SYNCBUSY.bit.CTRLB);
    
  // Enable SERCOM1
  SERCOM1->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
  while (SERCOM1->USART.SYNCBUSY.bit.ENABLE);
}

// Function to send a character
void serial_write_to_bluetooth(uint8_t data) {
  while (!(SERCOM1->USART.INTFLAG.bit.DRE)); // Wait until Data Register Empty
  SERCOM1->USART.DATA.reg = data;            //write the data
  
  while (!(SERCOM1->USART.INTFLAG.bit.TXC)); // Wait until transmit is complete
}

// Function to send multiple bytes
void serial_write_to_bluetooth_buffer(uint8_t* data, int length) {  
  for (int i=0; i<length; i++) {
    while (!(SERCOM1->USART.INTFLAG.bit.DRE)); // Wait until Data Register Empty
    SERCOM1->USART.DATA.reg = data[i];         //write the data
  }
  
  while (!(SERCOM1->USART.INTFLAG.bit.TXC)); // Wait until transmit is complete
}

// Function to read a single byte
uint8_t serial_read_from_bluetooth(void) {
  while (!(SERCOM1->USART.INTFLAG.bit.RXC)); // Wait until Receive Complete
  return SERCOM1->USART.DATA.reg;
}

// Function to read n bytes from the Bluetooth serial with a timeout (in microseconds)
// If the timeout is exceeded, the function returns
uint8_t serial_read_from_bluetooth_delay(uint8_t* buffer, unsigned int timeout_us, int count) {
  unsigned int start_time = micros();  //get the current time in microseconds  
  int timeout_exceeded = 0;
  
  for (int i=0; i<count; i++) {
    while (!(SERCOM1->USART.INTFLAG.bit.RXC)) {  //loop until the receive complete flag is set
      if (micros() > (start_time+timeout_us)) {
        return i; //timeout occurred and return the number of bytes received
      }
    }
  
    buffer[i] = SERCOM1->USART.DATA.reg;
  }

  return count; //success, there was no timeout
}

uint8_t bluetooth_serial_byte_available() {
  return SERCOM1->USART.INTFLAG.bit.RXC;
}

void test_bluetooth_loop() {
  while(1) {
    serial_write_to_bluetooth(100);
    delay(100); 
  }
}

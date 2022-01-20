void init_lb_uart(void) {
  /* Enable APB clock for SERCOM4 */
  PM->APBCMASK.reg |= PM_APBCMASK_SERCOM4;
  
  // Set up the generic clock (GCLK4) used to clock SERCOM4
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
                     GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization

  GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN |         // Enable GCLK4
                      GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                      GCLK_GENCTRL_ID(4);          // Select GCLK4
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization
  
 // Feed GCLK4 to SERCOM4
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to SERCOM4
                      GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                      GCLK_CLKCTRL_ID_SERCOM4_CORE;// Feed GCLK4 to SERCOM4
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization
  
  //set pin direction
  PORT->Group[1].DIRSET.reg = PORT_PB08; //set the direction to output
  //PORT->Group[1].OUTCLR.reg = PORT_PB08; //set the value to output LOW
  PORT->Group[1].OUTSET.reg = PORT_PB08; //set the value to output HIGH

  // Enable the peripheral multiplexer for the pin PB08.
  PORT->Group[1].PINCFG[8].reg |= PORT_PINCFG_PMUXEN;

  // Set PB08's function to function D. Function D is SERCOM4 for PB08.
  // Because this is an even numbered pin the PMUX is E (even) and the PMUX
  // index is pin number / 2, so 4.
  PORT->Group[1].PMUX[4].reg = PORT_PMUX_PMUXE_D;
    
  //use A1 = SER4:0 for lightbar UART TX

  //assuming fref = 16MHz, baud frequency = 115200, 16x oversampling
  //BAUD register = 65536 * (1 - 16 * (115200 / 16000000)) = 57986.2528
  
  SERCOM4->USART.CTRLA.reg =
      SERCOM_USART_CTRLA_DORD |              //LSB transmitted first
      SERCOM_USART_CTRLA_MODE_USART_INT_CLK | //use internal clock
      SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(2/*PAD0*/);
  while(SERCOM4->USART.SYNCBUSY.reg){}

  SERCOM4->USART.CTRLB.reg = SERCOM_USART_CTRLB_TXEN;    //enable the transmitter
  while(SERCOM4->USART.SYNCBUSY.reg){}

  delay(20);
     
  //uint64_t br = (uint64_t)65536 * (F_CPU - 16 * baud) / F_CPU;
  //SERCOM4->USART.BAUD.reg = (uint16_t)br+1;
  SERCOM4->USART.BAUD.reg = 57986;  //set to 115200 baud with 16MHz clock

  SERCOM4->USART.CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

  delay(20);
}

void send_lb_byte(unsigned char b) {
  SERCOM4->USART.DATA.reg = b;
}

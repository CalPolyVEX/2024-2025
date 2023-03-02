void init_lb_uart(void) {  //init UART on SmartPort header (J18 on v0.2 Herbie PCB)
  /* Enable APB clock for SERCOM0 */
  PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;
  
  // Feed GCLK6 to SERCOM0
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |         // Enable GCLK6 to SERCOM0
                      GCLK_CLKCTRL_GEN_GCLK6 |     // Select GCLK6
                      GCLK_CLKCTRL_ID_SERCOM0_CORE;// Feed GCLK6 to SERCOM0
  while (GCLK->STATUS.bit.SYNCBUSY);               // Wait for synchronization
  
  //set pin direction
  PORT->Group[0].DIRSET.reg = PORT_PA10; //set the direction to output

  // Enable the peripheral multiplexer for the pin PA10.
  PORT->Group[0].PINCFG[10].reg |= PORT_PINCFG_PMUXEN;

  // Set PA10's function to function C. Function C is SERCOM0 for PA10.
  // Because this is an even numbered pin the PMUX is E (even) and the PMUX
  // index is pin number / 2, so 5.  Group[0] indicates PORTA.
  PORT->Group[0].PMUX[5].reg |= PORT_PMUX_PMUXE_C;
    
  //use D1 = SER0:2 for lightbar UART TX

  //assuming fref = 16MHz, baud frequency = 115200, 16x oversampling
  //BAUD register = 65536 * (1 - 16 * (115200 / 16000000)) = 57986.2528
  
  /* Wait for synchronization */
  while(SERCOM0->USART.SYNCBUSY.bit.ENABLE);
  /* Disable the SERCOM UART module */
  SERCOM0->USART.CTRLA.bit.ENABLE = 0;
  /* Wait for synchronization */
  while(SERCOM0->USART.SYNCBUSY.bit.SWRST);
  /* Perform a software reset */
  SERCOM0->USART.CTRLA.bit.SWRST = 1;
  /* Wait for synchronization */
  while(SERCOM0->USART.CTRLA.bit.SWRST);
  /* Wait for synchronization */
  while(SERCOM0->USART.SYNCBUSY.bit.SWRST || SERCOM0->USART.SYNCBUSY.bit.ENABLE);
  
  SERCOM0->USART.CTRLA.reg =
      SERCOM_USART_CTRLA_DORD |              //LSB transmitted first
      SERCOM_USART_CTRLA_MODE_USART_INT_CLK | //use internal clock
      SERCOM_USART_CTRLA_RXPO(1/*PAD1*/) | SERCOM_USART_CTRLA_TXPO(1/*PAD2*/);
  while(SERCOM0->USART.SYNCBUSY.reg){}

  SERCOM0->USART.CTRLB.reg = SERCOM_USART_CTRLB_TXEN;    //enable the transmitter
  while(SERCOM0->USART.SYNCBUSY.reg){}
     
  //uint64_t br = (uint64_t)65536 * (F_CPU - 16 * baud) / F_CPU;
  //SERCOM4->USART.BAUD.reg = (uint16_t)br+1;
  SERCOM0->USART.BAUD.reg = 57986;  //set to 115200 baud with 16MHz clock

  SERCOM0->USART.CTRLA.bit.ENABLE = 1;
}

void send_lb_byte(unsigned char b) {  //send bytes on SmartPort header (J18 on v0.2 Herbie PCB)
  SERCOM0->USART.DATA.reg = b;
}

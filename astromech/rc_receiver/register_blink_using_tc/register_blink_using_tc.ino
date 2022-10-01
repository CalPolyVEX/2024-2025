#define GCLKGENNUM 5 //generic clock generator to use
#define GCLKNUM GCLK_CLKCTRL_ID_TCC2_TC3_Val //generic clock multiplexer to use
#define TC TC3 //TC to use
#define ONTIME 1000 //time (in ms) between led toggles
#define LEDPINPORTGROUP 0 //port group of led pin (0 for A, 1 for B, etc)
#define LEDPIN 17 //led pin number within port group

void setup() {
  //set our LED pin as an output
  PORT->Group[0].DIRSET.reg = 0x1ul << 17;
  
  
  //Initialize clock generator GCLKGENNUM's division
  GCLK->GENDIV.reg =
    GCLK_GENDIV_DIV(0x09) | //set a division factor of 8
    GCLK_GENDIV_ID(GCLKGENNUM); //apply the following clock division to

  //When the GENDIV register is written to, synchronization occurs. This waits for that to complete before continuing
  while(GCLK->STATUS.bit.SYNCBUSY);
  
  //Initialize and start clock generator 5
  /*NOTE: because of how this is done, the following bits are set to 0   
   *  RUNSTDBY
   *  OE
   *  OOV
   *  IDC
   *    this bit doesn't need to be set, as an even division factor is being used 
   */
  
  GCLK->GENCTRL.reg =
    GCLK_GENCTRL_SRC_OSC8M| //use OSC8M as clock source (with division factor of 8, a 1MHz signal is made)    
    GCLK_GENCTRL_GENEN| //indicates the generator should be started
    GCLK_GENCTRL_DIVSEL|
    GCLK_GENCTRL_ID(GCLKGENNUM); //apply this all to the proper generator
  
  //synchronization also happens when writing to GENCTRL
  while(GCLK->STATUS.bit.SYNCBUSY);

  //connect to specified gclock mux 
  GCLK->CLKCTRL.reg = 
    GCLK_CLKCTRL_CLKEN| //enable the clock connection to the peripheral(s)
    GCLK_CLKCTRL_GEN(GCLKGENNUM)| //use the specified generator
    GCLK_CLKCTRL_ID(GCLKNUM); //use the specified GCLOCK mux

  //Initialize TC
  TC->COUNT16.CTRLA.reg =
    TC_CTRLA_MODE_COUNT16| //use tc in 16 bit mode
    TC_CTRLA_WAVEGEN_MFRQ//use match frequency operation
    //TC_CTRLA_PRESCALER_DIV1024| //set prescaler to 1/1024 (so roughly 1000 counts per second)
    //TC_CTRLA_PRESCSYNC_PRESC //reset after match on next count tick (not gclock)
    ;
  //set match value
  TC->COUNT16.CC[0].reg = 7812;
    
  //synchronization will happen, so we must wait
  while(TC->COUNT16.STATUS.bit.SYNCBUSY);

  //enable intrrupts
  TC->COUNT16.INTENSET.bit.MC0 = 1;//enable tc interrupts for CC0 interrupts
  
  //enable TC
  TC->COUNT16.CTRLA.bit.ENABLE = 1;
    
  //synchronization will happen, so we must wait
  while(TC->COUNT16.STATUS.bit.SYNCBUSY);

  NVIC_DisableIRQ(TC3_IRQn);
  NVIC_ClearPendingIRQ(TC3_IRQn);
  NVIC_SetPriority(TC3_IRQn, 0); 
  NVIC_EnableIRQ(TC3_IRQn);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void TC3_Handler() {
  PORT->Group[LEDPINPORTGROUP].OUTTGL.reg = 0x1ul << LEDPIN;
  TC->COUNT16.INTFLAG.bit.MC0 = 1;
}

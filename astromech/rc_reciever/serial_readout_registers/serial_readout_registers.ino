/*ASSUMPTIONS THAT MIGHT HAUNT ANYBODY DEBUGGING THIS:
 * after enabling RX on serial, baud register can be written to
 */
 
 /* PROCESS
 * PART 0: PRESETUP
 *P  ensure SBUSSERCOM's interrupts are disabled
 *   use PORT peripheral to configure RXPIN and TXPIN to SBUSSERCOM control
 *     NOTE: PULLEN and DRVSTR bits are still controlled by PORT
 *     NOTE TO SELF: could initializing the TXPIN be useless due to it being disabled in the peripheral?
 * PART 1: CLOCK SETUP
 *   set up generic clock generator SERGEN to clock sercom SBUSSERCOM
 *    NOTE: if GCLOCK0 is selected, DO NOT DO THIS
 * PART 2: INITIALIZING SERCOM
 *P  reset to the peripheral and turn it off by writing 1 to CTRLA.SWRST
 *P  wait for sync
 *   set CTRLA.MODE to 0x1 to use the internal clock
 *   set CTRLA.CMODE to 0 to use async transmission
 *   set CTRLA.RXPO to proper pin (NOT standard pin numbers. refer to RXPO on pg 432)
 *   set CTRLA.TXPO as similar to RXPO
 *   set/ensure CTRLB.CHSIZE is set to 0x0 to set charater size to 8 bits
 *    note: sync unnecessary
 *   set CTRLA.DORD to 1 to use LSB character recieving
 *   set CTRLA.FORM to 0x1 to use a parity bit
 *   set/ensure CTRLB.PMODE to 0x0 for even parity
 *    no sync
 *   set CTLRB.SBMODE to 0x1 for 2 stop bits
 *    no sync
 *   set CTRLB.RXEN to 0x1
 *   wait for sync on CTRLB
 *   set BAUD register
 *   set/ensure CTRLB.TXEN to 0x0
 *P  wait for sync
 *   set INTENSET.RXC to 1
 *   set CTRLA.ENABLE to 1 to enable serial
 *   wait for sync
 * PART 3: INTERRUPT ENABLING
 *P  clear pending interrupts from SUBSSERCOM
 *   set priority to 0 for SBUSSERCOM
 *   enable interrupt requests from SBUSSERCOM
 * PART 4: ARDUINO SERIAL SETUP
 *   set SerialUSB up
 *   NOTE: this is in the irq handler function for SBUSSERCOM
 *   set newData boolean to true
 *   set regCont to data in DATA register
 * PART 5: ARDUINO LOOP
 *   if newData
 *    print into console serial regCont
 *    set newData to false
 */

#define PARANOIA //if defined, potentially excessive operations will be done to ensure intended functionality

/*
 * USING
 *  Sercom instance: SERCOM2
 *  RX Pin: PA20
 *  TX Pin: PA21
 *  Generic Clock Generator: GCLOCK0
 */

 /* PINOUT NOTES:
  *   PA10 is connected to D1/TXO on the Redboard
  *   PA11 is connected to D0/RXI on the Redboard
  */

volatile boolean newData = false;
volatile uint16_t regCont;

void setup() {

  //init debug led
  PORT->Group[0].DIRSET.reg = 1 << 17;
  PORT->Group[0].OUTCLR.reg = 1 << 17;
  
  #ifdef PARANOIA
  NVIC_DisableIRQ(SERCOM2_IRQn); //using sercom2
  #endif

  //INITIALIZING PADS
  //initialize RX pin to be controlled by serial
  PORT->Group[0].PINCFG[11].bit.PMUXEN = 1; //designate RXPIN as controlled by a peripheral
  //PORT->Group[0].PMUX[10].reg = PORT_PMUX_PMUXE(3); //set to use multiplexing C function

  //initialize TX pin to be controlled by serial
  PORT->Group[0].PINCFG[10].bit.PMUXEN = 1; //designate TXPIN as controlled by a peripheral
  PORT->Group[0].PMUX[5].reg = PORT_PMUX_PMUXO_D | PORT_PMUX_PMUXE_D; //set to use multiplexing C function

  //CLOCKING SERIAL
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
    GCLK_GENDIV_ID(4);   // Select Generic Clock (GCLK) 4
  GCLK->GENCTRL.reg = 
    GCLK_GENCTRL_IDC | 
    GCLK_GENCTRL_GENEN | //enable the clock connection to the peripheral(s)
    GCLK_GENCTRL_ID(4) | //use clock gen 4
    GCLK_GENCTRL_SRC_DFLL48M; // Set Clock to 48 MHz
  GCLK->CLKCTRL.reg =
    GCLK_CLKCTRL_CLKEN |
    GCLK_CLKCTRL_GEN_GCLK4 |
    GCLK_CLKCTRL_ID_SERCOM2_CORE;

  
  //INITIALIZING SERIAL
  #ifdef PARANOIA
  SERCOM2->USART.CTRLA.bit.SWRST = 1; //do a software reset on the serial peripheral
  while(SERCOM2->USART.SYNCBUSY.bit.SWRST);
  #endif

  SERCOM2->USART.CTRLA.bit.MODE = 1; //using internal clock
  SERCOM2->USART.CTRLA.bit.CMODE = 0; //use asyncronous communication
  SERCOM2->USART.CTRLA.bit.RXPO = 0x3; //PA11 multiplex mode d is sercom2 PAD[3]
  SERCOM2->USART.CTRLA.bit.TXPO = 0x2; //similar but is PAD[0]
  
  SERCOM2->USART.CTRLB.bit.CHSIZE = 0x0; //we have 8 bits of data per USART frame
  SERCOM2->USART.CTRLA.bit.DORD = 1; //using LSB
  SERCOM2->USART.CTRLA.bit.FORM = 0x1; //using 1 parity bit
  SERCOM2->USART.CTRLB.bit.PMODE = 0x0; //using even parity
  SERCOM2->USART.CTRLB.bit.SBMODE = 0x1; //using 2 stop bits
  SERCOM2->USART.BAUD.reg = 63351; //set the correct baud register value (calculated based on equation in samd21 datasheet)
  SERCOM2->USART.CTRLB.bit.RXEN = 0x1; //enable Serial RX
  while(SERCOM2->USART.SYNCBUSY.bit.CTRLB); //wait for sync
  SERCOM2->USART.CTRLB.bit.TXEN = 0x1; //turn tx pin off
  #ifdef PARANOIA
    while(SERCOM2->USART.SYNCBUSY.bit.CTRLB); // wait for sync
  #endif
  SERCOM2->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC; //enable RX interrupts
  SERCOM2->USART.CTRLA.bit.ENABLE = 1; //enable the serial module
  while(SERCOM2->USART.SYNCBUSY.bit.ENABLE); //wait for syncronization

  //ENABLING INTERRUPTS
  #ifdef PARANOIA
    NVIC_ClearPendingIRQ(SERCOM2_IRQn); //clear any incoming interrupt requests form SERCOM5
  #endif
  NVIC_SetPriority(SERCOM2_IRQn, 0); //set highest priority for SERCOM2
  NVIC_EnableIRQ(SERCOM2_IRQn); //enable interrupt requests for SERCOM2

  SerialUSB.begin(9600);
  //PORT->Group[0].OUTSET.reg = 1 << 17;
}

void loop() {
  if(newData) {
    SerialUSB.println(regCont, HEX);
    newData = false;
  }
}

void SERCOM2_Handler() {
  PORT->Group[0].OUTTGL.reg = 1 << 17;
  regCont = SERCOM2->USART.DATA.reg;
  newData = true;
}

/*ASSUMPTIONS THAT MIGHT HAUNT ANYBODY DEBUGGING THIS:
 * GCLOCK0 is clocked to the internal 48MHZ source
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
 *   set BAUD register to ``2429 (assuming 8MHz serial clock source)
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
 * PART 4: INTERRUPT HANDLER 
 *   NOTE: this is in the irq handler function for SBUSSERCOM
 *   set newData boolean to true
 *   set regCont to data in DATA register
 * PART 5: ARDUINO LOOP
 *   if newData
 *    print into console serial regCont
 *    set newData to false
 */

#define PARANOIA //if defined, potentially excessive operations will be done to ensure intended functionality

//#define SERGEN //generic clock generator to be used for the sercom instance recieving SBUS
//#define SBUSSERCOM //sercom instance to be used for recieving SBUS packets
///* NOTE:
// * RX and TX pin definitions can only be pins that the SBUSSERCOM instance
// * can use.
// */
//#define RXPINGROUP //pad group that RXPIN belongs to
//#define RXPIN //pad number of RXPIN
//#define RXPINMUXNUM //number of the RXPIN MUX group
//#define RXPINODD //1 if RXPIN is odd, 0 elsewise
//#define TXPINGROUP //pad group that TXPIN belongs to
//#define TXPIN //pad number of TXPIN
//#define TXPINMUXNUM //number of the TXPIN MUX group
//#define TXPINODD //1 if TXPIN is odd, 0 elsewise


//do not change
//#define SERCOMIRQ SBUSSERCOM+9



/*
 * USING
 *  Sercom instance: SERCOM2
 *  RX Pin: PA20
 *  TX Pin: PA21
 *  Generic Clock Generator: GCLOCK0
 */

volatile boolean newData = false;
volatile uint16_t regCont;

void setup() {
  #ifdef PARANOIA
  NVIC_DisableIRQ(SERCOM0_IRQn); //using sercom5
  #endif

  //init debug led
  PORT->Group[0].DIRSET.reg = 1 << 17;
  PORT->Group[0].OUTCLR.reg = 1 << 17;
  
  //INITIALIZING PADS
  //initialize RX pin to be controlled by serial
  PORT->Group[0].PINCFG[11].bit.PMUXEN = 1; //designate RXPIN as controlled by a peripheral
  //PORT->Group[0].PMUX[10].reg = PORT_PMUX_PMUXE(2); //set to use multiplexing C function

  //initialize TX pin to be controlled by serial
  PORT->Group[0].PINCFG[10].bit.PMUXEN = 1; //designate TXPIN as controlled by a peripheral
  PORT->Group[0].PMUX[5].reg = PORT_PMUX_PMUXO(2) | PORT_PMUX_PMUXE(2); //set to use multiplexing C function

  //CLOCKING SERIAL
  GCLK->CLKCTRL.reg = 
    GCLK_CLKCTRL_CLKEN| //enable the clock connection to the peripheral(s)
    GCLK_CLKCTRL_GEN(0)| //use clock gen 0
    GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_SERCOM0_CORE_Val); //use the specified GCLOCK mux
  
  //INITIALIZING SERIAL
  #ifdef PARANOIA
  SERCOM0->USART.CTRLA.bit.SWRST = 1; //do a software reset on the serial peripheral
  while(SERCOM0->USART.SYNCBUSY.bit.SWRST);
  #endif

  SERCOM0->USART.CTRLA.bit.MODE = 1; //using internal clock
  SERCOM0->USART.CTRLA.bit.CMODE = 0; //use asyncronous communication
  SERCOM0->USART.CTRLA.bit.RXPO = 0x3; //PA20 multiplex mode c is sercom5 PAD[2]
  SERCOM0->USART.CTRLA.bit.TXPO = 0x2; //similar but is PAD[3]
  
  SERCOM0->USART.CTRLB.bit.CHSIZE = 0x0; //we have 8 bits of data per USART frame
  SERCOM0->USART.CTRLA.bit.DORD = 1; //using LSB
  SERCOM0->USART.CTRLA.bit.FORM = 0x1; //using 1 parity bit
  SERCOM0->USART.CTRLB.bit.PMODE = 0x0; //using even parity
  SERCOM0->USART.CTRLB.bit.SBMODE = 0x1; //using 2 stop bits
  SERCOM0->USART.CTRLB.bit.RXEN = 0x1; //enable Serial RX
  SERCOM0->USART.BAUD.reg = 65399; //set the correct baud register value (calculated based on equation in samd21 datasheet)
  while(SERCOM0->USART.SYNCBUSY.bit.CTRLB); //wait for sync
  SERCOM0->USART.CTRLB.bit.TXEN = 0x1; //turn tx pin off
  #ifdef PARANOIA
    while(SERCOM0->USART.SYNCBUSY.bit.CTRLB); // wait for sync
  #endif
  SERCOM0->USART.INTENSET.bit.RXC = 1; //enable RX interrupts
  SERCOM0->USART.CTRLA.bit.ENABLE = 1; //enable the serial module
  while(SERCOM0->USART.SYNCBUSY.bit.ENABLE); //wait for syncronization

  //ENABLING INTERRUPTS
  #ifdef PARANOIA
    NVIC_ClearPendingIRQ(SERCOM0_IRQn); //clear any incoming interrupt requests form SERCOM5
  #endif
  NVIC_SetPriority(SERCOM0_IRQn, 0); //set highest priority for SERCOM5
  NVIC_EnableIRQ(SERCOM0_IRQn); //enable interrupt requests for SERCOM5
}

void loop() {
  SERCOM0->USART.DATA.bit.DATA = 0x5;
  delay(30);
}

void SERCOM0_Handler() {
  newData = true;
  regCont = SERCOM0->USART.DATA.bit.DATA;
  PORT->Group[0].OUTTGL.reg = 1<<17;
}

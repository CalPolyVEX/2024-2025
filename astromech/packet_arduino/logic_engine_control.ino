#include "logic_engine_control.h"

// LED Controller Uses Pin PA00 for RX and PA01 for TX

// Notes for PCB Connections:
// UART1 TX: D10 (PA18)
// UART1 RX: D11 (PA16)
// UART2 TX: A1 (PB08)
// UART2 RX: A2 (PB09)

// This Module Will Use UART2 TX with Sercom4, UART2 RX Will be Unused

// Data Transmission PseudoCode
// Have an Array of Size N Bytes to Contain the Data to be Transmitted
// This Array Should Have an Iterator That Contains the Current Byte to be Sent
// Upon Request to Send Packet, sendLEDCommand Will be Called.
// This Function Will Store The Data to be Transmitted, Its Size, Reset the
// Iterator, and Activate the First Byte to be Transmitted For Each Transmitted
// Byte that is Complete, an Interupt Will be Called, Requesting the Next Byte
// Increment the Iterator, but if Iterator Equals Packet Size, Stop Transmitting
// No Need for a Timer

// Array of Bytes
uint8_t transmit_bytes[13] = {'~', 'R', 'T', 'L', 'E', '0', '0', '0', '0', '0', '0', '0', 13};

// List of Predefined Commands
uint8_t logic_engine_commands[9][4] = {
    {'0', '5', '1', '5'}, {'0', '5', '2', '5'}, {'0', '5', '3', '5'},
    {'0', '5', '4', '5'}, {'0', '5', '5', '5'}, {'0', '5', '6', '5'},
    {'0', '5', '7', '5'}, {'0', '5', '8', '5'}, {'0', '5', '9', '5'},
};

// Index in Byte Array
uint8_t byte_index = 0;

// The Size of the Current Byte Packet
uint8_t packet_size = 0;

// Setup the USART for the Logic Engine Controller
void setupLogicEngine() {
    PORT->Group[1].DIRSET.reg |= PORT_PB08;
    PORT->Group[1].OUTCLR.reg |= PORT_PB08;
    // PORT->Group[1].DIR.reg |= PORT_PB08;

    NVIC_DisableIRQ(SERCOM4_IRQn);

    // CLOCKING SERIAL
    GCLK->GENDIV.reg =
        GCLK_GENDIV_DIV(250) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
        GCLK_GENDIV_ID(6);     // Select Generic Clock (GCLK) 6
    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |
                        GCLK_GENCTRL_GENEN | // enable the clock connection to the peripheral(s)
                        GCLK_GENCTRL_ID(6) | // use clock gen 6
                        GCLK_GENCTRL_SRC_DFLL48M;     // Set Clock to 48 MHz
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |          // enable the clock
                        GCLK_CLKCTRL_GEN_GCLK6 |      // enable generic clock generator 6
                        GCLK_CLKCTRL_ID_SERCOM4_CORE; // set clock to SERCOM4_CORE

    SERCOM4->USART.CTRLA.bit.SWRST = 1; // do a software reset on the serial peripheral
    while (SERCOM4->USART.SYNCBUSY.bit.SWRST)
        ;

    // INITIALIZING PADS
    // initialize TX pin to be controlled by serial
    PORT->Group[1].PINCFG[8].bit.PMUXEN = 1;        // designate RXPIN as controlled by a peripheral
    PORT->Group[1].PMUX[4].reg = PORT_PMUX_PMUXE_D; // set to use multiplexing D function

    // INITIALIZING SERIAL

    SERCOM4->USART.CTRLA.bit.MODE = 1;     // using internal clock
    SERCOM4->USART.CTRLA.bit.CMODE = 0;    // use asyncronous communication
    SERCOM4->USART.CTRLA.bit.SAMPR = 0x0;  // Set Serial to Arithmatic Mode
    SERCOM4->USART.CTRLA.bit.RXPO = 0x1;   // Set Unused RX Pin to Pad[1]
    SERCOM4->USART.CTRLA.bit.TXPO = 0x0;   // Set TX Pin to Pad[0]
    SERCOM4->USART.INTENCLR.bit.TXC = 0x1; // Set interupt to be disabled

    SERCOM4->USART.CTRLB.bit.CHSIZE = 0x0; // we have 8 bits of data per USART frame
    SERCOM4->USART.CTRLA.bit.DORD = 1;     // using MSB
    SERCOM4->USART.CTRLB.bit.SBMODE = 0x0; // using 1 stop bits
    SERCOM4->USART.BAUD.reg = 13107;       // Baud Rate for LED Controller is 9600
    SERCOM4->USART.CTRLB.bit.TXEN = 0x1;   // enable Serial TX
    SERCOM4->USART.CTRLB.bit.RXEN = 0x0;   // turn RX pin off
    while (SERCOM4->USART.SYNCBUSY.bit.CTRLB)
        ; // wait for sync

    SERCOM4->USART.CTRLA.bit.ENABLE = 1; // enable the serial module

    while (SERCOM4->USART.SYNCBUSY.bit.ENABLE)
        ; // wait for syncronization

    // Enable the Transmit Complete Interupt
    NVIC_ClearPendingIRQ(SERCOM4_IRQn); // clear any incoming interrupt requests from SERCOM4
    NVIC_SetPriority(SERCOM4_IRQn, 0);  // set highest priority for SERCOM4
    NVIC_EnableIRQ(SERCOM4_IRQn);       // enable interrupt requests for SERCOM4
    SERCOM4->USART.INTENSET.reg = SERCOM_USART_INTENSET_TXC; // enable TXC interrupts for when
                                                             // recieving is complete

    // Reset Byte Index
    byte_index = 0;
}

// Send Command to Logic Engine Controller
void sendLogicEngineCommand(uint8_t command_major, uint8_t command_minor, uint8_t color,
                            uint8_t speed) {
    // Copy Data of Package Parameter to Transmit Bytes Array
    transmit_bytes[6] = command_major;
    transmit_bytes[7] = command_minor;
    transmit_bytes[8] = color;
    transmit_bytes[9] = speed;

    // Send First Byte
    SERCOM4->USART.DATA.bit.DATA = transmit_bytes[0];

    // Reset Byte Index
    byte_index = 1;
}

// Send Command to Logic Engine Controller
void sendLogicEngineCommand(uint8_t preset_index) {
    // Copy Data of Package Parameter to Transmit Bytes Array
    uint8_t *preset = logic_engine_commands[preset_index];
    for (int i = 0; i < 4; i++)
        transmit_bytes[i + 6] = preset[i];

    // Send First Byte
    SERCOM4->USART.DATA.bit.DATA = transmit_bytes[0];

    // Reset Byte Index
    byte_index = 1;
}

void SERCOM4_Handler() {
    // Reset the Interupt Flag
    SERCOM4->USART.INTFLAG.bit.TXC = 0x1;

    // If Data is Still Left to be Transmitted, Transmit the Next Byte
    if (byte_index < 13) {
        SERCOM4->USART.DATA.bit.DATA = transmit_bytes[byte_index];
        byte_index++;
    }
}

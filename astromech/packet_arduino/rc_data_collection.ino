#include "rc_data_collection.h"

#define PARANOIA // if defined, potentially excessive operations will be done to ensure intended functionality
#define DOME_SERVO 4
#define DOME_SERVO_NUM 0
#define TOGGL_CHNL 5 // channel associated with the switch that determines whether to get input from RC or PC
#define TOGGL_VAL_RC 461 // value output by channel used for toggling to RC
#define TOGGL_VAL_PC 1529 // value output by channel used for toggling to PC

/*
 * USING
 *  Sercom instance: SERCOM2
 *  RX Pin: PA20
 *  TX Pin: PA21
 *  Generic Clock Generator: GCLOCK4
 */

/* PINOUT NOTES:
 *   PA10 is connected to D1/TXO on the Redboard
 *   PA11 is connected to D0/RXI on the Redboard
 */

// used for counter to stop reading data and allow the data to be stored
volatile boolean newData = false;
// temporary data storage var
volatile uint16_t regCont;
volatile boolean stayInPC = false;

// Queue class
// Implemented using a circular array
class Queue {
public:
    // Initialize Queue Object
    Queue() {
        // Set Head and Tail Initial Values
        head = 0;
        tail = 0;
        data_size = 0;

        dummy_var = 0;
    }

    // Enqueue data
    void enqueue(uint8_t new_data) {
        // If buffer is full, error
        if (data_size == BUFFER_SIZE) {
            return;
        }

        // Insert Data and Increment Head
        buffer[head] = new_data;
        data_size++;
        head++;

        // If head is outside buffer size, go back to start of array
        if (head > BUFFER_SIZE)
            head = 0;
    }

    // Dequeue Data
    uint8_t dequeue() {
        // If buffer is Empty, error
        if (!data_size) {
            return 0xf;
        }

        // Read Data and Increment Tail
        dummy_var = buffer[tail];
        data_size--;
        tail++;

        // If tail is outside buffer size, go back to start of array
        if (tail > BUFFER_SIZE)
            tail = 0;

        // Return data
        return dummy_var;
    }

    // Dequeues Data into Array
    uint8_t dequeue_array(unsigned int size, uint8_t *array) {
        // Test if there is sufficient data
        if (data_size < size) {
            return 0xf;
        }

        for (unsigned int i = 0; i < size; i++) {
            // Read Data and Increment Tail
            array[i] = buffer[tail];
            data_size--;
            tail++;

            // If tail is outside buffer size, go back to start of array
            if (tail > BUFFER_SIZE)
                tail = 0;
        }
    }

    // Reset the queue
    void reset() {
        // Set Head and Tail Initial Values
        head = 0;
        tail = 0;
        data_size = 0;
    }

private:
    // Circular Array
    uint8_t buffer[32];

    // Head and Tail Markers
    uint8_t head;
    uint8_t tail;

    // Number of Data in Buffer
    uint8_t data_size;

    // Constant Size of Buffer
    uint8_t BUFFER_SIZE = 31;

    uint8_t dummy_var;
};

// Queue object (acts as a serial buffer)
Queue queue;

// Array for a Complete SBUS Packet
uint8_t complete_packet[25];

// Array for channel data
uint16_t channel[16];

void receiver_setup() {

#ifdef PARANOIA
    NVIC_DisableIRQ(SERCOM2_IRQn); // using sercom2
#endif

    // INITIALIZING PADS
    // initialize RX pin to be controlled by serial
    PORT->Group[0].PINCFG[11].bit.PMUXEN = 1; // designate RXPIN as controlled by a peripheral

    // initialize TX pin to be controlled by serial
    PORT->Group[0].PINCFG[10].bit.PMUXEN = 1;                           // designate TXPIN as controlled by a peripheral
    PORT->Group[0].PMUX[5].reg = PORT_PMUX_PMUXO_D | PORT_PMUX_PMUXE_D; // set to use multiplexing C function

    // CLOCKING SERIAL
    GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                       GCLK_GENDIV_ID(5);   // Select Generic Clock (GCLK) 4
    GCLK->GENCTRL.reg =
        GCLK_GENCTRL_IDC |
        GCLK_GENCTRL_GENEN |      // enable the clock connection to the peripheral(s)
        GCLK_GENCTRL_ID(4) |      // use clock gen 4
        GCLK_GENCTRL_SRC_DFLL48M; // Set Clock to 48 MHz
    GCLK->CLKCTRL.reg =
        GCLK_CLKCTRL_CLKEN |          // enable the clock
        GCLK_CLKCTRL_GEN_GCLK4 |      // enable generic clock generator 4
        GCLK_CLKCTRL_ID_SERCOM2_CORE; // set clock to SERCOM2_CORE

// INITIALIZING SERIAL
#ifdef PARANOIA
    SERCOM2->USART.CTRLA.bit.SWRST = 1; // do a software reset on the serial peripheral
    while (SERCOM2->USART.SYNCBUSY.bit.SWRST)
        ; // wait for synchronization
#endif

    SERCOM2->USART.CTRLA.bit.MODE = 1;   // using internal clock
    SERCOM2->USART.CTRLA.bit.CMODE = 0;  // use asyncronous communication
    SERCOM2->USART.CTRLA.bit.RXPO = 0x3; // PA11 multiplex mode d is sercom2 PAD[3]
    SERCOM2->USART.CTRLA.bit.TXPO = 0x2; // similar but is PAD[0]

    SERCOM2->USART.CTRLB.bit.CHSIZE = 0x0; // we have 8 bits of data per USART frame
    SERCOM2->USART.CTRLA.bit.DORD = 1;     // using LSB
    SERCOM2->USART.CTRLA.bit.FORM = 0x1;   // using 1 parity bit
    SERCOM2->USART.CTRLB.bit.PMODE = 0x0;  // using even parity
    SERCOM2->USART.CTRLB.bit.SBMODE = 0x1; // using 2 stop bits
    SERCOM2->USART.BAUD.reg = 63351;       // set the correct baud register value (calculated based on equation in samd21 datasheet)
    SERCOM2->USART.CTRLB.bit.RXEN = 0x1;   // enable Serial RX
    while (SERCOM2->USART.SYNCBUSY.bit.CTRLB)
        ;                                // wait for sync
    SERCOM2->USART.CTRLB.bit.TXEN = 0x1; // turn tx pin off
#ifdef PARANOIA
    while (SERCOM2->USART.SYNCBUSY.bit.CTRLB)
        ; // wait for sync
#endif
    SERCOM2->USART.CTRLA.bit.ENABLE = 1; // enable the serial module
    while (SERCOM2->USART.SYNCBUSY.bit.ENABLE)
        ; // wait for syncronization

// ENABLING INTERRUPTS
#ifdef PARANOIA
    NVIC_ClearPendingIRQ(SERCOM2_IRQn); // clear any incoming interrupt requests from SERCOM5
#endif
    NVIC_SetPriority(SERCOM2_IRQn, 0);                       // set highest priority for SERCOM2
    NVIC_EnableIRQ(SERCOM2_IRQn);                            // enable interrupt requests for SERCOM2
    SERCOM2->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC; // enable RX interrupts for when recieving is complete

    // Initialize Clock for Timer
    GCLK->GENDIV.reg = GCLK_GENDIV_DIV(6) | // Divide the 48MHz clock source by divisor 6: 48MHz/6=8MHz
                       GCLK_GENDIV_ID(3);   // Select Generic Clock (GCLK) 3
    GCLK->GENCTRL.reg =
        GCLK_GENCTRL_SRC_DFLL48M | // use OSC8M as clock source (with division factor of 8, a 1MHz signal is made)
        GCLK_GENCTRL_GENEN |       // indicates the generator should be started
        GCLK_GENCTRL_ID(3);        // apply this all to the proper generator
    // connect to specified gclock mux
    GCLK->CLKCTRL.reg =
        GCLK_CLKCTRL_CLKEN | // enable the clock connection to the peripheral(s)
        GCLK_CLKCTRL_GEN_GCLK3 |
        GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_TCC2_TC3_Val); // use the specified GCLOCK mux

    // synchronization also happens when writing to GENCTRL
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // Initialize TC
    TC3->COUNT16.CTRLA.reg =
        TC_CTRLA_MODE_COUNT16 | // use tc in 16 bit mode
        TC_CTRLA_WAVEGEN_NFRQ;  // use normal counting mode

    // Set Counting Down Mode
    TC3->COUNT16.CTRLBSET.reg =
        TC_CTRLBSET_ONESHOT |   // set the countdown condition for setting an interupt when the counter reaches an overflow or underflow
        TC_CTRLBSET_DIR;        // set the direction to count down

    // synchronization will happen, so we must wait
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
        ;

    // enable intrrupts
    TC3->COUNT16.INTENSET.bit.OVF = 1; // enable tc overflow/underflow interupt

    // enable TC
    TC3->COUNT16.CTRLA.bit.ENABLE = 1;

    // synchronization will happen, so we must wait
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
        ;

    // Enable interrupts from the Timer
    NVIC_DisableIRQ(TC3_IRQn);
    NVIC_ClearPendingIRQ(TC3_IRQn);
    NVIC_SetPriority(TC3_IRQn, 0);
    NVIC_EnableIRQ(TC3_IRQn);
}

bool receiver_loop() {
    /* main 
    allows for switching between RC and PC */

    static bool pc_mode = false;

    if (newData) {

        // Disable newData flag
        newData = false;

        // Dequeue values into array
        queue.dequeue_array(25, complete_packet);

        // Test if packet is valid, reset if not
        if (!(complete_packet[0] == 0x0F && complete_packet[24] == 0x0)) {
            queue.reset();
            return 3;
        }

        // Decode Data into 11 bit channels
        decodeData();

        // TROUBLESHOOTING: print out the values of every channel into serial
        // for (int i = 0; i < 16; i++) {
        //     SerialUSB.print(channel[i]);
        //     SerialUSB.print(" ");
        // }
        // SerialUSB.println(" ");

        if(channel[TOGGL_CHNL] == TOGGL_VAL_PC) {
            pc_mode = true;
        } else {
            /* change values*/
            // convert from 11 bit to 8 bit before calling functions
            uint8_t ver_8bit = channel[6] * 255 / 2047; // motors vertical
            uint8_t hor_8bit = channel[7] * 255 / 2047; // motors horizontal
            uint8_t dome_servo_8bit = channel[4] * 255 / 2047; // dome "servo"
            
            // input motor values
            control_motors(ver_8bit, hor_8bit);

            /* change servo values*/
            set_servo_angle(DOME_SERVO_NUM, dome_servo_8bit);

            /* change LED values*/
            

            /* set pc_mode flag to false -> continued
            rc controller use*/
            pc_mode = false;

        }


        // reset the complete_packet buffer
        complete_packet[0] = 0;
    }
    return pc_mode;
}

// The RC Receiver uses a protocol (SBUS) that transmits 11 bit channels
// This function decodes the 11 bit channels from the byte-sized serial transmissions
void decodeData() {
    // this mess decodes the data bytes into actual channel values
    channel[0] = (complete_packet[1] | complete_packet[2] << 8) & 0x7FF;
    channel[1] = (complete_packet[2] >> 3 | complete_packet[3] << 5) & 0x7FF;
    channel[2] = (complete_packet[3] >> 6 | complete_packet[4] << 2 | complete_packet[5] << 10) & 0x7FF;
    channel[3] = (complete_packet[5] >> 1 | complete_packet[6] << 7) & 0x7FF;
    channel[4] = (complete_packet[6] >> 4 | complete_packet[7] << 4) & 0x7FF;
    channel[5] = (complete_packet[7] >> 7 | complete_packet[8] << 1 | complete_packet[9] << 9) & 0x7FF;
    channel[6] = (complete_packet[9] >> 2 | complete_packet[10] << 6) & 0x7FF;
    channel[7] = (complete_packet[10] >> 5 | complete_packet[11] << 3) & 0x7FF;
    channel[8] = (complete_packet[12] | complete_packet[13] << 8) & 0x7FF;
    channel[9] = (complete_packet[13] >> 3 | complete_packet[14] << 5) & 0x7FF;
    channel[10] = (complete_packet[14] >> 6 | complete_packet[15] << 2 | complete_packet[16] << 10) & 0x7FF;
    channel[11] = (complete_packet[16] >> 1 | complete_packet[17] << 7) & 0x7FF;
    channel[12] = (complete_packet[17] >> 4 | complete_packet[18] << 4) & 0x7FF;
    channel[13] = (complete_packet[18] >> 7 | complete_packet[19] << 1 | complete_packet[20] << 9) & 0x7FF;
    channel[14] = (complete_packet[20] >> 2 | complete_packet[21] << 6) & 0x7FF;
    channel[15] = (complete_packet[21] >> 5 | complete_packet[22] << 3) & 0x7FF;
}

void SERCOM2_Handler() {
    // Collect Data from RC Reciever and store in RegCont
    regCont = SERCOM2->USART.DATA.bit.DATA;
    queue.enqueue(regCont);

    // Start TC from Top Value
    TC3->COUNT16.COUNT.reg = 2000;

    // retrigger
    TC3->COUNT16.CTRLBSET.bit.CMD = 0x1;
}

void TC3_Handler() {
    // Clear Flag
    TC3->COUNT16.INTFLAG.bit.OVF = 1;

    // Decode
    newData = true;
}

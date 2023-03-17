#include "rc_data_collection.h"

// PC/RC toggle macros
#define DOME_SERVO 4
#define DOME_SERVO_NUM 0

// channel for determining whether to get input from RC or PC
#define TOGGL_CHNL 5
#define TOGGL_VAL_RC 995  // toggle to RC
#define TOGGL_VAL_PC 1529 // toggle to PC

// REON holoprojector macros
#define REON_CHNL 3    // channel for controlling the REON holoprojectors
#define REON_LOW 461   // maps to REON_OFF
#define REON_MID 995   // maps to REON_ON
#define REON_HIGH 1529 // maps to REON_WHITE

// logic engine macros
#define LOGIC_CHNL 2 // channel for controlling the logic engine

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
volatile boolean new_sbus_packet = false;
// temporary data storage var
volatile uint16_t incoming_rc_byte; // a temporary for the incoming RC data
volatile boolean stayInPC = false;
uint32_t last_sbus_valid_time =
    0; // holds the time (in milliseconds) of the last SBUS packet received
uint32_t lost_rc_frame_count = 0; // the number of lost frames from the transmitter

// Queue class
// Implemented using a circular array
#define QUEUE_BUFFER_SIZE 250 // should not be bigger than a uint8_t (255)
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
        if (data_size == QUEUE_BUFFER_SIZE) {
            return;
        }

        // Insert Data and Increment Head
        buffer[head] = new_data;
        data_size++;
        head++;

        // If head is outside buffer size, go back to start of array
        if (head > QUEUE_BUFFER_SIZE)
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
        if (tail > QUEUE_BUFFER_SIZE)
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
            buffer[tail] = 0;
            data_size--;
            tail++;

            // If tail is outside buffer size, go back to start of array
            if (tail > QUEUE_BUFFER_SIZE)
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

    uint8_t get_data_size() { return data_size; }

private:
    // Circular Array
    uint8_t buffer[QUEUE_BUFFER_SIZE + 1];

    // Head and Tail Markers
    uint8_t head;
    uint8_t tail;

    // Number of Data in Buffer
    uint8_t data_size;

    uint8_t dummy_var;
};

// Queue object (acts as a serial buffer)
Queue queue;

// Array for a Complete SBUS Packet
uint8_t sbus_packet[25];

// Array for channel data
uint16_t channel[16];

void receiver_setup() {
    NVIC_DisableIRQ(SERCOM2_IRQn); // using sercom2

    // INITIALIZING PADS
    // initialize RX pin to be controlled by serial
    PORT->Group[0].PINCFG[11].bit.PMUXEN = 1; // designate RXPIN as controlled by a peripheral

    // initialize TX pin to be controlled by serial
    PORT->Group[0].PINCFG[10].bit.PMUXEN = 1; // designate TXPIN as controlled by a peripheral
    PORT->Group[0].PMUX[5].reg =
        PORT_PMUX_PMUXO_D | PORT_PMUX_PMUXE_D; // set to use multiplexing C function

    // CLOCKING SERIAL
    GCLK->GENDIV.reg =
        GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
        GCLK_GENDIV_ID(4);   // Select Generic Clock (GCLK) 4
    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |
                        GCLK_GENCTRL_GENEN | // enable the clock connection to the peripheral(s)
                        GCLK_GENCTRL_ID(4) | // use clock gen 4
                        GCLK_GENCTRL_SRC_DFLL48M;     // Set Clock to 48 MHz
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |          // enable the clock
                        GCLK_CLKCTRL_GEN_GCLK4 |      // enable generic clock generator 4
                        GCLK_CLKCTRL_ID_SERCOM2_CORE; // set clock to SERCOM2_CORE

    // INITIALIZING SERIAL
    SERCOM2->USART.CTRLA.bit.SWRST = 1; // do a software reset on the serial peripheral
    while (SERCOM2->USART.SYNCBUSY.bit.SWRST)
        ; // wait for synchronization

    SERCOM2->USART.CTRLA.bit.MODE = 1;   // using internal clock
    SERCOM2->USART.CTRLA.bit.CMODE = 0;  // use asynchronous communication
    SERCOM2->USART.CTRLA.bit.RXPO = 0x3; // PA11 multiplex mode d is sercom2 PAD[3]
    SERCOM2->USART.CTRLA.bit.TXPO = 0x2; // similar but is PAD[0]

    SERCOM2->USART.CTRLB.bit.CHSIZE = 0x0; // we have 8 bits of data per USART frame
    SERCOM2->USART.CTRLA.bit.DORD = 1;     // using LSB
    SERCOM2->USART.CTRLA.bit.FORM = 0x1;   // using 1 parity bit
    SERCOM2->USART.CTRLB.bit.PMODE = 0x0;  // using even parity
    SERCOM2->USART.CTRLB.bit.SBMODE = 0x1; // using 2 stop bits
    SERCOM2->USART.BAUD.reg =
        63351; // set the correct baud register value (calculated based on equation in samd21
               // datasheet) SBUS baudrate is 100000, 8 bit data, even parity, 2 stop bits
               // (48MHz / 16) * (1 - BAUD/65536) = 100K, this gives 63351.466 for BAUD
    SERCOM2->USART.CTRLB.bit.RXEN = 0x1; // enable Serial RX
    while (SERCOM2->USART.SYNCBUSY.bit.CTRLB)
        ;                                // wait for sync
    SERCOM2->USART.CTRLB.bit.TXEN = 0x1; // turn tx pin off

    while (SERCOM2->USART.SYNCBUSY.bit.CTRLB)
        ; // wait for sync

    SERCOM2->USART.CTRLA.bit.ENABLE = 1; // enable the serial module
    while (SERCOM2->USART.SYNCBUSY.bit.ENABLE)
        ; // wait for syncronization

    // ENABLING INTERRUPTS
    NVIC_ClearPendingIRQ(SERCOM2_IRQn); // clear any incoming interrupt requests from SERCOM5

    NVIC_SetPriority(SERCOM2_IRQn, 0); // set highest priority for SERCOM2
    NVIC_EnableIRQ(SERCOM2_IRQn);      // enable interrupt requests for SERCOM2
    SERCOM2->USART.INTENSET.reg =
        SERCOM_USART_INTENSET_RXC; // enable RX interrupts for when receiving is complete

    // Initialize Clock for Timer
    GCLK->GENDIV.reg =
        GCLK_GENDIV_DIV(6) | // Divide the 48MHz clock source by divisor 6: 48MHz/6=8MHz
        GCLK_GENDIV_ID(3);   // Select Generic Clock (GCLK) 3
    GCLK->GENCTRL.reg = GCLK_GENCTRL_SRC_DFLL48M | // use DFLL48M as clock source
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
    TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | // use tc in 16 bit mode
                             TC_CTRLA_WAVEGEN_NFRQ;  // use normal counting mode

    // Set Counting Down Mode
    TC3->COUNT16.CTRLBSET.reg =
        TC_CTRLBSET_ONESHOT | // set the countdown condition for setting an interupt when the
                              // counter reaches an overflow or underflow
        TC_CTRLBSET_DIR;      // set the direction to count down

    // synchronization will happen, so we must wait
    while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
        ;

    // enable interrupts
    TC3->COUNT16.INTENSET.bit.OVF = 1; // enable tc overflow/underflow interrupt

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
    static uint16_t logic_eng_idx = -1;

    if (new_sbus_packet) {

        // Disable new_sbus_packet flag
        new_sbus_packet = false;

        // Dequeue values into array
        queue.dequeue_array(25, sbus_packet);

        // Test if packet is valid, reset if not
        if (!(sbus_packet[0] == 0x0F && sbus_packet[24] == 0x0)) {
            queue.reset();
            return false;
        }

        // TROUBLESHOOTING: print out the values of every channel into serial
        // for (int i = 0; i < 16; i++) {
        //     SerialUSB.print(channel[i]);
        //     SerialUSB.print(" ");
        // }
        // SerialUSB.println(" ");
        // Decode Data into 11 bit channels
        // decodeData();
        reverse_decode();
        //reverse_decode2();

        // print out the values of every channel into serial
        // for (int i = 0; i < 16; i++) {
        //     SerialUSB.print(channel[i]);
        //     SerialUSB.print(" ");
        // }
        // SerialUSB.println(" ");

        if (channel[TOGGL_CHNL] == TOGGL_VAL_PC) {
            led_on(LED2);
            pc_mode = true;
        } else if (channel[TOGGL_CHNL] == TOGGL_VAL_RC) {
            // indicate receiver mode
            led_on(LED1);
            led_off(LED2);

            /* motor control */
            // convert from 11 bit to 8 bit before calling control motors
            uint8_t ver_8bit = channel[6] >> 3;
            uint8_t hor_8bit = channel[7] >> 3;
            uint8_t dome_servo_8bit = channel[4] >> 3; // dome "servo"
            // input motor values
            control_motors_joystick(ver_8bit, hor_8bit);

            /* change servo values*/
            set_servo_angle(DOME_SERVO_NUM, dome_servo_8bit);

            /* REON Holoprojector control */
            uint16_t reon_val = channel[REON_CHNL];
            if (reon_val == REON_MID)
                reon_val = REON_ON;
            else if (reon_val == REON_HIGH)
                reon_val = REON_WHITE;
            else
                reon_val = REON_OFF;
            send_reon_command(reon_val, HP_FRNT_ADDR);
            send_reon_command(reon_val, HP_TOP_ADDR);
            send_reon_command(reon_val, HP_REAR_ADDR);

            /* logic engine control */
            uint16_t temp_logic_idx;
            if (logic_eng_idx != (temp_logic_idx = channel[LOGIC_CHNL] * 9 / 2046)) {
                logic_eng_idx = temp_logic_idx;
                sendLogicEngineCommand(logic_eng_idx);
            }

            // stay in receiver mode
            pc_mode = false;
        } else {
            led_off(LED1);
            led_off(LED2);
        }

        // reset the header and footer of the sbus_packet buffer by
        // setting the bytes to incorrect values and this prevents reusing 
        // data from a previous packet
        sbus_packet[0] = 0;
        sbus_packet[24] = 0xff;

        channel[TOGGL_CHNL] = 0;

        // reset the RC timeout timer by recording the time that the last
        // valid SBUS packet was received
        last_sbus_valid_time = millis();

        if ((sbus_packet[23] & 0x04) != 0) { // if the lost frame bit is set
            lost_rc_frame_count++;

            // if the frames are lost for about 1 second (100 frames), then
            // disable the motors
            if (lost_rc_frame_count > 100) {
                led_off(LED1);
                led_off(LED2);
                led_on(LED3);
                led_on(LED4);
                change_motor_speed(0, 0);
                change_motor_speed(1, 0);
                queue.reset();
            }

            lcd.setCursor(0, 1);
            lcd.print("lost RC frames:");
            lcd.print(lost_rc_frame_count);
        } else {
            led_off(LED3);
            led_off(LED4);
            lost_rc_frame_count = 0; // reset the lost frame count
        }

        // lcd.setCursor(0, 3);
        // lcd.print("time:");
        // lcd.print(process_time);
        // lcd.print(" ");
        //print_all_channels();
        //delay(10);
    } else { 
        // if no new data received from the RC receiver in the last 1000ms,
        // then assume the RC receiver is unplugged and stop the motors
        if (millis() > (last_sbus_valid_time + 1000)) {
            led_off(LED1);
            led_off(LED2);
            led_on(LED3);
            led_on(LED4);
            change_motor_speed(0, 0);
            change_motor_speed(1, 0);
            queue.reset();

            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("RC unplugged");
            delay(100);
        } else {
            led_off(LED3);
            led_off(LED4);
        }
    }

    return pc_mode;
}

// The RC Receiver uses a protocol (SBUS) that transmits 11 bit channels
// This function decodes the 11 bit channels from the byte-sized serial transmissions
void reverse_decode() {
    uint32_t *temp_ptr;

    // The ARM architecture automatically reverses the byte order when loading a
    // uint32_t.  This means we do not have to reverse the byte order manually as
    // long as a pointer to a uint32_t is used to reference the data.

    // handle original bytes 1-4
    temp_ptr = (uint32_t *)&sbus_packet[1];
    channel[0] = (*temp_ptr) & 0x7ff;
    channel[1] = (*temp_ptr >> 11) & 0x7ff;

    // handle original bytes 3-6
    temp_ptr = (uint32_t *)&sbus_packet[3];
    channel[2] = (*temp_ptr >> 6) & 0x7ff;
    channel[3] = (*temp_ptr >> 17) & 0x7ff;

    // handle original bytes 6-9
    temp_ptr = (uint32_t *)&sbus_packet[6];
    channel[4] = (*temp_ptr >> 4) & 0x7ff;

    // handle original bytes 7-10
    temp_ptr = (uint32_t *)&sbus_packet[7];
    channel[5] = (*temp_ptr >> 7) & 0x7ff;
    channel[6] = (*temp_ptr >> 18) & 0x7ff;

    // handle original bytes 10-13
    temp_ptr = (uint32_t *)&sbus_packet[10];
    channel[7] = (*temp_ptr >> 5) & 0x7ff;
    channel[8] = (*temp_ptr >> 16) & 0x7ff;

    // handle original bytes 13-16
    temp_ptr = (uint32_t *)&sbus_packet[13];
    channel[9] = (*temp_ptr >> 3) & 0x7ff;
    channel[10] = (*temp_ptr >> 14) & 0x7ff;

    // handle original bytes 16-19
    temp_ptr = (uint32_t *)&sbus_packet[16];
    channel[11] = (*temp_ptr >> 1) & 0x7ff;
    channel[12] = (*temp_ptr >> 12) & 0x7ff;

    // handle original bytes 18-21
    temp_ptr = (uint32_t *)&sbus_packet[18];
    channel[13] = (*temp_ptr >> 7) & 0x7ff;
    channel[14] = (*temp_ptr >> 18) & 0x7ff;

    // handle original bytes 21-22
    temp_ptr = (uint32_t *)&sbus_packet[21];
    channel[15] = (*temp_ptr >> 5) & 0x7ff;

    // copy channel data to global buffer
    //memcpy(channel, reverse_channel, 16 * sizeof(uint16_t));
}

void reverse_decode2() {
    uint8_t reverse_channel[16];
    uint32_t *temp_ptr32;
    uint64_t *temp_ptr64;
    uint64_t val;

    // This function addresses memory using a 64-bit integer.  The idea
    // is to reverse 8 bytes at a time for more efficiency.

    // handle original bytes 1-8
    temp_ptr64 = (uint64_t *)&sbus_packet[1];
    val = *temp_ptr64;
    channel[0] = (val) & 0x7ff;
    channel[1] = (val >> 11) & 0x7ff;
    channel[2] = (val >> 22) & 0x7ff;
    channel[3] = (val >> 33) & 0x7ff;
    channel[4] = (val >> 44) & 0x7ff;

    // handle original bytes 7-14
    temp_ptr64 = (uint64_t *)&sbus_packet[7];
    val = *temp_ptr64;
    channel[5] = (val >>  7) & 0x7ff;
    channel[6] = (val >> 18) & 0x7ff;
    channel[7] = (val >> 29) & 0x7ff;
    channel[8] = (val >> 40) & 0x7ff;
    channel[9] = (val >> 51) & 0x7ff;

    // handle original bytes 14-21
    temp_ptr64 = (uint64_t *)&sbus_packet[14];
    val = *temp_ptr64;
    channel[10] = (val >>  6) & 0x7ff;
    channel[11] = (val >> 17) & 0x7ff;
    channel[12] = (val >> 28) & 0x7ff;
    channel[13] = (val >> 39) & 0x7ff;
    channel[14] = (val >> 50) & 0x7ff;

    // handle original bytes 21-22
    temp_ptr32 = (uint32_t *)&sbus_packet[21];
    channel[15] = (*temp_ptr32 >> 5) & 0x7ff;

    // print out the results to compare the optimized vs. original version
    // lcd.setCursor(0,0);
    // lcd.print(channel[15]);
    // lcd.print(" ");
    // lcd.print(reverse_channel[15]);
    // lcd.print(" ");

    // lcd.print(channel[14]);
    // lcd.print(" ");
    // lcd.print(reverse_channel[14]);
}

void SERCOM2_Handler() {
    // This interrupt handler is called every 120us after a new
    // byte is received from the RC receiver

    // Collect data from RC receiver and store in incoming_rc_byte
    incoming_rc_byte = SERCOM2->USART.DATA.bit.DATA;
    queue.enqueue(incoming_rc_byte);

    // Start TC from Top Value - this interrupt will trigger if no new bytes
    // are received after 2000/8MHz = 250us
    TC3->COUNT16.COUNT.reg = 2000; 

    // retrigger
    TC3->COUNT16.CTRLBSET.bit.CMD = 0x1;
}

void TC3_Handler() {
    // This interrupt handler is triggered after the last byte of an SBUS
    // packet is received.  The serial input will go idle and TC3 countdown
    // will trigger.

    // Clear Flag
    TC3->COUNT16.INTFLAG.bit.OVF = 1;

    // Decode
    new_sbus_packet = true;
}

void print_all_channels() {
    // display all 16 RC channels to the LCD
    char buf[30];

    for (int i=0; i<4; i++) {
        lcd.setCursor(0,i);
        sprintf(buf, "%4d %4d %4d %4d ", channel[4*i+0], channel[4*i+1],
            channel[4*i+2], channel[4*i+3]);
        lcd.print(buf);
    }
}
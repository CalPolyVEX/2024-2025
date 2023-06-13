#include "rc_data_collection.h"

// RC channels 0,1,3,4,5 produce different ranges from the other channels.
// They produce values from 461-1529.  The other channels produce values
// in the range 174-1815.

// drive motor channels
#define FORWARD_CHNL 6
#define TURNING_CHNL 7

// PC/RC toggle macros
#define DOME_CHNL 14
#define DOME_SERVO_NUM 0

// channel for determining whether to get input from RC or PC
#define TOGGL_CHNL 15
#define TOGGL_VAL_RC 174   // toggle to RC
#define TOGGL_VAL_PC 1815 // toggle to PC

// REON holoprojector macros
#define REON_CHNL 13   // channel for controlling the REON holoprojectors
#define REON_LOW 174     // maps to REON_OFF
#define REON_MID 995   // maps to REON_ON
#define REON_HIGH 1815 // maps to REON_WHITE

// logic engine macros
#define LOGIC_CHNL 2 // channel for controlling the logic engine

// PSI macros
// #define PSI_CHNL 6 // channel for controlling the logic engine [WIP]

// Tsunami
#define TSUNAMI_SELECT_CHNL 8  // the channel to select which sound to play
#define TSUNAMI_TRIGGER_CHNL 0 // the channel to trigger playing a sound
#define TSUNAMI_MIN_VAL 174
#define TSUNAMI_MAX_VAL 1815
#define TSUNAMI_NUM_SOUNDS 3

// SBUS packet format defines
#define SBUS_HEADER_BYTE 0
#define SBUS_FOOTER_BYTE 24
#define SBUS_FRAME_FAILSAFE_BYTE 23

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
    static int print_channel_counter = 0;
    static bool print_channels_mode = false;
    static int receiver_mode_counter = 0;
    static uint32_t last_reon_time = 0;
    static uint32_t last_sound_time = 0;
    static uint32_t current_time = 0;

    // If DB0 is pressed while the code is running, the RC channel information
    // will be displayed on the LCD
    if (print_channel_counter == 300) { //periodically check is button 0 is held down
        print_channel_counter = 0;

        //if the button 0 is held down, then toggle displaying the channels
        if (get_btn_val(DB0) == 0) { 
            print_channels_mode = !print_channels_mode; 
        } 

        //if the button 1 is held down, then disable displaying the channels
        if (get_btn_val(DB1) == 0) {
            print_channels_mode = false;
            lcd.clear();
        } 
    } else {
        print_channel_counter++;
    }

    if (new_sbus_packet) {
        // Reset new_sbus_packet flag
        new_sbus_packet = false;

        // Dequeue values into array
        queue.dequeue_array(25, sbus_packet);

        // Test if packet is valid, reset if not
        if (!(sbus_packet[SBUS_HEADER_BYTE] == 0x0F && sbus_packet[SBUS_FOOTER_BYTE] == 0x0)) {
            queue.reset();
            return false;
        }

        // TROUBLESHOOTING: print out the values of every channel into serial
        //for (int i = 0; i < 16; i++) {
        //    SerialUSB.print(channel[i]);
        //    SerialUSB.print(" ");
        //}
        //SerialUSB.println(" ");

        //Decode Data into 11 bit channels
        reverse_decode2();

        // print out the values of every channel into serial
        // for (int i = 0; i < 16; i++) {
        //      SerialUSB.print(channel[i]);
        //      SerialUSB.print(" ");
        //  }
        //  SerialUSB.println(" ");

        if (channel[TOGGL_CHNL] == TOGGL_VAL_PC) { // if the toggle is set to receive commands from the PC
            // indicate receiving RC signals with a blinking LED
            if (receiver_mode_counter > 105) {
                receiver_mode_counter = 0;
                led_off(LED1);
            } else if (receiver_mode_counter > 100) {
                receiver_mode_counter++;
                led_on(LED1);
            } else {
                receiver_mode_counter++;
            }

            led_on(LED2);
            pc_mode = true;
        } else if (channel[TOGGL_CHNL] == TOGGL_VAL_RC) { // if the toggle is set to receive commands from the RC transmitter
            // indicate receiving RC signals with a blinking LED
            if (receiver_mode_counter > 105) {
                receiver_mode_counter = 0;
                led_off(LED1);
            } else if (receiver_mode_counter > 100) {
                receiver_mode_counter++;
                led_on(LED1);
            } else {
                receiver_mode_counter++;
                led_off(LED2);
            }

            current_time = millis();

            /* motor control */
            // convert from 11 bit to 8 bit before calling control motors
            uint16_t ver_8bit = channel[FORWARD_CHNL] >> 3;
            uint16_t hor_8bit = channel[TURNING_CHNL] >> 3;
            //uint8_t dome_servo_8bit = channel[4] >> 3; // dome "servo"
            //int16_t dome_servo_8bit = (channel[4] - 995) >> 4;
            int16_t dome_servo_8bit = (channel[DOME_CHNL] - 999) >> 4;

            // input motor values
            //SerialUSB.print(dome_servo_8bit);
            //SerialUSB.print("  ");
            control_motors_joystick(ver_8bit, hor_8bit);
            //SerialUSB.print(hor_8bit);
            //SerialUSB.print("  ");
            //SerialUSB.print(ver_8bit);
            //SerialUSB.print("\n");

            /* Set dome rotation speed */
            set_servo_angle(DOME_SERVO_NUM, dome_servo_8bit);

            /* logic engine control */
            uint16_t temp_logic_idx;
            if (logic_eng_idx != (temp_logic_idx = channel[LOGIC_CHNL] * 9 / 2046)) {
                logic_eng_idx = temp_logic_idx;
                SerialUSB.println(logic_eng_idx);
                sendLogicEngineCommand(logic_eng_idx);
            }

            /* Tsunami sound board - limit to play 1 sound file per second */
            if (current_time > (last_sound_time + 1000) && channel[TSUNAMI_TRIGGER_CHNL] > 1000) {
                char buf[10];
                int sound_step = (TSUNAMI_MAX_VAL - TSUNAMI_MIN_VAL) / TSUNAMI_NUM_SOUNDS;
                int sound_val = channel[TSUNAMI_SELECT_CHNL] / sound_step;

                playTsunamiSound(sound_val,10);    
                lcd.clear();
                lcd.print("playing ");

                sprintf(buf, "%4d ", sound_val);
                lcd.print(buf);
                last_sound_time = current_time;
            }

            /* REON Holoprojector control */
            uint16_t reon_val = channel[REON_CHNL];
            if (reon_val == REON_MID)
                reon_val = REON_ON;
            else if (reon_val == REON_HIGH)
                reon_val = REON_WHITE;
            else
                reon_val = REON_OFF;

            // send the REON command every 500ms
            if (millis() > (last_reon_time + 1500)) { 
                send_reon_command(reon_val, HP_FRNT_ADDR);
                send_reon_command(reon_val, HP_TOP_ADDR);
                send_reon_command(reon_val, HP_REAR_ADDR);
                last_reon_time = millis();
            }

            /* PSI control [WIP]*/
            // uint16_t psi_val, temp_psi_val;
            // if (psi_val != (temp_psi_val = channel[PSI_CHNL])) {
            //     psi_val = temp_psi_val;
            //     sendPSICommand(psi_val);
            // }

            // stay in receiver mode
            pc_mode = false;
        } else {
            led_off(LED1);
            led_off(LED2);
        }

        // reset the header and footer of the sbus_packet buffer by
        // setting the bytes to incorrect values and this prevents reusing 
        // data from a previous packet
        sbus_packet[SBUS_HEADER_BYTE] = 0;
        sbus_packet[SBUS_FOOTER_BYTE] = 0xff;
        if (print_channels_mode == true) {
            print_all_channels();
        }

        //reset the RC/PC toggle to an undefined value of 2047
        channel[TOGGL_CHNL] = 2047; 

        // reset the RC timeout timer by recording the time that the last
        // valid SBUS packet was received
        last_sbus_valid_time = millis();

        // if the lost frame bit is set or if the channel data is 
        // set to 640 (transmitter off value)
        if (((sbus_packet[SBUS_FRAME_FAILSAFE_BYTE] & 0x04) != 0) || 
            ((channel[0] == 640) && (channel[1] == 640))) { 
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
                set_servo_angle(0, 0);
                queue.reset();
            }

            lcd.setCursor(0, 1);
            lcd.print("lost RC frames:");
            lcd.print(lost_rc_frame_count);
        } else {
            led_off(LED3);
            led_off(LED4);

            if (lost_rc_frame_count != 0) 
                lcd.clear();
            lost_rc_frame_count = 0; // reset the lost frame count
        }

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
            set_servo_angle(0, 0);
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

void reverse_decode2() {
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
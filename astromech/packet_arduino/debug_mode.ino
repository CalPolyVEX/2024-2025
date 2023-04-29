#include "debug_mode.h"

extern TCA9534 ioex;

uint8_t btn_read(int num) {
    /* reads the value of debug
    button DB1, 2, or 3*/
    //return ioex.input(num);
}

void debug_loop() {
    /* main loop for debug mode 
    controlling is facilitated through
    onboard debug buttons DB1-DB3 */

    while(1) {


        // /* motor control */
        // // convert from 11 bit to 8 bit before calling control motors
        // uint8_t ver_8bit = channel[6] >> 3;
        // uint8_t hor_8bit = channel[7] >> 3;
        // uint8_t dome_servo_8bit = channel[4] >> 3; // dome "servo"
        // // input motor values
        // control_motors_joystick(ver_8bit, hor_8bit);

        // /* change servo values*/
        // set_servo_angle(DOME_SERVO_NUM, dome_servo_8bit);

        // /* logic engine control */
        // uint16_t temp_logic_idx;
        // if (logic_eng_idx != (temp_logic_idx = channel[LOGIC_CHNL] * 9 / 2046)) {
        //     logic_eng_idx = temp_logic_idx;
        //     SerialUSB.println(logic_eng_idx);
        //     sendLogicEngineCommand(logic_eng_idx);
        // }

        // /* soundboard control */
        // // WIP

        // /* REON Holoprojector control */
        // uint16_t reon_val = channel[REON_CHNL];
        // if (reon_val == REON_MID)
        //     reon_val = REON_ON;
        // else if (reon_val == REON_HIGH)
        //     reon_val = REON_WHITE;
        // else
        //     reon_val = REON_OFF;
        // send_reon_command(reon_val, HP_FRNT_ADDR);
        // send_reon_command(reon_val, HP_TOP_ADDR);
        // send_reon_command(reon_val, HP_REAR_ADDR);

        // /* PSI control [WIP]*/
        // // uint16_t psi_val, temp_psi_val;
        // // if (psi_val != (temp_psi_val = channel[PSI_CHNL])) {
        // //     psi_val = temp_psi_val;
        // //     sendPSICommand(psi_val);
        // // }
    }
}


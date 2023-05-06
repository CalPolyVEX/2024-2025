#include "debug_mode.h"
#include "nvm_control.h"

extern TCA9534 ioex;

#define DEBUG_MODE_COUNT 2

struct DebugFunction
{
    // The Address of the NVM Data
    FlashStorageBase* address;

    // The Current Value of the Function
    void* value;

    // The Function Pointer to Update the Value
    void(*func)(void*);

    // The LCD Text to be Displayed When Selected
    char* lcd_text;

    // The Size of the Data in Bytes
    int size;
};

// Motor Speed Debug
FlashStorage(motor_speed_debug, int);
int motor_speed_value = 0;

// Servo Speed Debug
FlashStorage(servo_speed_debug, int);
int servo_speed_value = 0;

// The Look-Up Table
DebugFunction debug_list[DEBUG_MODE_COUNT] = {0};

// The Current Index in the Look-Up Table
int look_up_index = 0;

void initialize_debug()
{
    // Initialize All of the Debug Functions for the Look-Up Table

    // Initialize Motor Speed Debug
    debug_list[0].address = &motor_speed_debug;
    debug_list[0].value = &motor_speed_value;
    debug_list[0].size = sizeof(motor_speed_value);

    // Initialize Servo Speed Debug
    debug_list[1].address = &servo_speed_debug;
    debug_list[1].value = &servo_speed_value;
    debug_list[1].size = sizeof(servo_speed_value);

    // Read the Flash Detection Flag. If 0, Board Has Been Flashed
    FlashStorage(flash_detector, bool);
    bool flashed = flash_detector.read();

    // If The Board Has Been Flashed, Reset to Default and Set Flag to 1
    if (!flashed)
    {
        // Reset Default Values
        reset_to_default();

        // Set the Flag
        flashed = true;
        flash_detector.write(flashed);
    }

    // Else, Read All of the Debug Values
    else
    {
        DebugFunction* end = debug_list + DEBUG_MODE_COUNT;
        for (DebugFunction* it = debug_list; it < end; it++)
          it->address->read(it->value);
    }
}

void reset_to_default()
{
    // Resets All of the Debug Values to Their Defaults
    // Any New Defaults Should be Defined as Macros in the Header
}

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


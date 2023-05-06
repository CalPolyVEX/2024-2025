#include "debug_mode.h"
#include "motor_servo_control.h"

extern TCA9534 ioex;

#define DEBUG_MODE_COUNT 8

struct DebugFunction
{
    // The Address of the NVM Data
    FlashStorageBase* address = 0;

    // The LCD Text to be Displayed When Selected
    char* lcd_text = 0;

    // The Current Value of the Function
    int value = 0;
};

// Motor Min/Max Speed Debug
FlashStorage(min_max_motor_speed_debug, int);
char* min_max_motor_speed_text = "Min/Max Motor Speed: ";

// Motor Speed Scalar Debug
FlashStorage(motor_speed_scalar_debug, int);
char* motor_speed_scalar_text = "Motor Speed Scalar: ";

// Servo Min/Max Speed Debug
FlashStorage(min_max_servo_speed_debug, int);
char* min_max_servo_speed_text = "Min/Max Servo Speed: ";

// Servo Speed Scalar Debug
FlashStorage(servo_speed_scalar_debug, int);
char* servo_speed_scalar_text = "Servo Speed Scalar: ";

// Logic Engine Debug
FlashStorage(logic_engine_debug, int);
char* logic_engine_text = "Logic Engine:";

// Tsunami Debug
FlashStorage(tsunami_debug, int);
char* tsunami_text = "Tsunami:";

// Holoprojector Debug
FlashStorage(hp_debug, int);
char* hp_text = "Holoprojector:";

// PSI Debug
FlashStorage(psi_debug, int);
char* psi_text = "PSI:";

// The Look-Up Table
DebugFunction debug_list[DEBUG_MODE_COUNT];

// The Current Index in the Look-Up Table
int look_up_index = 0;

void initialize_debug()
{
    // Initialize All of the Debug Functions for the Look-Up Table

    // Initialize Motor Speed Debug
    debug_list[0].address = &min_max_motor_speed_debug;
    debug_list[0].lcd_text = min_max_motor_speed_text;

    // Initialize Servo Speed Debug
    debug_list[1].address = &min_max_servo_speed_debug;
    debug_list[1].lcd_text = min_max_servo_speed_text;

    // Initialize Motor Speed Scalar Debug
    debug_list[2].address = &motor_speed_scalar_debug;
    debug_list[2].lcd_text = motor_speed_scalar_text;

    // Initialize Servo Speed Scalar Debug
    debug_list[3].address = &servo_speed_scalar_debug;
    debug_list[3].lcd_text = servo_speed_scalar_text;

    // Initialize Logic Engine Debug
    debug_list[4].address = &logic_engine_debug;
    debug_list[4].lcd_text = logic_engine_text;   
    
    // Initialize Tsunami Debug
    debug_list[5].address = &tsunami_debug;
    debug_list[5].lcd_text = tsunami_text;   
    
    // Initialize HP Debug
    debug_list[6].address = &hp_debug;
    debug_list[6].lcd_text = hp_text;   
    
    // Initialize PSI Debug
    debug_list[7].address = &psi_debug;
    debug_list[7].lcd_text = psi_text;

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

    // Else, Read All of the Stored Debug Values
    else
    {
        // Read All of the Debug Values
        DebugFunction* end = debug_list + DEBUG_MODE_COUNT;
        for (DebugFunction* it = debug_list; it < end; it++)
            it->address->read(&it->value);
    }
    
    // Update All the Values in the Code

    // Update Min/Max Motor Speed
    set_max_motor_speed(debug_list[0].value);

    // Update Motor Speed Scalar
    set_motor_speed_scalar(debug_list[1].value);

    // Update Servo Speed
}

void reset_to_default()
{
    // Resets All of the Debug Values to Their Defaults
    // Any New Defaults Should be Defined as Macros in the Header

    // Store Default Min/Max Motor Speed
    debug_list[0].value = 100;

    // Store Default Motor Speed Scalar
    debug_list[1].value = 0;

    // Write Each of the Values
    DebugFunction* end = debug_list + DEBUG_MODE_COUNT;
    for (DebugFunction* it = debug_list; it < end; it++)
        it->address->write(&it->value);
}

uint8_t btn_read(int num) {
    /* reads the value of debug
    button DB1, 2, or 3*/
    return ioex.input(num);
}

void debug_loop() {
    /* main loop for debug mode 
    controlling is facilitated through
    onboard debug buttons DB1-DB3 */
    uint8_t db0, db1, db2, cur_deb_func, trans_state;

    while(1) {

        // read button values
        db0 = btn_read(DB0);
        db1 = btn_read(DB1);
        db2 = btn_read(DB2);

        // establish action

        // old, not necessary due to array-based function indexing
        // switch (cur_deb_func) { // determines current functionality
        //     case TRANS:
        //         switch (trans_state) { // determines current state
        //             case MSD:           // motor speed debug
        //                 if(!db2)
        //                     cur_deb_func = TRANS;
        //                 break;
        //             case SSD:           // servo speed debug
        //                 break;
        //             case LED:           // logic engine debug
        //                 break;
        //             case SBD:           // soundboard debug
        //                 break;
        //             case HPD:           // holoprojector debug
        //                 break;
        //             case PSID:          // PSI debug
        //                 break;
        //         }
        //         break;
        //     case MSD:           // motor speed debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     case SSD:           // servo speed debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     case LED:           // logic engine debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     case SBD:           // soundboard debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     case HPD:           // holoprojector debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     case PSID:          // PSI debug
        //         if(!db2)
        //             cur_deb_func = TRANS;
        //         break;
        //     default:            // safety return to TRANS
        //         cur_deb_func = TRANS;
        //         break;
        // }

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


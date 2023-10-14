#include "debug_mode.h"
#include "motor_servo_control.h"
#include "reon_hp_i2c.h"
#include "logic_engine_control.h"
#include "packet_arduino.h"

extern TCA9534 ioex;

struct DebugFunction
{
    // The Address of the NVM Data
    FlashStorageBase* address;

    /* Function Pointer for Quick Tests of Components */
    void (*funct_ptr)(int);

    // The LCD Text to be Displayed When Selected
    char* lcd_text;

    // The Current Value of the Function
    int value;

    // Minimum Value of Function
    int min_val;

    // Maximum Value of Function
    int max_val;

    // Modifier Value of Function
    int mod_val;
};

// Motor Min/Max Speed Debug
FlashStorage(min_max_motor_speed_debug, int);
char* min_max_motor_speed_text = "Min/Max Motor Spd:";

// Motor Speed Scalar Debug
FlashStorage(motor_speed_scalar_debug, int);
char* motor_speed_scalar_text = "Motor Spd Scalar:";

// Motor Speed Bias Debug
FlashStorage(motor_speed_bias_debug, int);
char* motor_speed_bias_text = "Motor Spd Bias:";

// Motor Acceleration Debug
FlashStorage(motor_acceleration_debug, int);
char* motor_acceleration_text = "Motor Acceleration:";

// Servo Min/Max Speed Debug
FlashStorage(min_max_servo_speed_debug, int);
char* min_max_servo_speed_text = "Min/Max Servo Spd:";

// Servo Speed Scalar Debug
FlashStorage(servo_speed_scalar_debug, int);
char* servo_speed_scalar_text = "Servo Spd Scalar:";

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

/* Empty Function for Debug Functions Without a Test Function */
void emtpy_debug_test_function(int value) {}

void initialize_debug()
{
    // Initialize All of the Debug Functions for the Look-Up Table

    // Initialize Motor Speed Debug
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].address = &min_max_motor_speed_debug;
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].lcd_text = min_max_motor_speed_text;
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].min_val = 0;
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].max_val = 126;
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].mod_val = 2;
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Motor Speed Scalar Debug
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].address = &motor_speed_scalar_debug;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].lcd_text = motor_speed_scalar_text;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].min_val = -30;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].max_val = 30;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].mod_val = 2;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Motor Speed Bias Debug
    debug_list[MOTOR_SPEED_BIAS_DEBUG].address = &motor_speed_bias_debug;
    debug_list[MOTOR_SPEED_BIAS_DEBUG].lcd_text = motor_speed_bias_text;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].min_val = -50;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].max_val = 50;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].mod_val = 2;
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Motor Speed Bias Debug
    debug_list[MOTOR_ACCELERATION_DEBUG].address = &motor_acceleration_debug;
    debug_list[MOTOR_ACCELERATION_DEBUG].lcd_text = motor_acceleration_text;
    debug_list[MOTOR_ACCELERATION_DEBUG].min_val = 10;
    debug_list[MOTOR_ACCELERATION_DEBUG].max_val = 250;
    debug_list[MOTOR_ACCELERATION_DEBUG].mod_val = 2;
    debug_list[MOTOR_ACCELERATION_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Servo Speed Debug
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].address = &min_max_servo_speed_debug;
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].lcd_text = min_max_servo_speed_text;
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].min_val = 0;
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].max_val = 126;
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].mod_val = 2;
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Servo Speed Scalar Debug
    debug_list[SERVO_SPEED_SCALAR_DEBUG].address = &servo_speed_scalar_debug;
    debug_list[SERVO_SPEED_SCALAR_DEBUG].lcd_text = servo_speed_scalar_text;
    debug_list[SERVO_SPEED_SCALAR_DEBUG].min_val = -30;
    debug_list[SERVO_SPEED_SCALAR_DEBUG].max_val = 30;
    debug_list[SERVO_SPEED_SCALAR_DEBUG].mod_val = 2;
    debug_list[SERVO_SPEED_SCALAR_DEBUG].funct_ptr = &emtpy_debug_test_function;

    // Initialize Logic Engine Debug
    debug_list[LOGIC_ENGINE_DEBUG].address = &logic_engine_debug;
    debug_list[LOGIC_ENGINE_DEBUG].lcd_text = logic_engine_text;   
    debug_list[LOGIC_ENGINE_DEBUG].min_val = 0;
    debug_list[LOGIC_ENGINE_DEBUG].max_val = 8;
    debug_list[LOGIC_ENGINE_DEBUG].mod_val = 1;
    debug_list[LOGIC_ENGINE_DEBUG].funct_ptr = &update_logic_engine_debug;
    
    // Initialize Tsunami Debug
    debug_list[SOUNDBOARD_DEBUG].address = &tsunami_debug;
    debug_list[SOUNDBOARD_DEBUG].lcd_text = tsunami_text;   
    debug_list[SOUNDBOARD_DEBUG].min_val = 1;
    debug_list[SOUNDBOARD_DEBUG].max_val = 16;
    debug_list[SOUNDBOARD_DEBUG].mod_val = 1;
    debug_list[SOUNDBOARD_DEBUG].funct_ptr = &update_tsunami_debug;

    // Initialize HP Debug
    debug_list[HOLOPROJECTOR_DEBUG].address = &hp_debug;
    debug_list[HOLOPROJECTOR_DEBUG].lcd_text = hp_text;   
    debug_list[HOLOPROJECTOR_DEBUG].min_val = 0;
    debug_list[HOLOPROJECTOR_DEBUG].max_val = 8;
    debug_list[HOLOPROJECTOR_DEBUG].mod_val = 1;
    debug_list[HOLOPROJECTOR_DEBUG].funct_ptr = &update_holoprojector_debug;

    // Initialize PSI Debug
    debug_list[PSI_DEBUG].address = &psi_debug;
    debug_list[PSI_DEBUG].lcd_text = psi_text;
    debug_list[PSI_DEBUG].min_val = 0;
    debug_list[PSI_DEBUG].max_val = 16;
    debug_list[PSI_DEBUG].mod_val = 1;
    debug_list[PSI_DEBUG].funct_ptr = &update_psi_debug;

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
    set_max_motor_speed(debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].value);

    // Update Motor Speed Scalar
    set_motor_speed_scalar(debug_list[MOTOR_SPEED_SCALAR_DEBUG].value);

    // Update Motor Speed Bias
    set_motor_speed_bias(debug_list[MOTOR_SPEED_BIAS_DEBUG].value);

    // Update Motor Acceleration
    set_motor_acceleration(debug_list[MOTOR_ACCELERATION_DEBUG].value);

    // Update Min/Max Servo Speed
    set_max_servo_speed(debug_list[MIN_MAX_SERVO_SPEED_DEBUG].value);

    // Update Servo Speed Scalar
    set_servo_speed_scalar(debug_list[SERVO_SPEED_SCALAR_DEBUG].value);

    // Update Holoprojector
    /* update_holoprojector_debug(debug_list[HOLOPROJECTOR_DEBUG].value);*/

}

void reset_to_default()
{
    // Resets All of the Debug Values to Their Defaults
    // Any New Defaults Should be Defined as Macros in the Header

    // Store Default Min/Max Motor Speed
    debug_list[MIN_MAX_MOTOR_SPEED_DEBUG].value = 100;

    // Store Default Motor Speed Scalar
    debug_list[MOTOR_SPEED_SCALAR_DEBUG].value = 0;

    // Store Defualt Motor Speed Bias
    debug_list[MOTOR_SPEED_BIAS_DEBUG].value = 0;

    // Store Defualt Motor Acceleration
    debug_list[MOTOR_ACCELERATION_DEBUG].value = 100;

    // Store Default Min/Max Servo SPeed
    debug_list[MIN_MAX_SERVO_SPEED_DEBUG].value = 100;
    
    // Store Default Servo Speed Scalar
    debug_list[SERVO_SPEED_SCALAR_DEBUG].value = 0;

    // Store Default Logic Engine Preset
    debug_list[LOGIC_ENGINE_DEBUG].value = 0;

    // Store Default Tsunami Sound
    debug_list[SOUNDBOARD_DEBUG].value = 1;

    // Store Default Holoprojector Value
    debug_list[HOLOPROJECTOR_DEBUG].value = 0;

    // Store Default PSI Value
    debug_list[PSI_DEBUG].value = 0;

    // Write Each of the Values
    DebugFunction* end = debug_list + DEBUG_MODE_COUNT;
    for (DebugFunction* it = debug_list; it < end; it++)
        it->address->write(&it->value);
}

uint8_t get_btn_val(int num) {
    /* get value of debug button DB0/1/2*/
    return ioex.input(num);
}

uint8_t btn_read(int num, int& btn_delay) {
    /* reads value of debug button and configures
    a delay if pressed*/

    if (!btn_delay)
    {
        int val = get_btn_val(num);
        if (!val)
            btn_delay = 50;
        return val;
    }

    else
    {
        btn_delay--;
        return 1;
    }
}

void debug_loop() {
    /* main loop for debug mode 
    controlling is facilitated through
    onboard debug buttons DB1-DB3 */
    uint8_t db0, db1, db2;
    bool function_engaged=false, mod_check=false;

    int btn0_delay=0, btn1_delay=0, btn2_delay=0;  
    int8_t debug_idx=0;

    // Update Initial LCD Text
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(debug_list[0].lcd_text);
    lcd.setCursor(0,1);
    lcd.print(debug_list[0].value);
    lcd.write(" ");
    lcd.setCursor(0,3);
    lcd.write("...selecting...");

    while(1) {

        delay(5);

        // read button values and set button delays
        db0 = btn_read(DB0, btn0_delay);
        db1 = btn_read(DB1, btn1_delay);
        db2 = btn_read(DB2, btn2_delay);

        if(!function_engaged) {
            // debug function selection
            if(!db2) {
                // lock in debug function
                function_engaged = true;
                mod_check = true;
            } else if(!db1) {
                // navigate to next function
                debug_idx++;
                if(debug_idx >= DEBUG_MODE_COUNT)
                    debug_idx = 0;
            } else if(!db0) {
                // navigate to previous function
                debug_idx--;
                if(debug_idx < 0)
                    debug_idx = DEBUG_MODE_COUNT - 1;
            }
        } else {
            // debug function usage
            if(!db2) {
                // lock in debug values, exit to selection menu
                function_engaged = false;
                mod_check = false;

                // Write to Flash
                debug_list[debug_idx].address->write(&debug_list[debug_idx].value);

                // If Test Function Exists, Call It
                debug_list[debug_idx].funct_ptr(debug_list[debug_idx].value);

            } else if(!db0) {
                // decrease value
                debug_list[debug_idx].value -= debug_list[debug_idx].mod_val;
                if(debug_list[debug_idx].value < debug_list[debug_idx].min_val) {
                    // overshooting case handling
                    debug_list[debug_idx].value = debug_list[debug_idx].min_val;
                    // indicate minimum reached
                    lcd.setCursor(0, 2);
                    lcd.write("Min value!");
                    delay(500);
                }
            } else if(!db1) {
                // increase value
                debug_list[debug_idx].value += debug_list[debug_idx].mod_val;
                if(debug_list[debug_idx].value > debug_list[debug_idx].max_val) {
                    // overshooting case handling
                    debug_list[debug_idx].value = debug_list[debug_idx].max_val;
                    // indicate maximum reached
                    lcd.setCursor(0, 2);
                    lcd.write("Max value!");
                    delay(500);
                }
            }
        }

        // Only Update LCD if Buttons Were Pressed
        if (!db0 || !db1 || !db2) {
            // update LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.write(debug_list[debug_idx].lcd_text);
            lcd.setCursor(0,1);
            lcd.print(debug_list[debug_idx].value);
            lcd.write(" ");
            lcd.setCursor(0,3);
            if(mod_check)
                lcd.write("...modifying...");
            else
                lcd.write("...selecting...");
        }
    }
}
#ifndef REDBOARD_MAIN_H
#define REDBOARD_MAIN_H

#include "logic_engine_control.h"
#include "pc_decoder.h"
#include "psi_control.h"
#include "rc_data_collection.h"
#include "reon_hp_i2c.h"
#include "debug_mode.h"
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include "nvm_control.h"
#include "tsunami_control.h"

#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_ADDRESS 0x27

#define DEBUG_DUMMY 1 // temporary variable for debug switch status

extern hd44780_I2Cexp lcd; // for pc_decoder

#endif

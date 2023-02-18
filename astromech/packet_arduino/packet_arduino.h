#ifndef REDBOARD_MAIN_H
#define REDBOARD_MAIN_H

#include "pc_decoder.h"
#include "rc_data_collection.h"
#include "logic_engine_control.h"
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

#include "tsunami_control.h"

#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_ADDRESS 0x27

extern hd44780_I2Cexp lcd; // for pc_decoder

#endif

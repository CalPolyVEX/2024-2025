#include "CRCdecoder.h"

void setup()
{
  // Set up the motors
  motor_setup();
  int status;
  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) // non zero status means it was unsuccesful
  {
    // hd44780 has a fatalError() routine that blinks an led if possible
    // begin() failed so blink error code using the onboard LED if possible
    hd44780::fatalError(status); // does not return
  }

  // Set up the USB reading
  SerialUSB.begin(38400);
}

void loop()
{
  // put your main code here, to run repeatedly:
  get_input();
}

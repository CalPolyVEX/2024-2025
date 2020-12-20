#include <cstdarg>

#define LCD_E 2    //PA14
#define CLK_LCD 3  //PA09
#define DATA_LCD 4 //PA08

#define PIN_LCD_E PORT_PA14    //PA14
#define PIN_CLK_LCD PORT_PA09  //PA09
#define PIN_DATA_LCD PORT_PA08 //PA08

static uint8_t s_lcd_line = 0;

void lcdInit(void) {
  pinMode(LCD_E,OUTPUT);
  pinMode(CLK_LCD,OUTPUT);
  pinMode(DATA_LCD,OUTPUT);

  digitalWrite(LCD_E,LOW);
  digitalWrite(CLK_LCD,LOW);
  digitalWrite(DATA_LCD,LOW);

  // // NOTE: Using delay() here doesn't work for some reason, so we use delayMicroseconds instead
  delayMicroseconds(15000);
  // we start in 8bit mode, try to set 4 bit mode
  lcdWrite4Bits(0x03, true);
  delayMicroseconds(4500); // wait min 4.1ms
  // second try
  lcdWrite4Bits(0x03, true);
  delayMicroseconds(4500); // wait min 4.1ms
  // third go!
  lcdWrite4Bits(0x03, true);
  delayMicroseconds(150);
  // finally, set to 4-bit interface
  lcdWrite4Bits(0x02, true);
  // finally, set # lines, font size, etc.
  lcdWrite(0x28, true);
  // turn the display on with no cursor or blinking default
  lcdWrite(0x0C, true);
  // clear it off
  lcdClear();
  // set the entry mode
  lcdWrite(0x06, true);
}

void lcdWrite4Bits(uint8_t data, bool is_control)
{
  //   const uint8_t oV = PORTB & ~(0x60);
  //   const uint8_t dV = oV | 0x40;
  //   const uint8_t cV = oV | 0x20;

  //   if (data & _BV(3)) PORTB = dV;
  //     PORTB = cV; //clock high
  //     PORTB = oV; //clock low
  //   if (data & _BV(2)) PORTB = dV;
  //     PORTB = cV;
  //     PORTB = oV;
  //   if (data & _BV(1)) PORTB = dV;
  //     PORTB = cV;
  //     PORTB = oV;
  //   if (data & _BV(0)) PORTB = dV;
  //     PORTB = cV;
  //     PORTB = oV;

  char mask = 1 << 3;
  for (int count=3; count>=0; count--) 
  {
    if (data & mask)
    {
      REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
      // REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
      // REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
    }
    else
    {
      REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
      // REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
      // REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
    }

    //toggle the clock
    REG_PORT_OUTSET0 = PIN_CLK_LCD; //set data pin high
    delayMicroseconds(1);
    REG_PORT_OUTCLR0 = PIN_CLK_LCD; //set data pin low
    delayMicroseconds(1);

    mask = mask >> 1;
  }

  //   // set the LCD_RS pin
  //   if (is_control) {
  //   } else {
  //     PORTB = dV;
  //   }
  //   PORTB = cV;
  //   PORTB = oV;

  if (is_control)
  {
    REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
    // REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
    // REG_PORT_OUTCLR0 = PIN_DATA_LCD; //set data pin low
  }
  else
  {
    REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
    // REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
    // REG_PORT_OUTSET0 = PIN_DATA_LCD; //set data pin high
  }
  //toggle the clock
  REG_PORT_OUTSET0 = PIN_CLK_LCD; //set data pin high
  delayMicroseconds(1);
  REG_PORT_OUTCLR0 = PIN_CLK_LCD; //set data pin low
  delayMicroseconds(1);

  //   // toggle the LCD_E pin
  //   PORTE |= _BV(PE3);
  //   PORTE &= ~_BV(PE3);
  //   delayMicroseconds(100);
  REG_PORT_OUTSET0 = PIN_LCD_E; //set LCD_E pin high
  delayMicroseconds(1);
  REG_PORT_OUTCLR0 = PIN_LCD_E; //set LCD_E pin low
  delayMicroseconds(100);
}

void lcdWrite(uint8_t data, bool is_control) {
  if (data == '\n') {
    // swap the line between 0 and 1
    s_lcd_line = (s_lcd_line == 1) ? 0 : 1;
    lcdSetCursor(0, s_lcd_line);
    return;
  } else if (data == '\r') {
    return;
  }
  lcdWrite4Bits(data >> 4, is_control);
  lcdWrite4Bits(data, is_control);
}


void lcdSetCursor(uint8_t col, uint8_t row) {
  if (col >= 16 || row >= 2) {
    return;
  }
  const uint8_t addr = 0x80 + row * 0x40 + col;
  lcdWrite(addr, true);
}

void lcdClear(void) {
  // clear the display
  lcdWrite(0x01, true);
  delayMicroseconds(3300);
  s_lcd_line = 0;
}

void lcdPrintf(const char *format, ...) {
  char buf[34];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  for (char *p = &buf[0]; *p; p++) { // emulate cooked mode for newlines
    if (*p == '\n')
      write('\r');
    write(*p);
  }
  va_end(ap);
}

void printFloat(float val, uint8_t precision) {
  // prints val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
  // example: printDouble( 3.1415, 2); // prints 3.14 (two decimal places)

  lcdPrintf("%d", int(val));  //prints the int part
  if ( precision > 0) {
    lcdPrintf("."); // print the decimal point
    unsigned long frac;
    unsigned long mult = 1;
    uint8_t padding = precision - 1;
    while (precision--)
      mult *= 10;

    if (val >= 0)
      frac = (val - int(val)) * mult;
    else
      frac = (int(val) - val ) * mult;
    unsigned long frac1 = frac;
    while ( frac1 /= 10 )
      padding--;
    while (  padding--)
      lcdPrintf("0");
    lcdPrintf("%d", frac);
  }
}

size_t write(uint8_t character) {
  lcdWrite(character, false);
  return 1;
}
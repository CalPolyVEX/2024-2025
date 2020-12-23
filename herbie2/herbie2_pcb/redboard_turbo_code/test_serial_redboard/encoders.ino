#include <SPI.h>
#include "LS7366R.h"

#define ENC1_SS 12 //ss
#define ENC2_SS 13 //ss
#define SPI_CLK 3000000

const unsigned short crctable[256] =
{
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

//////////////////////////////////////////////////
//check_crc - check the crc of the incoming packet
int check_crc(char* data, int len) //len is the length including the CRC 
{
    unsigned short crc = 0;
    unsigned short received_crc;

    received_crc = data[len-2] << 8; //the crc is the last 2 bytes of the packet
    received_crc |= data[len-1];

    //Calculates CRC16 of the (n-2) bytes of data in the packet
    for (int byte = 0; byte < (len-2); byte++)
    {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
    }

    if (crc == received_crc) 
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//////////////////////////////////////////////////
//init_motors
void init_motors() {
    // Set GCLK5's prescaler
    GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                       GCLK_GENDIV_ID(5);   // Select Generic Clock (GCLK) 5
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Configure GCLK5 to use DFLL48M
    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC |         // Set the duty cycle to 50/50 HIGH/LOW
                        GCLK_GENCTRL_GENEN |       // Enable GCLK5
                        GCLK_GENCTRL_SRC_DFLL48M | // Set the 48MHz clock source
                        GCLK_GENCTRL_ID(5);        // Select GCLK5
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Connect GCLK5 to TCC0, TCC1
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |     // Enable GCLK5 to TCC0 and TCC1
                        GCLK_CLKCTRL_GEN_GCLK5 | // Select GCLK5
                        GCLK_CLKCTRL_ID_TCC0_TCC1; // Feed the GCLK5 to TCC0 and TCC1
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Set prescaler TCCDiv for TCC1
    TCC1->CTRLA.reg |= TCC_CTRLA_PRESCALER(TCC_CTRLA_PRESCALER_DIV1_Val); //set prescalar to divide by 1

    //Use Normal PWM
    TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (TCC1->SYNCBUSY.bit.WAVE) {};

    //The PER register determines the period of the PWM
    uint32_t period = 2400; //48000000/2400 = 20KHz

    TCC1->PER.reg = period;  //this is a 24-bit register
    while (TCC1->SYNCBUSY.bit.PER) {};

    ///////////////////////////////
    //Left Motor

    //Set the duty cycle to 50%
    TCC1->CC[1].reg = period / 2; //this set the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC1) {};

    //set PA07 to output
    PORT->Group[0].DIRSET.reg = PORT_PA07; //set the direction to output
    PORT->Group[0].OUTCLR.reg = PORT_PA07; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[7].reg |= PORT_PINCFG_PMUXEN;

    // Set PA07's function to function E. Function E is TCC1/WO[1] for PA07.
    // Because this is an odd numbered pin the PMUX is O (odd) and the PMUX
    // index is pin number - 1 / 2, so 3.
    PORT->Group[0].PMUX[3].reg = PORT_PMUX_PMUXO_E;

    //set motor dir PA16 to output
    PORT->Group[0].DIRSET.reg = PORT_PA16; //set the direction to output
    PORT->Group[0].OUTCLR.reg = PORT_PA16; //set the value to output LOW

    ///////////////////////////////
    //Right Motor
    //Set the duty cycle to 25%
    TCC1->CC[0].reg = period / 4; //this sets the on time of the PWM cycle
    while (TCC1->SYNCBUSY.bit.CC0) {};

    //set PA06 to output
    PORT->Group[0].DIRSET.reg = PORT_PA06; //set the direction to output
    PORT->Group[0].OUTCLR.reg = PORT_PA06; //set the value to output LOW

    /* Enable the peripheral multiplexer for the pins. */
    PORT->Group[0].PINCFG[6].reg |= PORT_PINCFG_PMUXEN;

    // Set PA06's function to function E. Function E is TCC1/WO[0] for PA06.
    // Because this is an even numbered pin the PMUX is E (even) and the PMUX
    // index is pin number / 2, so 3.
    PORT->Group[0].PMUX[3].reg |= PORT_PMUX_PMUXE_E;

    //set motor dir PA18 to output
    PORT->Group[0].DIRSET.reg = PORT_PA18; //set the direction to output
    PORT->Group[0].OUTCLR.reg = PORT_PA18; //set the value to output LOW

    ///////////////////////////////
    //Enable TCC1
    TCC1->CTRLA.reg |= (TCC_CTRLA_ENABLE);
    while (TCC1->SYNCBUSY.bit.ENABLE) {};
}

void set_motor_speed(int motor_num, int speed) 
{
    int val;
    int dir = 1; //set the default direction

    if (speed > 2400) //check input speed for out of bounds
    {
        speed = 2400;
    }
    else if (speed < -2400) 
    {
        speed = -2400;
    }

    if (speed < 0) 
    {
        dir = 0;
        val = -speed; //max timer value is 2400, but must be positive
    }
    else 
    {
        val = speed;
    }

    if (motor_num == 0) //left
    {
        if (dir == 1) 
        {
            PORT->Group[0].OUTSET.reg = PORT_PA16; //set the value to output HIGH
        }
        else 
        {
            PORT->Group[0].OUTCLR.reg = PORT_PA16; //set the value to output LOW
        }

        TCC1->CC[1].reg = val; //this set the on time of the PWM cycle
        while (TCC1->SYNCBUSY.bit.CC1) {};
    }
    else if (motor_num == 1) //right
    {
        if (dir == 1) //set the direction
        {
            PORT->Group[0].OUTSET.reg = PORT_PA18; //set the value to output HIGH
        }
        else 
        {
            PORT->Group[0].OUTCLR.reg = PORT_PA18; //set the value to output LOW
        }

        TCC1->CC[0].reg = val; //this sets the on time of the PWM cycle
        while (TCC1->SYNCBUSY.bit.CC0) {};
    }
}

//////////////////////////////////////////////////
//compute_crc - compute a new crc for sending out the encoder packet
unsigned short compute_crc(unsigned char *buf, int len) 
{
    unsigned short crc = 0;

    //Calculates CRC16 of nBytes of data in byte array message
    for (int byte = 0; byte < len; byte++)
    {
        crc = (crc << 8) ^ crctable[((crc >> 8) ^ buf[byte])];
    }

    return crc;
}

//*************************************************
//Channel Encoder Slave Select Control
//*************************************************
void setSSEnc(int enable, int encoder)
//*************************************************
{
   if(encoder == 1) {
       if (enable == 1)
           REG_PORT_OUTCLR0 = PORT_PA19;   //PA19
       else
           REG_PORT_OUTSET0 = PORT_PA19;   //PA19
   } else { //encoder 2
       if (enable == 1)
           REG_PORT_OUTCLR0 = PORT_PA17;   //PA17
       else
           REG_PORT_OUTSET0 = PORT_PA17;   //PA17
   }
} //end func

void clearStrReg(int encoder)
//*************************************************
{
   SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
   setSSEnc(SPI_ENABLE, encoder);
   SPI.transfer(CLR_STR);// Select STR || CLEAR register
   setSSEnc(SPI_DISABLE, encoder);
   SPI.endTransaction();
} //end func

void rstEncCnt(int encoder)
{
   SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
   setSSEnc(SPI_ENABLE, encoder);
   SPI.transfer(CLR_CNTR);
   setSSEnc(SPI_DISABLE, encoder);
   SPI.endTransaction();
} //end func

void init_encoders() 
{
    //set SS pins to high
    pinMode(ENC1_SS,OUTPUT);
    pinMode(ENC2_SS,OUTPUT);
    REG_PORT_OUTSET0 = PORT_PA19; //PA19
    REG_PORT_OUTSET0 = PORT_PA17; //PA17

    SPI.begin();
    delay(100);
    //SPI.usingInterrupt(20);  //SPI will be access during Timer 5 interrupts

    //LS7366 notes
    //1.  data transferred MSB first
    //2.  data is latched from MOSI (into the LS7366) on the rising clock edge 
    //3.  SAMD21 SPI Mode 0 data transfer clocking
    //4.  choose software controlled slave select

    //on the Redboard Turbo
    //SCK - PB11 (SERCOM4/PAD3)
    //MOSI - PB10 (SERCOM4/PAD2)
    //MISO - PA12 (SERCOM4/PAD0 - ALT)

    //initialize the 2 encoders
    for (int num = 1; num <= 2; num++)
    {
        //Set MDR0
        SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
        delay(1);
        setSSEnc(SPI_ENABLE, num);
        SPI.transfer(WRITE_MDR0);  // Select MDR0 | WR register
        // Filter clock division factor = 2 || Asynchronous Index ||
        // disable index || free-running count mode || x4 quadrature count mode
        SPI.transfer(FILTER_2 | DISABLE_INDX | FREE_RUN | QUADRX4); 
        setSSEnc(SPI_DISABLE, num);
        delay(1);

        //Set MDR1
        setSSEnc(SPI_ENABLE, num);
        SPI.transfer(WRITE_MDR1);       // Select MDR1 | WR register
        SPI.transfer(BYTE_4 | EN_CNTR); //4-byte counter mode || Enable counting
        setSSEnc(SPI_DISABLE, num);
        delay(1);

        setSSEnc(SPI_ENABLE, num);
        SPI.transfer(CLR_CNTR); // Select CNTR || CLEAR register
        setSSEnc(SPI_DISABLE, num);
        delay(1);

        clearStrReg(num); //reseting the counter value inside the encoder chips to 0
        delay(1);
        rstEncCnt(num);
        SPI.endTransaction();
    }
}

unsigned char readMDR1(int encoder) 
{
    unsigned char mdr1;

    SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
    setSSEnc(SPI_ENABLE, encoder);
    SPI.transfer(READ_MDR0);    // Select MDR1
    mdr1 = SPI.transfer(0x00);  // read MDR1
    setSSEnc(SPI_DISABLE, encoder);
    SPI.endTransaction();

    return mdr1;
}

//*************************************************
//*****************************************************
long getChanEncoderValue(int encoder)
//*****************************************************
{
    unsigned int cnt1Value, cnt2Value, cnt3Value, cnt4Value;
    long result;
    unsigned char buf[5];

    SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
    setSSEnc(SPI_ENABLE, encoder);

    buf[0] = READ_CNTR;
    SPI.transfer(buf,5); //transfer 5 bytes with the first byte being the command READ_CNTR

    setSSEnc(SPI_DISABLE, encoder);
    SPI.endTransaction();

    cnt1Value = buf[1];
    cnt2Value = buf[2];
    cnt3Value = buf[3];
    cnt4Value = buf[4];

    result = ((long)cnt1Value << 24) + ((long)cnt2Value << 16) + ((long)cnt3Value << 8) + (long)cnt4Value;

    return result;
}
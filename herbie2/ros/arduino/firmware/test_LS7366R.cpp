#include <SPI.h>

#include <Arduino.h>
#include "LS7366R.h"

#define LEFT_ENCODER_INPUT 4
#define RIGHT_ENCODER_INPUT 6

//JS
long int encoder_values[2] = {0,0};

//function prototypes
void blinkActLed(uint8_t state);
long getChanEncoderValue(int encoder);
uint8_t getChanEncoderReg(uint8_t opcode, uint8_t encoder);
void rstEncCnt(uint8_t encoder);
void setSSEnc(bool enable, int encoder);
void clearStrReg(int encoder);
void setSSEncCtrlBits(int value);
void Init_LS7366Rs(void);

void setDFlagChMux(int encoder);

void loadRstReg(unsigned char op_code);
void writeSingleByte(unsigned char op_code, unsigned char data);
unsigned char readSingleByte(unsigned char op_code);

//Global Variables
uint8_t DFlagCh;

//*************************************************
//*****************************************************
void setup()
//*****************************************************
{
    uint8_t a=0;

    pinMode(LED_ACT_pin, OUTPUT);

    pinMode(CLK_SEL_DFAG_pin, OUTPUT);
    pinMode(EN_ENC_SS_pin, OUTPUT);

    pinMode(nSS_ENC_A2_pin, OUTPUT);
    pinMode(nSS_ENC_A1_pin, OUTPUT);
    pinMode(nSS_ENC_A0_pin, OUTPUT);

    digitalWrite(LED_ACT_pin, LOW);

    digitalWrite(CLK_SEL_DFAG_pin, LOW);
    digitalWrite(EN_ENC_SS_pin, LOW);

    digitalWrite(nSS_ENC_A2_pin, HIGH);
    digitalWrite(nSS_ENC_A1_pin, HIGH);
    digitalWrite(nSS_ENC_A0_pin, HIGH);

    pinMode(DFLAG_pin, INPUT);
    pinMode(LFLAG_pin, INPUT_PULLUP);

    //initialize the register in all of the 6 chips
    Init_LS7366Rs();

    //set Ch1 DFLAG to INT1
    DFlagCh = 1;
    setDFlagChMux(DFlagCh);

    Serial.begin(115200);
    //end JS
} //end func

void compute_crc(uint8_t* data, uint8_t len) {
  uint16_t crc=0;

  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < (len-2); byte++) {
    crc = crc ^ ((uint16_t)data[byte] << 8);
    for (unsigned char bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }

  data[len-2] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[len-1] = crc & 0xFF; //send the low byte of the crc
}

void read_values_and_send() {
  uint8_t i;
  uint8_t data_array[12];
  uint8_t counter = 0;

  data_array[0] = 0xff;
  data_array[1] = 0xfe;

  encoder_values[0] = getChanEncoderValue(LEFT_ENCODER_INPUT);
  counter = 2;
  for(i=0;i<4;i++) {
    /* Serial.write(encoder_values[0] & 0xff); */
    data_array[counter] = encoder_values[0] & 0xff;
    counter++;
    encoder_values[0] = encoder_values[0] >> 8;
  }

  encoder_values[1] = getChanEncoderValue(RIGHT_ENCODER_INPUT);
  for(i=0;i<4;i++) {
    /* Serial.write(encoder_values[1] & 0xff); */
    data_array[counter] = encoder_values[1] & 0xff;
    counter++;
    encoder_values[1] = encoder_values[1] >> 8;
  }

  compute_crc(data_array,12);

  for(i=0;i<12;i++) {
    Serial.write(data_array[i]);
  }
}

//*************************************************
//main loop
//*****************************************************
void loop()
//*****************************************************
{ 
    uint8_t a = 0;
    uint8_t tmpStr = 0;
    static uint8_t led_toggle = 0;
    uint16_t i;
    long left, right;

    read_values_and_send();
    
    for (i=0; i<32; i++) {
      delay(1);
    }
} //end loop

//*************************************************
//*****************************************************  
void blinkActLed(uint8_t state)
//*****************************************************
{
   //static boolean LedBlink = false;     
   //LedBlink = !LedBlink;
   if (state == 0)
     digitalWrite(LED_ACT_pin, false);
   else
     digitalWrite(LED_ACT_pin, true);
}

//*************************************************
//*****************************************************  
long getChanEncoderValue(int encoder)
//*****************************************************
{
   unsigned int cnt1Value, cnt2Value, cnt3Value, cnt4Value;
   long result;

   setSSEnc(ENABLE, encoder);

   SPI.transfer(READ_CNTR); // Request count
   cnt1Value = SPI.transfer(0x00); // Read highest order byte
   cnt2Value = SPI.transfer(0x00);
   cnt3Value = SPI.transfer(0x00);
   cnt4Value = SPI.transfer(0x00); // Read lowest order byte

   setSSEnc(DISABLE, 0);

   result = ((long) cnt1Value << 24) + ((long) cnt2Value << 16) + ((long) cnt3Value << 8) + (long) cnt4Value;

   return result;
} //end func

//*************************************************
//*****************************************************
uint8_t getChanEncoderReg(uint8_t opcode, uint8_t encoder)
//*****************************************************
{
   uint8_t Value;

   setSSEnc(ENABLE, encoder);
   SPI.transfer(opcode);
   Value = SPI.transfer(0x00); // Read byte
   setSSEnc(DISABLE, 0);
   return Value;
} //end func

//*************************************************
//*****************************************************
void rstEncCnt(uint8_t encoder)
//*****************************************************
{
   //setSSEnc(DISABLE, encoder);
   //JS
   setSSEnc(ENABLE, encoder);
   //end JS

   SPI.transfer(CLR_CNTR);
   setSSEnc(DISABLE, 0);
} //end func

//*************************************************
//Channel Encoder Slave Select Control
//*************************************************
void setSSEnc(bool enable, int encoder)
//*************************************************
{
   if(encoder>0)
      setSSEncCtrlBits(encoder-1);
		
   if(enable)
      digitalWrite(EN_ENC_SS_pin, HIGH);
   else
      digitalWrite(EN_ENC_SS_pin, LOW);
} //end func

//*************************************************
//*************************************************
void setSSEncCtrlBits(int value)
//*************************************************
//*************************************************
{   
   //set the bits to enable the correct encoder
   if (value & 0x4) {
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
   } else {
      digitalWrite(nSS_ENC_A2_pin, LOW);
   }

   if (value & 0x2) {
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
   } else {
      digitalWrite(nSS_ENC_A1_pin, LOW);
   }

   if (value & 0x1) {
      digitalWrite(nSS_ENC_A0_pin, HIGH); 
   } else {
      digitalWrite(nSS_ENC_A0_pin, LOW);
   }
} //end func

//*************************************************
//*************************************************
void clearStrReg(int encoder)
//*************************************************
{
   setSSEnc(ENABLE, encoder);
   SPI.transfer(CLR_STR);// Select STR || CLEAR register
   setSSEnc(DISABLE, 0);
} //end func

//*************************************************
//*************************************************
void setDFlagChMux(int encoder)
//*************************************************
{
   setSSEncCtrlBits(encoder);
   //Clock D-FF
   digitalWrite(CLK_SEL_DFAG_pin, LOW);
   digitalWrite(CLK_SEL_DFAG_pin, HIGH);
   digitalWrite(CLK_SEL_DFAG_pin, LOW);	
} //end func

//*************************************************
//*************************************************
// LS7366 Initialization and configuration
//*************************************************
void Init_LS7366Rs(void)
//*************************************************
{
   int a = 1;

   // SPI initialization
   SPI.begin();
   SPI.setClockDivider(SPI_CLOCK_DIV128); // SPI at 125Khz (on 16Mhz clock)
   setSSEnc(DISABLE, 0);
   delay(100);

   //initialize the 6 
   for (a = 1; a <= 6; a++) 
   {
      //Set MDR0
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_MDR0);// Select MDR0 | WR register
      SPI.transfer(FILTER_2|DISABLE_INDX|FREE_RUN|QUADRX4);// Filter clock division factor = 1 || Asynchronous Index || 
      // disable index || free-running count mode || x4 quadrature count mode
      setSSEnc(DISABLE, 0);

      //Set MDR1
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_MDR1);// Select MDR1 | WR register   
      SPI.transfer(CMP_FLAG|BYTE_4|EN_CNTR);//4-byte counter mode || Enable counting || FLAG on CMP (B5 of STR)
      setSSEnc(DISABLE, 0);

      //Set DTR
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_DTR);// Select DTR | WR register
      SPI.transfer(0x00);// DTR MSB
      SPI.transfer(0x00);// DTR 
      SPI.transfer(0x00);// DTR 
      SPI.transfer(0x0A);// DTR LSB
      setSSEnc(DISABLE, 0);

      setSSEnc(ENABLE, a);
      SPI.transfer(LOAD_CNTR);
      setSSEnc(DISABLE, 0);  

      /*Serial.print(" Ch");
        Serial.print(a);
        Serial.print("=");
        Serial.print(getChanEncoderValue(a),HEX);
        Serial.print(";");*/
      //********
      //********      
      setSSEnc(ENABLE, a);
      SPI.transfer(CLR_CNTR);// Select CNTR || CLEAR register
      setSSEnc(DISABLE, 0);
      //********
      //********
      clearStrReg(a);   //reseting the counter value inside the encoder chips to 0
      rstEncCnt(a);
   }	
} //end func

//*************************************************
//*************************************************
void loadRstReg(unsigned char op_code) //dataless write command
//*************************************************
{
   unsigned char spi_data;

   Slave_Select_High; 			    // Keep SS/ High for LS7366 deselect
   Slave_Select_Low; 			    // Switch SS/ low for new command
   SPDR = op_code; 			      // Send command to LS7366
   while (!(SPSR & (1<<SPIF))) // Wait for end of the transmission
   {
   };
   spi_data = SPDR; 		        // Reset SPIF
   Slave_Select_High; 		      // Switch SS/ high for end of command
}

//*************************************************
//*************************************************
void writeSingleByte(unsigned char op_code, unsigned char data) //single byte write command
//*************************************************
{
    unsigned char spi_data;
    Slave_Select_High; 		// Keep SS/ High for LS7366 deselect
    Slave_Select_Low; 		// Switch SS/ low for new command
    SPDR = op_code; 		// Send command to LS7366
    while (!(SPSR & (1<<SPIF))) // Wait for end of the transmission
    {
    };
    spi_data = SPDR; 		// Reset SPIF
    SPDR = data; 			// Send data to be written to LS7366 register
    while (!(SPSR & (1<<SPIF))) // Wait for end of the transmission
    {
    };
    spi_data = SPDR; 		// Reset SPIF
    /*additional bytes can be sent here for multibyte write, e.g., write_DTR*/
    Slave_Select_High; 		// Switch SS/ high for end of command
}
//*************************************************
//*************************************************
unsigned char readSingleByte(unsigned char op_code) //single byte read command
//*************************************************
{
    unsigned char spi_data;
    Slave_Select_High; 		// deselect the the LS7366
    Slave_Select_Low; 		// Switch SS/ low for new command
    SPDR = op_code; 		// send op_code for read to LS7366
    while (!(SPSR & (1<<SPIF))) // Wait for end of transmission
    {
    };
    spi_data = SPDR; 		// Reset SPIF
    SPDR = 0xFF; 			// Start dummy transmission to read data from LS7366
    while (!(SPSR & (1<<SPIF))) // Wait for end of the transmission
    {
    };
    spi_data = SPDR; 		// Reset SPIF
    /*additional bytes can be received here for multibyte read, e.g., read_OTR*/
    Slave_Select_High; 		// Switch SS/ high for end of command
    return spi_data;
}

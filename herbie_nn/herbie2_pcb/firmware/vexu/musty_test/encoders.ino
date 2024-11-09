#include <SPI.h>
#include "LS7366R.h"

#define SPI_CLK 3900000 //for the LS7366, 4.166MHz is fastest clock at 3.3V

extern int num_bytes_received;
extern int num_error_counter;

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

  received_crc = data[len - 2] << 8; //the crc is the last 2 bytes of the packet
  received_crc |= data[len - 1];

  //Calculates CRC16 of the (n-2) bytes of data in the packet
  for (int byte = 0; byte < (len - 2); byte++)
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
  if (encoder == 1) {
    if (enable == 1)
      REG_PORT_OUTCLR1 = PORT_PB04;   //PB04
    else
      REG_PORT_OUTSET1 = PORT_PB04;   //PB04
  } else if (encoder == 2) { //encoder 2
    if (enable == 1)
      REG_PORT_OUTCLR1 = PORT_PB05;   //PB05
    else
      REG_PORT_OUTSET1 = PORT_PB05;   //PB05
  } else if (encoder == 3) {
    if (enable == 1)
      REG_PORT_OUTCLR1 = PORT_PB06;   //PB06
    else
      REG_PORT_OUTSET1 = PORT_PB06;   //PB06
  } else if (encoder == 4) {
    if (enable == 1)
      REG_PORT_OUTCLR1 = PORT_PB07;   //PB07
    else
      REG_PORT_OUTSET1 = PORT_PB07;   //PB07
  }
} //end func

void clearStrReg(int encoder)
{
  setSSEnc(SPI_ENABLE, encoder);
  transferDataSPI(CLR_STR);  //SPI.transfer(CLR_STR);// Select STR || CLEAR register
  setSSEnc(SPI_DISABLE, encoder);
} //end func

void rstEncCnt(int encoder)
{
  setSSEnc(SPI_ENABLE, encoder);
  transferDataSPI(CLR_CNTR);  //SPI.transfer(CLR_CNTR);
  setSSEnc(SPI_DISABLE, encoder);
} //end func

void init_sercom4_spi() {
  // Enable the SERCOM4 clock in the PM // Enable the clock for SERCOM4
  MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM4;

  // Select the GCLK (Generic Clock) for SERCOM4
  GCLK->PCHCTRL[SERCOM4_GCLK_ID_CORE].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;

  // Configure the SCK, MISO, and MOSI pins
  // Using PB13 (SCK), PB14 (MISO), and PB15 (MOSI)
  // Using PORTB so that is Group 1.
  PORT->Group[1].PINCFG[13].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[13 >> 1].reg |= (2 << 4); //PORT_PMUX_PMUXO_C = number 2 shifted to the left 4 bits

  PORT->Group[1].PINCFG[14].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[14 >> 1].reg |= 2; //PORT_PMUX_PMUXE_C

  PORT->Group[1].PINCFG[15].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[15 >> 1].reg |= (2 << 4); //PORT_PMUX_PMUXO_C = number 2 shifted to the left 4 bits

  // Reset the SERCOM4
  SERCOM4->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_SWRST;
  while (SERCOM4->SPI.CTRLA.bit.SWRST || SERCOM4->SPI.SYNCBUSY.bit.SWRST);

  // Configure SERCOM4 for SPI mode
  SERCOM4->SPI.CTRLA.reg = 12 | // Host mode operation
                           SERCOM_SPI_CTRLA_DIPO(2) |            // MISO on Sercom4.2
                           SERCOM_SPI_CTRLA_DOPO(2);             // MOSI on Sercom4.3 and SCK on Sercom4.1

  // Configure SERCOM4 for SPI mode
  SERCOM4->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;  // Enable the receiver

  // Set the clock frequency
  uint64_t baud = ((uint64_t)120000000 / (uint64_t)(2 * SPI_CLK)) - 1;
  SERCOM4->SPI.BAUD.reg = (uint8_t) baud;

  // Enable SERCOM4 for SPI mode
  SERCOM4->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
  while (SERCOM4->SPI.CTRLA.bit.SWRST || SERCOM4->SPI.SYNCBUSY.bit.SWRST);
}

uint8_t transferDataSPI(uint8_t data)
{
  SERCOM4->SPI.DATA.bit.DATA = data; // Writing data into Data register

  while ( SERCOM4->SPI.INTFLAG.bit.RXC == 0 )
  {
    // Waiting Complete Reception
  }

  return SERCOM4->SPI.DATA.bit.DATA;  // Reading data
}

void init_encoders()
{
  //set SS pins to high
  PORT->Group[1].DIRSET.reg = PORT_PB04; //set PB04 to output
  REG_PORT_OUTSET1 = PORT_PB04;

  PORT->Group[1].DIRSET.reg = PORT_PB05; //set PB05 to output
  REG_PORT_OUTSET1 = PORT_PB05;

  PORT->Group[1].DIRSET.reg = PORT_PB06; //set PB06 to output
  REG_PORT_OUTSET1 = PORT_PB06;

  PORT->Group[1].DIRSET.reg = PORT_PB07; //set PB07 to output
  REG_PORT_OUTSET1 = PORT_PB07;

  //SPI.begin();
  init_sercom4_spi();
  delay(100);

  //LS7366 notes
  //1.  data transferred MSB first
  //2.  data is latched from MOSI (into the LS7366) on the rising clock edge
  //3.  SAMD51 SPI Mode 0 data transfer clocking
  //4.  choose software controlled slave select

  //on the SAMD51J19
  //SCK -  PB13 (SERCOM4/PAD1)
  //MISO - PB14 (SERCOM4/PAD2)
  //MOSI - PB15 (SERCOM4/PAD3)

  //initialize the 4 encoders
  for (int num = 1; num <= 4; num++)
  {
    //Set MDR0
    delay(1);
    setSSEnc(SPI_ENABLE, num);
    transferDataSPI(WRITE_MDR0); //SPI.transfer(WRITE_MDR0);  // Select MDR0 | WR register

    // Filter clock division factor = 2 || Asynchronous Index ||
    // disable index || free-running count mode || x4 quadrature count mode
    //SPI.transfer(FILTER_2 | DISABLE_INDX | FREE_RUN | QUADRX4);
    transferDataSPI(FILTER_1 | DISABLE_INDX | FREE_RUN | QUADRX1);

    setSSEnc(SPI_DISABLE, num);
    delay(1);

    //Set MDR1
    setSSEnc(SPI_ENABLE, num);
    transferDataSPI(WRITE_MDR1);  //SPI.transfer(WRITE_MDR1);       // Select MDR1 | WR register
    transferDataSPI(BYTE_4 | EN_CNTR);  //SPI.transfer(BYTE_4 | EN_CNTR); //4-byte counter mode || Enable counting
    setSSEnc(SPI_DISABLE, num);
    delay(1);

    clearStrReg(num);  //clear status register
    delay(1);
    rstEncCnt(num);    //reseting the counter value inside the encoder chips to 0
    delay(1);
  }
}

//*************************************************
//*****************************************************
void getChanEncoderValue(int encoder, unsigned char* buf)
//*****************************************************
{
  setSSEnc(SPI_ENABLE, encoder);

  //transfer 5 bytes with the first byte being the command READ_CNTR
  transferDataSPI(READ_CNTR);
  buf[0] = transferDataSPI(READ_CNTR);  //MSB
  buf[1] = transferDataSPI(READ_CNTR);
  buf[2] = transferDataSPI(READ_CNTR);
  buf[3] = transferDataSPI(READ_CNTR);  //LSB

  setSSEnc(SPI_DISABLE, encoder);
}

uint8_t encoder_buf[4]; //encoder readings are stored here 

int get_encoder_reading(int encoder_num) { //get the encoder reading for a single encoder
  int encoder_reading = 0;

  getChanEncoderValue(encoder_num, encoder_buf);  //readings are stored in the global encoder_buf array

  encoder_reading = encoder_buf[3] | (encoder_buf[2] << 8) | (encoder_buf[1] << 16) | (encoder_buf[0] << 24);

  return encoder_reading;
}

unsigned char read_mdr0(int encoder) {
  unsigned char val;
  setSSEnc(SPI_ENABLE, encoder);

  //transfer 2 bytes with the first byte being the command READ_MDR0
  transferDataSPI(READ_MDR0);
  val = transferDataSPI(READ_MDR0);

  setSSEnc(SPI_DISABLE, encoder);

  return val & 0x7F;  //should return 3
}

void read_4_encoder() {
  uint8_t buf[18];  //4 encoder readings (4 bytes each) and 2 blank bytes
  uint8_t output_buf[40];
  int buf_counter = 0;
  struct cobs_encode_result result;
  
  for (int i=1; i<=4; i++) {
    int reading = get_encoder_reading(i);

    /*if (i == 1) {
      reading = num_bytes_received; //FIXME - for encoder 1, send the number of bytes received from the brain
       
      encoder_buf[0] = (uint8_t) ((reading >> 24) & 0xFF);
      encoder_buf[1] = (uint8_t) ((reading >> 16) & 0xFF);
      encoder_buf[2] = (uint8_t) ((reading >> 8) & 0xFF);
      encoder_buf[3] = (uint8_t) ((reading >> 0) & 0xFF);
    } else if (i == 2) { 
      reading = num_error_counter; //FIXME - for encoder 2, send the number of times less than 4 bytes was received from the brain
      
      encoder_buf[0] = (uint8_t) ((reading >> 24) & 0xFF);
      encoder_buf[1] = (uint8_t) ((reading >> 16) & 0xFF);
      encoder_buf[2] = (uint8_t) ((reading >> 8) & 0xFF);
      encoder_buf[3] = (uint8_t) ((reading >> 0) & 0xFF);
    }*/
    
    buf[buf_counter] = encoder_buf[3];  //data sent LSB first
    buf_counter++;
    buf[buf_counter] = encoder_buf[2];
    buf_counter++;
    buf[buf_counter] = encoder_buf[1];
    buf_counter++;
    buf[buf_counter] = encoder_buf[0];
    buf_counter++;  
  }

  buf[16] = 0;  //send 2 blank bytes for future use
  buf[17] = 0;
   
  result = cobs_encode(output_buf, 40, buf, 18); //encode the data into COBS format
   
  serial_write_to_brain_buffer(output_buf, result.out_len);  //write the COBS packet to the VEX brain 
}

int test_encoder_chips() {
  unsigned int counter = 0;
  unsigned int read_val;

  for (int j = 1; j < 100000; j++) {
    for (int i = 1; i <= 3; i++) {
      //write DTR
      setSSEnc(SPI_ENABLE, i);
      transferDataSPI(WRITE_DTR);
      transferDataSPI((unsigned char) ((counter >> 24) & 0xFF));
      transferDataSPI((unsigned char) ((counter >> 16) & 0xFF));
      transferDataSPI((unsigned char) ((counter >> 8) & 0xFF));
      transferDataSPI((unsigned char) (counter & 0xFF));
      setSSEnc(SPI_DISABLE, i);
      delayMicroseconds(1);

      //transfer DTR to CNTR
      setSSEnc(SPI_ENABLE, i);
      transferDataSPI(LOAD_CNTR);
      setSSEnc(SPI_DISABLE, i);
      delayMicroseconds(1);
      
      //read CNTR
      read_val = 0;
      setSSEnc(SPI_ENABLE, i);

      transferDataSPI(READ_CNTR);
      read_val |= transferDataSPI(READ_CNTR) << 24;
      read_val |= transferDataSPI(READ_CNTR) << 16;
      read_val |= transferDataSPI(READ_CNTR) << 8;
      read_val |= transferDataSPI(READ_CNTR);
     
      setSSEnc(SPI_DISABLE, i);
      delayMicroseconds(1);

      //Serial.print (counter);
      //Serial.print ("  ");
      //Serial.print (read_val);
      //Serial.print ("\n");
      
      if (read_val != counter)
        return 0;  //error

      counter++;
    }
  }

  return 1; //success

}

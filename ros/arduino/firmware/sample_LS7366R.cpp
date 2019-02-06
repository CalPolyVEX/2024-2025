// the LS7366R communicates using SPI, so include the library:
#include <SPI.h>
#include <ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Int32MultiArray.h>

#include <Arduino.h>

//JS
ros::NodeHandle nh;

std_msgs::String str_msg;
std_msgs::Int32MultiArray wheel_enc_msg;
ros::Publisher chatter("chatter", &str_msg);
ros::Publisher encoder("encoder", &wheel_enc_msg);
char hello[13] = "hello world!";
//end JS

#define ENABLE    1
#define DISABLE   0

/***MDR0 configuration data - the configuration byte is formed with***
***single segments taken from each group and ORing all together.***/
//Count modes
#define NQUAD 		0x00 //non-quadrature mode
#define QUADRX1 	0x01 //X1 quadrature mode
#define QUADRX2 	0x02 //X2 quadrature mode
#define QUADRX4 	0x03 //X4 quadrature mode
//Running modes
#define FREE_RUN 	0x00
#define SINGE_CYCLE 0x04
#define RANGE_LIMIT 0x08
#define MODULO_N 	0x0C
//Index modes
#define DISABLE_INDX 	0x00 	//index_disabled
#define INDX_LOADC 		0x10 	//index_load_CNTR
#define INDX_RESETC 	0x20 	//index_rest_CNTR
#define INDX_LOADO 		0x30 	//index_load_OL
#define ASYNCH_INDX 	0x00 	//asynchronous index
#define SYNCH_INDX 		0x80 	//synchronous index
//Clock filter modes
#define FILTER_1 		0x00 	//filter clock frequncy division factor 1
#define FILTER_2 		0x80 	//filter clock frequncy division factor 2
/* **MDR1 configuration data; any of these***
***data segments can be ORed together***/
//Flag modes
#define NO_FLAGS 		0x00 	//all flags disabled
#define IDX_FLAG 		0x10 	//IDX flag
#define CMP_FLAG 		0x20 	//CMP flag
#define BW_FLAG 		0x40 	//BW flag
#define CY_FLAG 		0x80 	//CY flag
//1 to 4 bytes data-width
#define BYTE_4 			0x00 	//four byte mode
#define BYTE_3 			0x01 	//three byte mode
#define BYTE_2 			0x02 	//two byte mode
#define BYTE_1 			0x03 	//one byte mode
//Enable/disable counter
#define EN_CNTR 		0x00 	//counting enabled
#define DIS_CNTR 		0x04 	//counting disabled
/* LS7366R op-code list */
#define CLR_MDR0 		  0x08
#define CLR_MDR1 		  0x10
#define CLR_CNTR 		  0x20
#define CLR_STR 		  0x30
#define READ_MDR0 		0x48
#define READ_MDR1 		0x50
#define READ_CNTR 		0x60
#define READ_OTR 		  0x68
#define READ_STR 		  0x70
#define WRITE_MDR1 		0x90
#define WRITE_MDR0 		0x88
#define WRITE_DTR 		0x98
#define LOAD_CNTR 		0xE0
#define LOAD_OTR 		  0xE4

//the lines are used by 74HC138 chip to select the cable select lines
int nSS_ENC_A2_pin = 10;//C  A2
int nSS_ENC_A1_pin = 9; //B  A1 
int nSS_ENC_A0_pin = 8; //A  A0

//CLK Select DFLAG DF-F
int CLK_SEL_DFAG_pin = 4;

//Enable ENC_SS
int EN_ENC_SS_pin = 5;

//Blue LED
int LED_ACT_pin = 6;

//DFLAG
int DFLAG_pin = 3; 

//LFLAG
int LFLAG_pin = 2;

#define Slave_Select_Low PORTB &= ~(1 << PB4)
#define Slave_Select_High PORTB |= (1 << PB4)

//function prototypes
void blinkActLed(void);
long getChanEncoderValue(int encoder);
unsigned int getChanEncoderReg(int opcode, int encoder);
void rstEncCnt(int encoder);
void setSSEnc(bool enable, int encoder);
void clearStrReg(int encoder);
void setSSEncCtrlBits(int value);
void Init_LS7366Rs(void);

void setDFlagChMux(int encoder);

void loadRstReg(unsigned char op_code);
void writeSingleByte(unsigned char op_code, unsigned char data);
unsigned char readSingleByte(unsigned char op_code);

//Global Variables
int IsrDFlag;
int DFlagCh;
int IsrLFlag;
int LFlagCnt[6];

//*************************************************
//*****************************************************  
void ISR_DFlag()
//*****************************************************
{
    IsrDFlag = 1;	
}
//*************************************************
//*****************************************************  
void ISR_LFlag()
//*****************************************************
{
    IsrLFlag = 1;
}

//*************************************************
//*****************************************************
void setup()
//*****************************************************
{
    int a=0;

    //Serial.begin(9600);

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
    IsrDFlag = 0;
    DFlagCh = 1;
    setDFlagChMux(DFlagCh);

    IsrLFlag = 0;
    for (a = 0; a< 6; a++){   
        LFlagCnt[a] = 0;
    }

    //attachInterrupt(digitalPinToInterrupt(DFLAG_pin), ISR_DFlag, FALLING );
    //attachInterrupt(digitalPinToInterrupt(LFLAG_pin), ISR_LFlag, FALLING );
    //JS
    attachInterrupt(1, ISR_DFlag, FALLING );
    attachInterrupt(0, ISR_LFlag, FALLING );

    //ROS setup
    nh.initNode();
    nh.advertise(chatter);
    nh.advertise(encoder);
    //end JS

} //end func

//*************************************************
//main loop
//*****************************************************
void loop()
//*****************************************************
{ 
    uint8_t a = 0;
    int tmpStr = 0;

    for ( a = 1; a <= 6; a++)
    {    
        //Serial.print(" Ch");
        //Serial.print(a);
        //Serial.print("=");
        //Serial.print(getChanEncoderValue(a),DEC);
        //Serial.print(";");

        //Serial.print(" STR=");
        //Serial.print(getChanEncoderReg(READ_STR,a),BIN);
        //Serial.print(";");
    } 
    //Serial.print("\t");

    //Serial.print(" DFLGCh=");
    //Serial.print(DFlagCh);
    //Serial.print(";");

    //Serial.print(" LFLG=");
    //Serial.print(digitalRead(LFLAG_pin));
    //Serial.print(";");
    //Serial.print(" Cnt=");

    for ( a = 0; a < 6; a++)
    {   
        //Serial.print(LFlagCnt[a]);
        //Serial.print("/");
    }

    //Serial.print(";");
    //Serial.print("\r\n");

    ///////////////
    if(IsrDFlag)
    {
        DFlagCh++;
        if(DFlagCh > 6){
            DFlagCh = 1;
        }
        setDFlagChMux(DFlagCh);//check next encoder
        IsrDFlag = 0;
    }
    ///////////////
    if(IsrLFlag)
    {
        for(int a=1;a<=6;a++){
            tmpStr = getChanEncoderReg(READ_STR,a); 
            tmpStr &= 0b00100000;//test CMP
            if(tmpStr){
                LFlagCnt[a-1]++;
                clearStrReg(a); 
            }
        }
        IsrLFlag = 0;	
    }
    ///////////////
    blinkActLed();

    //JS
    str_msg.data = hello;
    wheel_enc_msg.layout.dim[0].label = "encoder count";
    wheel_enc_msg.layout.dim[0].stride = 1;
    wheel_enc_msg.layout.dim[0].size = 2;
    wheel_enc_msg.layout.data_offset = 0;
    wheel_enc_msg.data[0] = 5;
    wheel_enc_msg.data[1] = 6;
    chatter.publish(&str_msg);
    encoder.publish(&wheel_enc_msg);
    nh.spinOnce();
    delay(33);
    //end JS
} //end loop

//*************************************************
//*****************************************************  
void blinkActLed(void)
//*****************************************************
{
static boolean LedBlink;     
  LedBlink = !LedBlink;
  digitalWrite(LED_ACT_pin, LedBlink);
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
unsigned int getChanEncoderReg(int opcode, int encoder)
//*****************************************************
{
    unsigned int Value;
    
    setSSEnc(ENABLE, encoder);
    SPI.transfer(opcode);
    Value = SPI.transfer(0x00); // Read byte
    setSSEnc(DISABLE, 0);
    return Value;
} //end func

//*************************************************
//*****************************************************
void rstEncCnt(int encoder)
//*****************************************************
  {
    setSSEnc(DISABLE, encoder);
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
//*************************************************//*************************************************
{   
   switch (value) 
   {
    case 0:
      digitalWrite(nSS_ENC_A2_pin, LOW);
      digitalWrite(nSS_ENC_A1_pin, LOW); 
      digitalWrite(nSS_ENC_A0_pin, LOW); 
      break;
      
    case 1:
      digitalWrite(nSS_ENC_A2_pin, LOW); 
      digitalWrite(nSS_ENC_A1_pin, LOW); 
      digitalWrite(nSS_ENC_A0_pin, HIGH); 
      break;
      
    case 2:
      digitalWrite(nSS_ENC_A2_pin, LOW); 
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
      digitalWrite(nSS_ENC_A0_pin, LOW); 
      break;
      
    case 3:
      digitalWrite(nSS_ENC_A2_pin, LOW); 
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
      digitalWrite(nSS_ENC_A0_pin, HIGH); 
      break;
      
    case 4:
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
      digitalWrite(nSS_ENC_A1_pin, LOW); 
      digitalWrite(nSS_ENC_A0_pin, LOW); 
      break;
      
    case 5:
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
      digitalWrite(nSS_ENC_A1_pin, LOW); 
      digitalWrite(nSS_ENC_A0_pin, HIGH); 
      break;
	  
    case 6:
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
      digitalWrite(nSS_ENC_A0_pin, LOW); 
      break;

    case 7:
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
      digitalWrite(nSS_ENC_A0_pin, HIGH); 
      break;
	  
    default:
      digitalWrite(nSS_ENC_A2_pin, HIGH); 
      digitalWrite(nSS_ENC_A1_pin, HIGH); 
      digitalWrite(nSS_ENC_A0_pin, HIGH); 	
	  break;
  } //end switch

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
    SPI.setClockDivider(SPI_CLOCK_DIV128);      // SPI at 125Khz (on 16Mhz clock)
    setSSEnc(DISABLE, 0);
    delay(100);

    //Serial.print("\r\n");
    //Serial.print("\r\n");
    
    //initialize the 6 
    for (a = 1; a <= 6; a++) 
    {
      //********
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_MDR0);// Select MDR0 | WR register
      SPI.transfer(FILTER_2|DISABLE_INDX|FREE_RUN|QUADRX1);// Filter clock division factor = 1 || Asynchronous Index || 
                         // disable index || free-running count mode || x4 quadrature count mode
      setSSEnc(DISABLE, 0);
      
      /*Serial.print(" TX MDR0=");
      Serial.print(FILTER_2|DISABLE_INDX|FREE_RUN|QUADRX1,HEX);
      Serial.print(";");
      Serial.print(" RX MDR0=");
      Serial.print(getChanEncoderReg(READ_MDR0,a),HEX);
      Serial.print(";"); */
      //********
      //********
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_MDR1);// Select MDR1 | WR register   
      SPI.transfer(CMP_FLAG|BYTE_4|EN_CNTR);//4-byte counter mode || Enable counting || FLAG on CMP (B5 of STR)
      setSSEnc(DISABLE, 0);

      /*Serial.print(" TX MDR1=");
      Serial.print(CMP_FLAG|BYTE_4|EN_CNTR,HEX);
      Serial.print(";");
      Serial.print(" RX MDR1=");
      Serial.print(getChanEncoderReg(READ_MDR1,a),HEX);
      Serial.print(";");*/
      //********
      //********
      setSSEnc(ENABLE, a);
      SPI.transfer(WRITE_DTR);// Select DTR | WR register
      SPI.transfer(0x00);// DTR MSB
      SPI.transfer(0x00);// DTR 
      SPI.transfer(0x00);// DTR 
      SPI.transfer(0x0A);// DTR LSB
      setSSEnc(DISABLE, 0);
      //********
      //********
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
       //********
      //********
      /*Serial.print(" STR=");
      Serial.print(getChanEncoderReg(READ_STR,a),BIN);
      Serial.print(";");
      Serial.print("\t");*/
      //********
      //********
      /*Serial.print("\r\n");*/  
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

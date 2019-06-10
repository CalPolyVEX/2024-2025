#include <SPI.h>
#include <ros.h>
#include <std_msgs/Empty.h>
#include <std_msgs/String.h>
#include <std_msgs/Int32MultiArray.h>
#include <rosserial_arduino/Test.h>
#include <std_srvs/Empty.h>

#include <Arduino.h>
#include "LS7366R.h"

#define LEFT_ENCODER_INPUT 6
#define RIGHT_ENCODER_INPUT 4

//JS
ros::NodeHandle nh;
using std_srvs::Empty;
void reset_encoder_callback(const std_msgs::Empty& reset_msg);
void read_encoder_callback(const std_msgs::Empty &reset_msg);

//std_msgs::String str_msg;
std_msgs::Int32MultiArray wheel_enc_msg;
//ros::Publisher chatter("chatter", &str_msg);
ros::Publisher encoder("encoder", &wheel_enc_msg);
ros::Subscriber<std_msgs::Empty> reset_encoder("reset_encoder", &reset_encoder_callback);

//topics to handle the read encoder request
ros::Subscriber<std_msgs::Empty> read_encoder_cmd("read_encoder_cmd", &read_encoder_callback);
ros::Publisher encoder_service("encoder_service", &wheel_enc_msg);
//char hello[13] = "hello world!";
char dim0_label[8] = "encoder";
std_msgs::MultiArrayLayout mal;
std_msgs::MultiArrayDimension mad[1];
long int encoder_values[2] = {0,0};
uint8_t counter = 0;

//testing service code
//void service_cb(const Empty::Request &req, Empty::Response &res);
//ros::ServiceServer<Empty::Request, Empty::Response> test_server("test_service", &service_cb);
//end JS

//function prototypes
void blinkActLed(void);
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
uint8_t IsrDFlag;
uint8_t DFlagCh;
uint8_t IsrLFlag;
//int LFlagCnt[6];

//when a message is received on /read_encoder_cmd, then publish a message
//with the encoder readings
void read_encoder_callback(const std_msgs::Empty &reset_msg){
   encoder_service.publish(&wheel_enc_msg);
}

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

void reset_encoder_callback(const std_msgs::Empty &reset_msg){
   //send an Empty message to the /reset_encoder topic to reset
   //'rostopic pub /reset_encoder std_msgs/Empty --once' to test
   
   //digitalWrite(13, HIGH-digitalRead(13));   // blink the led
   encoder_values[0] = 0;
   encoder_values[1] = 0;
   rstEncCnt(LEFT_ENCODER_INPUT); //reset the left encoder
   rstEncCnt(RIGHT_ENCODER_INPUT); //reset the right encoder
}

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
    IsrDFlag = 0;
    DFlagCh = 1;
    setDFlagChMux(DFlagCh);

    IsrLFlag = 0;
    for (a = 0; a< 6; a++){   
        //LFlagCnt[a] = 0;
    }

    //attachInterrupt(digitalPinToInterrupt(DFLAG_pin), ISR_DFlag, FALLING );
    //attachInterrupt(digitalPinToInterrupt(LFLAG_pin), ISR_LFlag, FALLING );
    //JS
    //attachInterrupt(1, ISR_DFlag, FALLING );
    //attachInterrupt(0, ISR_LFlag, FALLING );

    //ROS setup
    //nh.getHardware()->setBaud(115200);
    nh.initNode();
    //nh.advertise(chatter);
    nh.advertise(encoder);
    nh.subscribe(reset_encoder);
    //nh.advertiseService(test_server);

    //read encoder command
    nh.advertise(encoder_service);
    nh.subscribe(read_encoder_cmd);

    mad[0].label = (char*) &dim0_label;
    mad[0].size = 2;
    mad[0].stride = 1*2;

    mal.dim = mad;
    mal.data_offset = 0;
    mal.dim_length = 1;
    wheel_enc_msg.data_length = 2;
    wheel_enc_msg.layout = mal;
    wheel_enc_msg.data = (long int*) &encoder_values;

    //end JS

} //end func

//*************************************************
//main loop
//*****************************************************
void loop()
//*****************************************************
{ 
    uint8_t a = 0;
    uint8_t tmpStr = 0;
    static uint8_t led_toggle = 0;

    for ( a = 1; a <= 6; a++)
    {    
        //Serial.print(getChanEncoderValue(a),DEC);
        //Serial.print(getChanEncoderReg(READ_STR,a),BIN);
    } 

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
        for(uint8_t a=1;a<=6;a++){
            tmpStr = getChanEncoderReg(READ_STR,a); 
            tmpStr &= 0b00100000;//test CMP
            if(tmpStr){
                //LFlagCnt[a-1]++;
                clearStrReg(a); 
            }
        }
        IsrLFlag = 0;	
    }
    
    //blink the LED on the encoder shield
    counter++;
    if (counter > 254) {
      blinkActLed();
      counter = 0;
      led_toggle = 0;
    } else if (counter > 220 && led_toggle == 0) {
      blinkActLed();
      led_toggle = 1;
    }

    //JS
    //str_msg.data = hello;

    //get the encoder values
    wheel_enc_msg.data[0]=getChanEncoderValue(LEFT_ENCODER_INPUT);
    wheel_enc_msg.data[1]=getChanEncoderValue(RIGHT_ENCODER_INPUT);
    //wheel_enc_msg.data[1]=getChanEncoderReg(READ_STR,1);
    //wheel_enc_msg.data[1]=getChanEncoderReg(READ_MDR0,4);
    //chatter.publish(&str_msg);
    //encoder.publish(&wheel_enc_msg);
    nh.spinOnce();
    delay(2);
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
   //JS
   
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
   
   //end JS
/*   
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
  } */ 
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

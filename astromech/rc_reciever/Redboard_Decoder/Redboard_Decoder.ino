#define DATA_LENGTH 25 //length of SBUS packet
#define PACKET_TEST_DELAY 4
#define CHANNEL_MINIMUM 0x1CD
#define CHANNEL_MAXIMUM 0x5F9

int channel[16]; //for storing the processed analog channels
uint8_t dataBuffer[DATA_LENGTH]; //buffer that contains data bytes
int bufferPosition = 0; //what is the next available byte in the buffer / how many bytes are in the buffer
boolean synchNeeded = true; //does the redboard need to find the start of the next packet again?
boolean channel17;
boolean channel18;
boolean frameLost;
boolean failsafeActive;
void setup()
{
  Serial1.begin(100000, SERIAL_8E2); //SBUS uses a baud rate of 100000 with 2 stop bits
  SerialUSB.begin(9600);
}

void loop()
{
  //SerialUSB.println("loop begun");
  if (Serial1.available() > 0) {

    if (synchNeeded) {
      /* QUICK NOTE
         in order to find the start of an SBUS packet, 2 things have to happen:
          1. There must be a serial byte of 0x0
          2. After the 0x0 byte there must be no new serial data for a time greater than the delay between bytes in a packet
             being sent, but less than the delay between packets (ensuring the 0x0 just read was in fact the footer byte)
         Given these two things, the next 25 bytes over serial should be the start of the next complete SBUS packet
      */

      while (Serial1.available() > 1) { //ensures when the delay is tested, it is tested between the correct bytes
        Serial1.read();
      }

      if (Serial1.read() == 0) { //test our first condition
        if (testDelay()) { //test our second condition
          synchNeeded = false;
        }
      }
    } else { //we are synched
      if (bufferPosition == DATA_LENGTH) {
        if (dataBuffer[0] == 0x0F && dataBuffer[DATA_LENGTH - 1] == 0x0) {
          parseData();
          if(SerialUSB.available() > 0) {
            if(SerialUSB.read() == '1') {
              printData();
            }
          }

          bufferPosition = 0;
          addToBuffer(Serial1.read());
        } else {
          SerialUSB.println("packet wrong!");
          synchNeeded = true;
          bufferPosition = 0;
        }
      } else {
        addToBuffer(Serial1.read());
      }
    }
  }
}

// null -> boolean
//if the number of Serial1 available packets does not change from when the function is called to PACKET_TEST_DELAY ms afterwards
boolean testDelay() {
  int priorAvailable = Serial1.available(); //recording the bytes available at the start of the function call
  delay(PACKET_TEST_DELAY);
  return priorAvailable == Serial1.available(); //return the result of the condition we're testing for
}


//uint_8 -> null
//adds a specified addition to dataBuffer and updates bufferPosition accordingly. Throws
//  an error if a write is attempted at an index greater than dataBuffer's defined largest
//  index
void addToBuffer(uint8_t addition) {
  if (bufferPosition < DATA_LENGTH) {
    dataBuffer[bufferPosition] = addition; //add to buffer
    bufferPosition++; //update buffer position
  } else {
    SerialUSB.println("attempted buffer overload");
    SerialUSB.print("  write attempted at index: ");
    SerialUSB.println(bufferPosition);
  }
}

void parseData() {
  decodeData();
  channel17 = bitRead(dataBuffer[23], 0);
  channel18 = bitRead(dataBuffer[23], 1);
  frameLost = bitRead(dataBuffer[23], 2);
  failsafeActive = bitRead(dataBuffer[23], 3);
}

//decodes raw SBUS data into channel values
void decodeData() {
  //this mess decodes the data bytes into actual channel values
  channel[0] = (dataBuffer[1] | dataBuffer[2] << 8 ) & 0b11111111111;
  channel[1] = (dataBuffer[2] >> 3 | dataBuffer[3] << 5) & 0b11111111111;
  channel[2] = (dataBuffer[3] >> 6 | dataBuffer[4] << 2 | dataBuffer[5] << 10) & 0b11111111111;
  channel[3] = (dataBuffer[5] >> 1 | dataBuffer[6] << 7) & 0b11111111111;
  channel[4] = (dataBuffer[6] >> 4 | dataBuffer[7] << 4) & 0b11111111111;
  channel[5] = (dataBuffer[7] >> 7 | dataBuffer[8] << 1 | dataBuffer[9] << 9) & 0b11111111111;
  channel[6] = (dataBuffer[9] >> 2 | dataBuffer[10] << 6) & 0b11111111111;
  channel[7] = (dataBuffer[10] >> 5 | dataBuffer[11] << 3) & 0b11111111111;
  channel[8] = (dataBuffer[12] | dataBuffer[13] << 8) & 0b11111111111;
  channel[9] = (dataBuffer[13] >> 3 | dataBuffer[14] << 5) & 0b11111111111;
  channel[10] = (dataBuffer[14] >> 6 | dataBuffer[15] << 2 | dataBuffer[16] << 10) & 0b11111111111;
  channel[11] = (dataBuffer[16] >> 1 | dataBuffer[17] << 7) & 0b11111111111;
  channel[12] = (dataBuffer[17] >> 4 | dataBuffer[18] << 4) & 0b11111111111;
  channel[13] = (dataBuffer[18] >> 7 | dataBuffer[19] << 1 | dataBuffer[20] << 9) & 0b11111111111;
  channel[14] = (dataBuffer[20] >> 2 | dataBuffer[21] << 6) & 0b11111111111;
  channel[15] = (dataBuffer[21] >> 5 | dataBuffer[22] << 3) & 0b11111111111;
}

//null -> null
//sends channel values so they are viewable in the serial monitor
void printData() {
  for (int i = 0; i < 16; i++) {
    SerialUSB.print(channel[i], DEC);
    SerialUSB.print(" ");
  }
  SerialUSB.println("");
}

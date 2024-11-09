#include <DFRobot_VL53L0X.h>
#include "cobs.h"

DFRobot_VL53L0X sensor1;


int number_of_sensors = 1;

void setup_VL53L0X() {
  //join i2c bus (address optional for master)
  Wire.begin();

  //Set I2C sub-device address
  sensor1.begin(0x50);

  //Set to Back-to-back mode and high precision mode
  sensor1.setMode(sensor1.eContinuous, sensor1.eHigh);

  //Laser rangefinder begins to work
  sensor1.start();
}

float get_value_of_VL53L0X(int n) 
{
  //Get the distance
  if (n == 1){
    //Serial.print("Distance: ");
    //Serial.println(sensor1.getDistance());
    return sensor1.getDistance();

  } else {
    return 0.0;
  }

}

void read_VL53L0X_to_brain(){

  

  uint8_t buf[4]; 
  uint8_t output_buf[40];
  
  float val = get_value_of_VL53L0X(1);

  struct cobs_encode_result result;
  
  memcpy(buf, &val, sizeof(float));


  result = cobs_encode(output_buf, 40, buf, 4); //encode the data into COBS format
   
  serial_write_to_brain_buffer(output_buf, result.out_len);
}




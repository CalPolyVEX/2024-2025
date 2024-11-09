//Install the Arduino libraries:  Sparkfun toolkit, Sparkfun OTOS library

#include "SparkFun_Qwiic_OTOS_Arduino_Library.h"
#include "Wire.h"

// Create an Optical Tracking Odometry Sensor object
QwiicOTOS otos;

void setup_otos()
{
  
  Serial.println("Qwiic OTOS Example 1 - Basic Readings");


  Wire.begin();

  // Attempt to begin the sensor
  while (otos.begin() == false)
  {
    Serial.println("OTOS not connected, check your wiring and I2C address!");
    delay(1000);
  }

  Serial.println("OTOS connected!");

  Serial.println("Ensure the OTOS is flat and stationary to calibrate the IMU");

  Serial.println("Calibrating IMU...");

  // Calibrate the IMU, which removes the accelerometer and gyroscope offsets
  otos.calibrateImu();

  // Reset the tracking algorithm - this resets the position to the origin,
  // but can also be used to recover from some rare tracking errors
  otos.resetTracking();


}



void get_otos_position(uint8_t* buf) {
  // Get the latest position, which includes the x and y coordinates, plus the
  // heading angle
  sfe_otos_pose2d_t myPosition;
  otos.getPosition(myPosition);
  float* temp_ptr;

  // Print measurement
  //Serial.println();
  //Serial.println("Position:");
  //Serial.print("X (Inches): ");
  //Serial.println(myPosition.x);
  temp_ptr = (float*) buf;
  *temp_ptr = myPosition.x;
  
  //Serial.print("Y (Inches): ");
  //Serial.println(myPosition.y);
  temp_ptr++;
  *temp_ptr = myPosition.y;
  
  //Serial.print("Heading (Degrees): ");
  //Serial.println(myPosition.h);
  
  
  temp_ptr++;
  *temp_ptr = myPosition.h;

  // Wait a bit so we don't spam the serial port
  //delay(500);
  

  // Alternatively, you can comment out the print and delay code above, and
  // instead use the following code to rapidly refresh the data
  // DID THIS
  Serial.print(myPosition.x);
  Serial.print("\t");
  Serial.print(myPosition.y);
  Serial.print("\t");
  Serial.println(myPosition.h);
  
}

void send_otos_data(){
  uint8_t buf[12]; // 4 bytes each for each dimension (xyh)
  uint8_t output_buf[40];
  
  struct cobs_encode_result result;
  
  get_otos_position(buf);

  result = cobs_encode(output_buf, 40, buf, 12); //encode the data into COBS format
   
  serial_write_to_brain_buffer(output_buf, result.out_len);  //write the COBS packet to the VEX brain 

}





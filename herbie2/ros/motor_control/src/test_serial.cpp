//3/27/20 This node is used to read/write serial data with the Arduino

#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include <rtabmap_ros/Info.h>

#define R_PACKET_SIZE 14

using namespace std;

ros::NodeHandle *nh;
ros::Publisher pub_;

class TestSerial {
  std::string dev_name;
  int baud_rate, address;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  serial::Serial *my_serial;

  public:
    TestSerial(); 

    void read_status(unsigned short* status);
    void test_read();
    int check_crc(unsigned char* data, int len);
};

TestSerial::TestSerial() : tf_listener_(tf_buffer_) {
  pub_ = nh->advertise<std_msgs::Int32MultiArray>("/encoder_service", 1);

  ROS_INFO("Connecting to arduino");
  nh->param<std::string>("dev1", dev_name, "/dev/arduino");
  nh->param<int>("baud1", baud_rate, 115200);

  // Open the serial port
  my_serial = new serial::Serial(dev_name, baud_rate, serial::Timeout::simpleTimeout(1000));
  
  if(my_serial->isOpen())
    cout << "Successfully opened serial port." << endl;
  else {
    cout << "Could not open serial port." << endl;
    ROS_FATAL("Could not connect to Arduino");
  }
}

void TestSerial::test_read() {
  unsigned char data[R_PACKET_SIZE];
  int i,x;
  int left_encoder, right_encoder;
  int mask,counter;
  unsigned char extra_byte[2];
  std_msgs::Int32MultiArray wheel_enc_msg;

  my_serial->flushOutput();
  my_serial->flushInput();

  while (ros::ok()) {
    x = my_serial->read(data,R_PACKET_SIZE);
    if (check_crc(data,R_PACKET_SIZE) == 1) {
      /* cout << x << endl; */
      wheel_enc_msg.data.clear();
      left_encoder = 0;
      right_encoder = 0;

      //encoder data sent least significant byte first
      counter = 0;
      for(i=2;i<6;i++) {
        mask = data[i] << (8*counter);
        left_encoder |= mask;
        counter++;
      }
      wheel_enc_msg.data.push_back(left_encoder);

      counter = 0;
      for(i=6;i<10;i++) {
        mask = data[i] << (8*counter);
        right_encoder |= mask;
        counter++;
      }
      wheel_enc_msg.data.push_back(right_encoder);

      extra_byte[0] = data[R_PACKET_SIZE-4]; //these are 2 extra bytes of data for future use
      extra_byte[1] = data[R_PACKET_SIZE-3];

      pub_.publish(wheel_enc_msg);
    }
  }
}

int TestSerial::check_crc(unsigned char* data, int len) {
  unsigned short crc=0;
  unsigned short received_crc=0;

  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < (len-2); byte++) {        
    crc = crc ^ ((unsigned short)data[byte] << 8);        
    for (unsigned char bit = 0; bit < 8; bit++) {            
      if (crc & 0x8000) {                
        crc = (crc << 1) ^ 0x1021;            
      } else {                
        crc = crc << 1;   
      } 
    } 
  } 

  received_crc = (data[len-2] << 8) & 0xFF00;
  received_crc = received_crc | data[len-1]; 

  if (received_crc == crc) //return 1 if the CRC is correct
     return 1;
  else
     return 0;
}

void TestSerial::read_status(unsigned short* status) {
  unsigned char data[8];

  my_serial->flushOutput();
  my_serial->flushInput();

  data[0] = address;
  data[1] = 90; //read status
  my_serial->write(data,2);

  my_serial->read(data,4);
  *status = data[0] << 8;
  *status += data[1];
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "testserial_node");
  ros::NodeHandle n;
  nh = &n;

  TestSerial ts;

  ROS_INFO("Starting test serial");
  ts.test_read();

  ros::spin();
  ros::waitForShutdown();
}

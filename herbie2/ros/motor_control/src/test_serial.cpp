//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include <rtabmap_ros/Info.h>

using namespace std;

ros::NodeHandle *nh;
ros::Publisher pub_;

class TestSerial {
  std::string dev_name;
  int baud_rate, address;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  boost::mutex planner_mutex;
  serial::Serial *my_serial;

  public:
    TestSerial(); 

    void read_status(unsigned short* status);
    void test_read();
    int check_crc(unsigned char* data, int len);
};

TestSerial::TestSerial() : tf_listener_(tf_buffer_) {
  pub_ = nh->advertise<std_msgs::Int32MultiArray>("/enc", 1);

  ROS_INFO("Connecting to arduino");
  nh->param<std::string>("dev1", dev_name, "/dev/arduino");
  nh->param<int>("baud1", baud_rate, 115200);

  // Open the serial port
  my_serial = new serial::Serial(dev_name, baud_rate, serial::Timeout::simpleTimeout(1000));
  
  if(my_serial->isOpen())
    cout << "Successfully opened serial port." << endl;
  else {
    cout << "Could not open serial port." << endl;
    ROS_FATAL("Could not connect to Roboclaw");
  }
  
}

void TestSerial::test_read() {
  unsigned char data[12];
  int x;
  std_msgs::Int32MultiArray wheel_enc_msg;
  std_msgs::MultiArrayLayout mal;
  std_msgs::MultiArrayDimension mad[1];
  char dim0_label[8] = "encoder";
  long int encoder_values[2] = {0,0};

  /* mad[0].label = (char*) &dim0_label; */
  /* mad[0].size = 2; */
  /* mad[0].stride = 1*2; */

  /* mal.dim = mad; */
  /* mal.data_offset = 0; */
  /* mal.dim_length = 1; */
  /* wheel_enc_msg.data_length = 2; */
  /* wheel_enc_msg.layout = mal; */
  /* wheel_enc_msg.data = (long int*) &encoder_values; */

  my_serial->flushOutput();
  my_serial->flushInput();

  while (ros::ok()) {
    x = my_serial->read(data,12);
    if (check_crc(data,12) == 1) {
      cout << x << endl;
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

  if (received_crc == crc)
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

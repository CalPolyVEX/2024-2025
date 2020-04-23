//3/27/20 This node is used to read/write serial data with the Arduino

#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/Int16.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <rtabmap_ros/Info.h>

#define RECEIVE_PACKET_SIZE 13
#define SEND_PACKET_SIZE 4

using namespace std;

ros::NodeHandle *nh;
ros::Publisher pub_;
ros::Publisher e_stop_pub;
ros::Subscriber arduino_cmd_sub;
boost::lockfree::queue<int> q(128);

class ArduinoNode {
  std::string dev_name;
  int baud_rate, address;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  serial::Serial *my_serial;

  public:
    ArduinoNode(); 

    void read_status(unsigned short* status);
    void test_read();
    int check_receive_crc(unsigned char* data, int len);
    void compute_transmit_crc(unsigned char* data, int len);
    void command_received_callback(const std_msgs::Int16::ConstPtr& m);
};

ArduinoNode::ArduinoNode() : tf_listener_(tf_buffer_) {
  pub_ = nh->advertise<std_msgs::Int32MultiArray>("/encoder_service", 1);
  e_stop_pub = nh->advertise<std_msgs::Empty>("/e_stop", 1);
  arduino_cmd_sub = nh->subscribe("/arduino_cmd", 1, &ArduinoNode::command_received_callback, this);

  ROS_INFO("Connecting to arduino");
  nh->param<std::string>("dev", dev_name, "/dev/arduino");
  nh->param<int>("baud", baud_rate, 115200);

  // Open the serial port
  my_serial = new serial::Serial(dev_name, baud_rate, serial::Timeout::simpleTimeout(1000));
  
  if(my_serial->isOpen())
    cout << "Successfully opened serial port." << endl;
  else {
    cout << "Could not open serial port." << endl;
    ROS_FATAL("Could not connect to Arduino");
  }
}

void ArduinoNode::command_received_callback(const std_msgs::Int16::ConstPtr& m) {
  int temp = m->data;
  q.push(temp);
  q.push(temp);
}

void ArduinoNode::test_read() {
  unsigned char data[RECEIVE_PACKET_SIZE];
  int i,x;
  int left_encoder, right_encoder;
  int mask,counter;
  unsigned char extra_byte[2];
  std_msgs::Int32MultiArray wheel_enc_msg;

  my_serial->flushOutput();
  my_serial->flushInput();

  while (ros::ok()) {
    x = my_serial->read(data,RECEIVE_PACKET_SIZE);
    if ((check_receive_crc(data,RECEIVE_PACKET_SIZE) == 1) && (data[0] == 0xFF)) {
      /* cout << x << endl; */
      wheel_enc_msg.data.clear();
      left_encoder = 0;
      right_encoder = 0;

      //encoder data sent least significant byte first
      counter = 1;
      for(i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        left_encoder |= mask;
        counter++;
      }
      wheel_enc_msg.data.push_back(left_encoder);

      for(i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        right_encoder |= mask;
        counter++;
      }
      wheel_enc_msg.data.push_back(right_encoder);

      extra_byte[0] = data[RECEIVE_PACKET_SIZE-4]; //these are 2 extra bytes of data for future use
      extra_byte[1] = data[RECEIVE_PACKET_SIZE-3];
      if ((extra_byte[1] & 0x1) == 0) {
        //if analog pin 0 is grounded on the Arduino, then activate the e_stop
        std_msgs::Empty e;
        e_stop_pub.publish(e);
      }

      pub_.publish(wheel_enc_msg);
    }

    //send data to the Arduino
    int output_data;
    if (q.pop(output_data)) {
      unsigned char send_data[SEND_PACKET_SIZE];
      my_serial->flushOutput();
      send_data[0] = (output_data >> 8) & 0xFF; 
      send_data[1] = output_data & 0xFF; 
      compute_transmit_crc(send_data, SEND_PACKET_SIZE);
      my_serial->write(send_data,SEND_PACKET_SIZE);
    }

    ros::spinOnce();
  }
}

void ArduinoNode::compute_transmit_crc(unsigned char* data, int len) {
  unsigned short crc=0;

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

  data[len-2] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[len-1] = crc & 0xFF; //send the low byte of the crc
}

int ArduinoNode::check_receive_crc(unsigned char* data, int len) {
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

void ArduinoNode::read_status(unsigned short* status) {
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

  ArduinoNode ts;

  ROS_INFO("Starting test serial");
  ts.test_read();

  ros::spin();
  ros::waitForShutdown();
}

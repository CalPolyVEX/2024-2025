//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/ByteMultiArray.h>
#include <std_msgs/Empty.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include "encoder_odom.h"

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel, autonomous_sub;
ros::Subscriber sub_stop;
ros::Subscriber control_board_sub;
ros::Subscriber camera_error_sub;
ros::Publisher pub_;
ros::Publisher twist_pub;

OdometryPublisher::OdometryPublisher() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  actual_vel_mutex.unlock();
  last_set_speed_time_mutex.unlock();
  desired_vel_mutex.unlock();
  setmotor_mutex.unlock();
  update_encoder_mutex.unlock();
  herbie_board_queue_mutex.unlock();
  
  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //publish Twist messages to this topic
  twist_pub = nh->advertise<geometry_msgs::TwistWithCovarianceStamped>("/roboclaw_twist", 1);

  //listen for Twist messages on /cmd_vel
  sub_cmd_vel = nh->subscribe("/cmd_vel", 2, &OdometryPublisher::cmd_vel_callback, this);
  
  //listen for Twist messages on /cmd_vel
  sub_planner_cmd_vel = nh->subscribe("/nav_cmd_vel", 2, &OdometryPublisher::planner_cmd_vel_callback, this);

  //listen for Int8 messages on /autonomous
  autonomous_sub = nh->subscribe("/autonomous", 2, &OdometryPublisher::autonomous_mode_callback, this);

  //listen for messages to send to the control board
  control_board_sub = nh->subscribe("/control_board", 5, &OdometryPublisher::control_board_callback, this);
  
  //listen for camera initialization errors
  camera_error_sub = nh->subscribe("/camera_error", 1, &OdometryPublisher::camera_error_callback, this);

  ROS_INFO("Connecting to Herbie control board");
  nh->param<std::string>("dev1", dev_name, "/dev/herbie");
  nh->param<int>("baud1", baud_rate, 230400);
  nh->param<int>("address1", address, 128);
  nh->param<double>("max_abs_linear_speed1", MAX_ABS_LINEAR_SPEED, 1.0);
  nh->param<double>("max_abs_angular_speed1", MAX_ABS_ANGULAR_SPEED, 2.0);
  nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 4467);
  //nh->param<double>("base_width1", BASE_WIDTH, 0.371475);
  nh->param<double>("base_width1", BASE_WIDTH, 0.280988);
  nh->param<double>("acc_lim1", ACC_LIM, 0.1);

  nh->setParam("/autonomous_mode", false);

  if (address > 0x87 || address < 0x80) {
    ROS_INFO("Address out of range");
  }

  // Open the serial port
  my_serial = new serial::Serial(dev_name, baud_rate, serial::Timeout::simpleTimeout(1000));
  
  if(my_serial->isOpen())
    cout << "Successfully opened serial port." << endl;
  else {
    cout << "Could not open serial port." << endl;
    ROS_FATAL("Could not connect to Roboclaw");
  }
  
  setmotor_mutex.lock();
  setmotor(0,0); //set motors to stop
  cur_left_motor=0;
  cur_right_motor=0;
  setmotor_mutex.unlock();

  for (int i=0;i<INTEGRAL_ARRAY_SIZE;i++) {
    left_integral[i] = 0;
    right_integral[i] = 0;
  }

  left_counter = 0;
  right_counter = 0;

  create_control_board_msg(5,(void*) 0); //backlight on
}

void OdometryPublisher::setmotor(int duty_cyclel, int duty_cycler) {
  signed short dl = duty_cyclel;
  signed short dr = duty_cycler;
  unsigned char data[8];
  unsigned short crc = 0;
  int sent_board_packets = 0;

  /* my_serial->flushOutput(); */
  /* my_serial->flushInput(); */

  data[0] = address;
  data[1] = 34; // set left/right motors command

  data[2] = (dl >> 8) & 0xFF; //send the high byte of the duty cycle
  data[3] = dl & 0xFF; //send the low byte of the duty cycle

  data[4] = (dr >> 8) & 0xFF; //send the high byte of the duty cycle
  data[5] = dr & 0xFF; //send the low byte of the duty cycle

  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < 6; byte++) {        
     crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
  }

  data[6] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[7] = crc & 0xFF; //send the low byte of the crc

  my_serial->write(data,8);
  /* std::cout << "\tsending packet" << std::endl; */

  //handle the packets to be sent to the Herbie control board
  herbie_board_queue_mutex.lock();
  while ((board_queue.size() != 0) && (sent_board_packets < 7)) {
     herbie_board_queue_mutex.unlock();
     usleep(1500);
     struct packet p;

     herbie_board_queue_mutex.lock();
     p = board_queue.front();  //remove the packet from the queue
     board_queue.pop();
     herbie_board_queue_mutex.unlock();

     my_serial->write(p.data,p.size); //send the data

     //add extra delay for clear screen
     if (p.data[1] == 0)
        usleep(500);

     sent_board_packets++;

     herbie_board_queue_mutex.lock();
  }
  herbie_board_queue_mutex.unlock();
}

void OdometryPublisher::control_board_callback(const std_msgs::Int32MultiArray::ConstPtr& board_msg) {
   //this function handles commands that are sent to the Herbie control board
   //the packets are stored in a queue and the function setmotor() will send 
   //out the packets to the board (every packet must be at least 8 bytes long)
   
   struct packet p;
   
   p.size = board_msg->data[0]; //get the size of the incoming packet
   for (int i=0; i<p.size; i++) { //copy over the packet data
      p.data[i] = board_msg->data[i+1];
   }

   herbie_board_queue_mutex.lock();
   board_queue.push(p);  //push the packet into the queue
   herbie_board_queue_mutex.unlock();
}

void OdometryPublisher::safety_callback(const ros::TimerEvent& ev) {
  unsigned short voltage;
  unsigned short left_current, right_current;
  ros::Time last_set_speed_time2;

  last_set_speed_time_mutex.lock();
  last_set_speed_time2 = last_set_speed_time;
  last_set_speed_time_mutex.unlock();

  if ((ros::Time::now() - last_set_speed_time2).toSec() > 3.0) {
    ROS_INFO("Did not get command for 1 second, stopping");

    //reset the integral term for the PID controller
    for (int i=0;i<INTEGRAL_ARRAY_SIZE;i++) {
        left_integral[i] = 0;
        right_integral[i] = 0;
    }

    setmotor_mutex.lock();
    setmotor(0,0); //turn off the motors
    cur_left_motor=0;
    cur_right_motor=0;
    last_left_error=0;
    last_right_error=0;
    setmotor_mutex.unlock();

    last_set_speed_time_mutex.lock();
    last_set_speed_time = ev.current_real;
    last_set_speed_time_mutex.unlock();

    desired_vel_mutex.lock();
    desired_vl = 0;
    desired_vr = 0;
    desired_vel_mutex.unlock();

    last_left_vel = 0;
    last_right_vel = 0;
    left_tick_vel = 0;
    right_tick_vel = 0;
  }

  //check for stalled motors

  //check for tilted robot

  //read_voltage(&voltage);
  //ROS_INFO("Voltage: %f", (float) voltage / 10.0);
}

void OdometryPublisher::camera_error_callback(const std_msgs::Empty::ConstPtr& board_msg) {
  create_control_board_msg(0,(void*) 0); //clear screen
  char buf[10];
  char s[32];
  buf[0] = 0; //set the col
  buf[1] = 0; //set the row
  create_control_board_msg(1,(void*) buf); //set cursor to top
  create_control_board_msg(2,(void*) "Camera error"); //send camera error message

  unsigned char x = 2; //LED 2 = green LED
  create_control_board_msg(7,(void*) &x); //turn on LED
}

void OdometryPublisher::serial_loop() {
  unsigned char data[RECEIVE_PACKET_SIZE];
  int x;
  int led_counter = 0;
  int left_encoder, right_encoder;
  unsigned int mask,counter;
  unsigned char extra_byte[2];
  boost::shared_ptr<std_msgs::Int32MultiArray> test_msg(new std_msgs::Int32MultiArray());

  my_serial->flushOutput();
  my_serial->flushInput();

  while (ros::ok()) {
    //this loop waits for serial data to be received from the Herbie PCB (which is
    //sent at 30Hz
    x = my_serial->read(data,RECEIVE_PACKET_SIZE);
    if ((check_receive_crc(data,RECEIVE_PACKET_SIZE) == 1) && (data[0] == 0xFF)) {
      /* std::cout << "valid packet received" << std::endl; */
      left_encoder = 0;
      right_encoder = 0;

      //encoder data sent least significant byte first
      counter = 1;
      for(int i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        left_encoder |= mask;
        counter++;
      }

      for(int i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        right_encoder |= mask;
        counter++;
      }

      extra_byte[0] = data[9]; //these are 2 extra bytes of data for future use
      extra_byte[1] = data[10];

      encoder_message_callback(left_encoder, right_encoder); 
    }

    //periodically blink green LED on the Herbie PCB to indicate this loop is running
    led_counter++;

    if ((led_counter & 63) == 0) {
       unsigned char x = 2; //LED 2 = green LED
       create_control_board_msg(7,(void*) &x); //turn on LED
    } else if ((led_counter & 63) == 2) {
       unsigned char x = 2; //LED 2 = green LED
       create_control_board_msg(8,(void*) &x); //turn off LED
    }

    //periodically blink yellow LED to indicate autonomous mode
    if ((led_counter & 15) == 0 && (planner == 1)) {
       unsigned char x = 3; //LED 3 = yellow LED
       create_control_board_msg(7,(void*) &x); //turn on LED
    } else if ((led_counter & 15) == 0 && (planner == 0)) {
       unsigned char x = 3; //LED 3 = yellow LED
       create_control_board_msg(8,(void*) &x); //turn off LED
    }
    ros::spinOnce();
  }
}

void OdometryPublisher::create_control_board_msg(int num, void* arg) {
   boost::shared_ptr<std_msgs::Int32MultiArray> test_msg(new std_msgs::Int32MultiArray());
   unsigned char buf[8];
   unsigned short test_crc;

   buf[0] = 128;  //set the address to 128
   buf[1] = num;  //this is the command number

   if (num == 7 || num == 8) { //led on/off
      buf[2] = *((char*)arg); //led num
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
   } else if (num == 0) { //clear screen
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
   } else if (num == 1) { //set cursor to starting position
      char col = *((char*) arg);
      char row = *(((char*) arg) + 1);

      buf[2] = col;
      buf[3] = row;
      buf[4] = 0;
      buf[5] = 0;
   } else if (num == 3) { //print int
      int val = *((int*) arg);
      for (int i=0; i<4; i++) {
         buf[i+2] = (val >> (8*i)) & 0xFF; 
      }
   } else if (num == 4 || num == 5) {  //backlight off/on
      for (int i=2; i<6; i++) {
         buf[i] = 0; 
      }
   } else if (num == 2) { //print string
      char str_buf[32];
      char* str_ptr = (char*) arg;
      int len = strlen((char*) arg);

      str_buf[0] = 128;  //set the address to 128
      str_buf[1] = num;  //this is the command number
      str_buf[2] = (char) len;  //store the length
      for (int i=0; i<len; i++) {
         str_buf[3+i] = str_ptr[i];
      }

      test_crc = compute_crc((unsigned char*) str_buf,len+3);
      str_buf[len+3] = (test_crc >> 8) & 0xFF;
      str_buf[len+4] = test_crc & 0xFF;

      test_msg->data.clear();
      test_msg->data.push_back(len+5); //size of the packet
      for (int i=0; i<(len+5); i++) {
         test_msg->data.push_back(str_buf[i]);
      }
      control_board_callback(test_msg);
      return;
   }

   test_crc = compute_crc(buf,6);
   buf[6] = (test_crc >> 8) & 0xFF;
   buf[7] = test_crc & 0xFF;

   test_msg->data.clear();
   test_msg->data.push_back(8); //size of the packet
   for (int i=0; i<8; i++) {
      test_msg->data.push_back(buf[i]);
   }
   control_board_callback(test_msg);
}

unsigned short OdometryPublisher::compute_crc(unsigned char* data, int len) {
  unsigned short crc=0;

  for (int byte = 0; byte < len; byte++)
  {
     crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
  }

  return crc;
}

int OdometryPublisher::check_receive_crc(unsigned char* data, int len) {
  unsigned short crc=0;
  unsigned short received_crc=0;

  for (int byte = 0; byte < (len-2); byte++)
  {
     crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
  }

  received_crc = (data[len-2] << 8) & 0xFF00;
  received_crc = received_crc | data[len-1]; 

  if (received_crc == crc) //return 1 if the CRC is correct
     return 1;
  else
     return 0;
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "roboclaw_node");
  ros::NodeHandle n;
  nh = &n;

  OdometryPublisher odom_pub;
  //ros::AsyncSpinner s(4); //use 4 threads;

  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> diag_callback;
  diag_callback=boost::bind(&OdometryPublisher::safety_callback,&odom_pub,_1);

  ROS_INFO("Starting motor drive");
  //run diagnostics function at 5Hz (.2 seconds)
  /* ros::Timer diag_timer = nh->createTimer(ros::Duration(1.2), diag_callback); */

  odom_pub.serial_loop();

  ros::spin();
  ros::waitForShutdown();
}

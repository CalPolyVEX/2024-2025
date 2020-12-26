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
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include "encoder_odom.h"
#include <rtabmap_ros/Info.h>

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel;
ros::Subscriber sub_stop;
ros::Subscriber rtabmap_info_sub;
ros::Subscriber control_board_sub;
ros::Publisher pub_, loop_closure_pub;
ros::Publisher twist_pub;

OdometryPublisher::OdometryPublisher() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  actual_vel_mutex.unlock();
  last_set_speed_time_mutex.unlock();
  desired_vel_mutex.unlock();
  setmotor_mutex.unlock();
  update_encoder_mutex.unlock();
  
  //encoder Int32MultiArray messages are received from the Arduino on this topic
  sub_ = nh->subscribe("/encoder_service", 1, &OdometryPublisher::encoder_message_callback, this);

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //publish Twist messages to this topic
  twist_pub = nh->advertise<geometry_msgs::TwistWithCovarianceStamped>("/roboclaw_twist", 1);

  //publish Odometry messages to this topic
  loop_closure_pub = nh->advertise<std_msgs::Int32MultiArray>("/update_loop_closure_lcd", 1);

  //listen for Twist messages on /cmd_vel
  sub_cmd_vel = nh->subscribe("/cmd_vel", 1, &OdometryPublisher::cmd_vel_callback, this);
  
  //listen for Twist messages on /cmd_vel
  sub_planner_cmd_vel = nh->subscribe("/planner/cmd_vel", 1, &OdometryPublisher::planner_cmd_vel_callback, this);

  //listen for Empty messages on /robot_stop
  sub_stop = nh->subscribe("/robot_stop", 1, &OdometryPublisher::stop_toggle_callback, this);

  //listen for Empty messages on /rtabmap/info
  rtabmap_info_sub = nh->subscribe("/rtabmap/info", 1, &OdometryPublisher::rtabmap_info_callback, this);
  
  //listen for messages to send to the control board
  control_board_sub = nh->subscribe("/control_board", 5, &OdometryPublisher::control_board_callback, this);

  ROS_INFO("Connecting to roboclaw");
  nh->param<std::string>("dev1", dev_name, "/dev/roboclaw");
  nh->param<int>("baud1", baud_rate, 38400);
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

  board_queue.reserve(64);
}

void OdometryPublisher::setmotor(int duty_cyclel, int duty_cycler) {
  signed short dl = duty_cyclel;
  signed short dr = duty_cycler;
  unsigned char data[8];
  unsigned short crc = 0;
  int sent_board_packets = 0;

  my_serial->flushOutput();
  my_serial->flushInput();

  data[0] = address;
  data[1] = 34; // set left/right motors command

  data[2] = (dl >> 8) & 0xFF; //send the high byte of the duty cycle
  data[3] = dl & 0xFF; //send the low byte of the duty cycle

  data[4] = (dr >> 8) & 0xFF; //send the high byte of the duty cycle
  data[5] = dr & 0xFF; //send the low byte of the duty cycle

  //Calculates CRC16 of nBytes of data in byte array message
  /* for (int byte = 0; byte < 6; byte++) { */        
  /*   crc = crc ^ ((unsigned int)data[byte] << 8); */        
  /*   for (unsigned char bit = 0; bit < 8; bit++) { */            
  /*     if (crc & 0x8000) { */                
  /*       crc = (crc << 1) ^ 0x1021; */            
  /*     } else { */                
  /*       crc = crc << 1; */   
  /*     } */ 
  /*   } */ 
  /* } */ 
  
  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < 6; byte++) {        
     crc = (crc << 8) ^ crctable[((crc >> 8) ^ data[byte])];
  }

  data[6] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[7] = crc & 0xFF; //send the low byte of the crc

  my_serial->write(data,8);

  //handle the packets to be sent to the Herbie control board
  while ( (board_queue.empty() != false) && (sent_board_packets < 3)) {
     usleep(3000);
     struct packet p;

     board_queue.pop(p);
     my_serial->write(p.data,p.size);
     sent_board_packets++;
  }
}

void OdometryPublisher::control_board_callback(const std_msgs::ByteMultiArray::ConstPtr& board_msg) {
   //this function handles command that are sent to the Herbie control board
   //the packets are stored in a queue and the function setmotor() will send 
   //out the packets to the board
   
   struct packet p;
   
   p.size = board_msg->data[0];
   for (int i=0; i<p.size; i++) {
      p.data[i] = board_msg->data[i+1];
   }

   board_queue.push(p);
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

  //check for overcurrent situation with motors
  setmotor_mutex.lock();
  read_motor_currents(&left_current, &right_current);
  setmotor_mutex.unlock();
  left_current *= 10; //convert the current values to mA
  right_current *= 10;
  ROS_INFO("Motor current: %d %d", left_current, right_current);

  if ((left_current > 3000) || (right_current > 3000)) { //3000mA
    current_counter++;
  } else {
    current_counter = 0;
  }

  if (current_counter > 1) {
    //overcurrent for motors
    //stop = 1;
    ROS_INFO("Overcurrent for motors");
  }

  //check for stalled motors

  //check for tilted robot

  //read_voltage(&voltage);
  //ROS_INFO("Voltage: %f", (float) voltage / 10.0);
}

void OdometryPublisher::read_voltage(unsigned short* voltage) {
  unsigned char data[8];

  my_serial->flushOutput();
  my_serial->flushInput();

  data[0] = address;
  data[1] = 24; //read main battery voltage command
  my_serial->write(data,2);

  my_serial->read(data,4);
  *voltage = data[0] << 8; //voltage units are 0.1V
  *voltage += data[1];
}

void OdometryPublisher::read_motor_currents(unsigned short* left_current, unsigned short* right_current) {
  unsigned char data[8];

  my_serial->flushOutput();
  my_serial->flushInput();

  data[0] = address;
  data[1] = 49; //read motor current command
  my_serial->write(data,2);

  my_serial->read(data,6);
  *left_current = data[0] << 8;
  *left_current += data[1];
  *right_current = data[2] << 8;
  *right_current += data[3];
}

void OdometryPublisher::read_status(unsigned short* status) {
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

void OdometryPublisher::test_read() {
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
      /* wheel_enc_msg.data.clear(); */
      left_encoder = 0;
      right_encoder = 0;

      //encoder data sent least significant byte first
      counter = 1;
      for(i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        left_encoder |= mask;
        counter++;
      }
      /* wheel_enc_msg.data.push_back(left_encoder); */

      for(i=0;i<4;i++) {
        mask = data[counter] << (8*i);
        right_encoder |= mask;
        counter++;
      }
      /* wheel_enc_msg.data.push_back(right_encoder); */

      extra_byte[0] = data[RECEIVE_PACKET_SIZE-4]; //these are 2 extra bytes of data for future use
      extra_byte[1] = data[RECEIVE_PACKET_SIZE-3];

      pub_.publish(wheel_enc_msg);
    }

    ros::spinOnce();
  }
}

int OdometryPublisher::check_receive_crc(unsigned char* data, int len) {
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
  ros::Timer diag_timer = nh->createTimer(ros::Duration(1.2), diag_callback);

  ros::spin();
  ros::waitForShutdown();
}

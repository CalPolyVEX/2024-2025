//6/16/19 This is used to publish odometry messages when
//each encoder message arrives

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include <boost/thread.hpp>
#include "encoder_odom.h"
#include <rtabmap_ros/Info.h>

using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel, sub_planner_cmd_vel;
ros::Subscriber sub_stop;
ros::Subscriber rtabmap_info_sub;
ros::Publisher pub_, loop_closure_pub;

OdometryPublisher::OdometryPublisher() : tf_listener_(tf_buffer_) {
  last_set_speed_time = ros::Time::now();

  actual_vel_mutex.unlock();
  last_set_speed_time_mutex.unlock();
  desired_vel_mutex.unlock();
  setmotor_mutex.unlock();
  update_encoder_mutex.unlock();
  
  //read_encoder_cmd is used to send messages to the Arduino to request an encoder update
  //odom_req = nh->advertise<std_msgs::Empty>("/read_encoder_cmd", 1);
  
  //encoder Int32MultiArray messages are received from the Arduino on this topic
  sub_ = nh->subscribe("/encoder_service", 1, &OdometryPublisher::encoder_message_callback, this);

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //publish Odometry messages to this topic
  loop_closure_pub = nh->advertise<std_msgs::Empty>("/loop_closure", 1);

  //listen for Twist messages on /cmd_vel
  sub_cmd_vel = nh->subscribe("/cmd_vel", 1, &OdometryPublisher::cmd_vel_callback, this);
  
  //listen for Twist messages on /cmd_vel
  sub_planner_cmd_vel = nh->subscribe("/planner/cmd_vel", 1, &OdometryPublisher::planner_cmd_vel_callback, this);

  //listen for Empty messages on /robot_stop
  sub_stop = nh->subscribe("/robot_stop", 1, &OdometryPublisher::stop_toggle_callback, this);

  //listen for Empty messages on /rtabmap/info
  rtabmap_info_sub = nh->subscribe("/rtabmap/info", 1, &OdometryPublisher::rtabmap_info_callback, this);

  ROS_INFO("Connecting to roboclaw");
  nh->param<std::string>("dev1", dev_name, "/dev/roboclaw");
  nh->param<int>("baud1", baud_rate, 38400);
  nh->param<int>("address1", address, 128);
  nh->param<double>("max_abs_linear_speed1", MAX_ABS_LINEAR_SPEED, .7);
  nh->param<double>("max_abs_angular_speed1", MAX_ABS_ANGULAR_SPEED, 2.0);
  nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 6800);
  nh->param<double>("base_width1", BASE_WIDTH, 0.371475);
  nh->param<double>("acc_lim1", ACC_LIM, 0.1);

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
  
  /* self.updater = diagnostic_updater.Updater() */
  /* self.updater.setHardwareID("Roboclaw") */
  /* self.updater.add(diagnostic_updater. */
  /*              FunctionDiagnosticTask("Vitals", self.check_vitals)) */

  /* try: */
  /* version = roboclaw.ReadVersion(self.address) */
  /* except Exception as e: */
  /* rospy.logwarn("Problem getting roboclaw version") */
  /* rospy.logdebug(e) */
  /* pass */

  /* if not version[0]: */
  /* rospy.logwarn("Could not get version from roboclaw") */
  /* else: */
  /* rospy.logdebug(repr(version[1])) */

  //read_version();
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
}

void OdometryPublisher::setmotor(int duty_cyclel, int duty_cycler) {
  signed short dl = duty_cyclel;
  signed short dr = duty_cycler;
  unsigned char data[8];
  unsigned int crc = 0;

  my_serial->flushOutput();
  my_serial->flushInput();

  data[0] = address;
  data[1] = 34; // set left/right motors command

  data[2] = (dl >> 8) & 0xFF; //send the high byte of the duty cycle
  data[3] = dl & 0xFF; //send the low byte of the duty cycle

  data[4] = (dr >> 8) & 0xFF; //send the high byte of the duty cycle
  data[5] = dr & 0xFF; //send the low byte of the duty cycle

  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < 6; byte++) {        
    crc = crc ^ ((unsigned int)data[byte] << 8);        
    for (unsigned char bit = 0; bit < 8; bit++) {            
      if (crc & 0x8000) {                
        crc = (crc << 1) ^ 0x1021;            
      } else {                
        crc = crc << 1;   
      } 
    } 
  } 

  data[6] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[7] = crc & 0xFF; //send the low byte of the crc

  my_serial->write(data,8);
}

void OdometryPublisher::run(const ros::TimerEvent& ev) {
  unsigned short voltage;
  ros::Time last_set_speed_time2;

  last_set_speed_time_mutex.lock();
  last_set_speed_time2 = last_set_speed_time;
  last_set_speed_time_mutex.unlock();

  if ((ros::Time::now() - last_set_speed_time2).toSec() > 1.0) {
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

  read_voltage(&voltage);
  ROS_INFO("Voltage: %f", (float) voltage / 10.0);
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

int main(int argc, char** argv) {
  ros::init(argc, argv, "roboclaw_node");
  ros::NodeHandle n;
  nh = &n;

  OdometryPublisher odom_pub;
  //ros::AsyncSpinner s(4); //use 4 threads;


  //since the diagnostics callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> diag_callback;
  diag_callback=boost::bind(&OdometryPublisher::run,&odom_pub,_1);

  ROS_INFO("Starting motor drive");
  //run diagnostics function at 5Hz (.2 seconds)
  ros::Timer diag_timer = nh->createTimer(ros::Duration(.5), diag_callback);

  //since the pid callback is part of the odom_pub object, 
  //bind the callback using boost:bind and boost:function
  //boost::function<void()> pid_callback;
  //pid_callback=boost::bind(&OdometryPublisher::run_pid,&odom_pub);

  //run pid function at 30Hz (.033 seconds)
  //ros::Timer pid_timer = nh->createTimer(ros::Duration(.035), pid_callback);

  //boost::thread testthread(pid_callback);

  //s.start();
  ros::spin();
  ros::waitForShutdown();
}

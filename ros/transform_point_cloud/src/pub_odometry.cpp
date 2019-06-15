//6/1/19 This is a currently unused file to publish odometry when 
//each encoder message arrives
//
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <ros/console.h>
#include <serial/serial.h>
#include <iostream>
#include "encoder_odom.h"
using namespace std;

ros::NodeHandle nh;
ros::Subscriber sub_;
ros::Publisher pub_, odom_req;

OdometryPublisher::OdometryPublisher() : tf_listener_(tf_buffer_) {
  //publish Odometry messages to this topic
  pub_ = nh.advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //encoder Int32MultiArray messages are received from the Arduino on this topic
  sub_ = nh.subscribe("/encoder_service", 1, &OdometryPublisher::encoder_message_callback, this);

  //listen for Twist messages on /cmd_vel
  //sub_cmd_vel("cmd_vel", 1, Twist, self.cmd_vel_callback, 1)

  //read_encoder_cmd is used to send messages to the Arduino to request an encoder update
  odom_req = nh.advertise<std_msgs::Empty>("/read_encoder_cmd", 1);

  //wheel speed publisher
  //wheels_speeds_pub = nh.advertise<Wheel_speeds>("/motors/commanded_speeds", 1);
  //motor current publisher
  //motor_current_pub = nh.advertise<Motors_currents>("/motors/read_current", 1);

  ROS_INFO("Connecting to roboclaw");
  std::string dev_name;
  int baud_rate, address;
  nh.param<std::string>("dev", dev_name, "/dev/ttyACM0");
  nh.param<int>("baud", baud_rate, 38400);
  nh.param<int>("address", address, 128);

  if (address > 0x87 || address < 0x80) {
    ROS_INFO("Address out of range");
  }

  // Open the serial port
  serial::Serial my_serial(dev_name, baud_rate, serial::Timeout::simpleTimeout(1000));

  if(my_serial.isOpen())
    cout << " Yes." << endl;
  else {
    cout << " No." << endl;
    ROS_FATAL("Could not connect to Roboclaw");
    /* rospy.signal_shutdown("Could not connect to Roboclaw") */
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

  /* roboclaw.SpeedM1M2(self.address, 0, 0) */

  nh.param<double>("max_abs_linear_speed", MAX_ABS_LINEAR_SPEED, 1.0);
  nh.param<double>("max_abs_angular_speed", MAX_ABS_ANGULAR_SPEED, 1.0);
  nh.param<double>("ticks_per_meter", TICKS_PER_METER, 6683);
  nh.param<double>("base_width", BASE_WIDTH, 0.315);
  nh.param<double>("acc_lim", ACC_LIM, 0.1);

  //left_integral = [x for x in range(5)]
  //right_integral = [x for x in range(5)]
  left_counter = 0;
  right_counter = 0;
  left_pwm = 0; //current PWM values sent to Roboclaw
  right_pwm = 0;
  last_set_speed_time = ros::Time::now();
  last_left_error = 0;
  last_right_error = 0;
  vl = 0;
  vr = 0;

  /*         rospy.sleep(1) */
  /* EncoderOdom encodm(TICKS_PER_METER, BASE_WIDTH); */
  /* self.left_integral = [x for x in range(5)] */
  /* self.right_integral = [x for x in range(5)] */
  /* self.left_counter = 0 */
  /* self.right_counter = 0 */
  /* self.left_pwm = 0 #current PWM values sent to Roboclaw */
  /* self.right_pwm = 0 */
  /* self.last_set_speed_time = rospy.get_rostime() */
  /* self.last_left_error = 0 */
  /* self.last_right_error = 0 */
  /* self.vl = 0 */
  /* self.vr = 0 */

  /*         rospy.sleep(1) */
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "roboclaw_node");
  OdometryPublisher odom_pub;
  ros::spin();
}

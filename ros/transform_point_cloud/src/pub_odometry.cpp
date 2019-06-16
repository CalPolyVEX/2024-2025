//6/1/19 This is a currently unused file to publish odometry when 
//each encoder message arrives
//
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
#include "encoder_odom.h"
using namespace std;

ros::NodeHandle *nh;
ros::Subscriber sub_, sub_cmd_vel;
ros::Publisher pub_, odom_req;

OdometryPublisher::OdometryPublisher() : tf_listener_(tf_buffer_) {
  //read_encoder_cmd is used to send messages to the Arduino to request an encoder update
  odom_req = nh->advertise<std_msgs::Empty>("/read_encoder_cmd", 1);
  
  //encoder Int32MultiArray messages are received from the Arduino on this topic
  sub_ = nh->subscribe("/encoder_service", 1, &OdometryPublisher::encoder_message_callback, this);

  //publish Odometry messages to this topic
  pub_ = nh->advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

  //listen for Twist messages on /cmd_vel
  sub_cmd_vel = nh->subscribe("/cmd_vel", 1, &OdometryPublisher::cmd_vel_callback, this);

  //wheel speed publisher
  //wheels_speeds_pub = nh->advertise<Wheel_speeds>("/motors/commanded_speeds", 1);
  //motor current publisher
  //motor_current_pub = nh->advertise<Motors_currents>("/motors/read_current", 1);

  ROS_INFO("Connecting to roboclaw");
  nh->param<std::string>("dev1", dev_name, "/dev/ttyACM0");
  nh->param<int>("baud1", baud_rate, 38400);
  nh->param<int>("address1", address, 128);

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

  nh->param<double>("max_abs_linear_speed1", MAX_ABS_LINEAR_SPEED, 1.0);
  nh->param<double>("max_abs_angular_speed1", MAX_ABS_ANGULAR_SPEED, 1.0);
  nh->param<double>("ticks_per_meter1", TICKS_PER_METER, 6683);
  nh->param<double>("base_width1", BASE_WIDTH, 0.315);
  nh->param<double>("acc_lim1", ACC_LIM, 0.1);

  //left_integral = [x for x in range(5)]
  //right_integral = [x for x in range(5)]
  left_counter = 0;
  right_counter = 0;
  left_pwm = 0; //current PWM values sent to Roboclaw
  right_pwm = 0;
  last_set_speed_time = ros::Time::now();
  last_left_error = 0;
  last_right_error = 0;
}

void OdometryPublisher::setmotor(int motor_num, int duty_cycle) {
  //need non-blocking write to serial port
  //  M1DUTY = 32
  //  M2DUTY = 33
  signed short d = duty_cycle;
  unsigned char data[6];
  unsigned int crc = 0;

  data[0] = address;
  data[1] = 32;
  if (motor_num > 0) //left motor is 0, right motor is 1
    data[1]++;

  data[2] = (d >> 8) & 0xFF; //send the high byte of the duty cycle
  data[3] = d & 0xFF; //send the low byte of the duty cycle

  //Calculates CRC16 of nBytes of data in byte array message
  for (int byte = 0; byte < 4; byte++) {        
    crc = crc ^ ((unsigned int)data[byte] << 8);        
    for (unsigned char bit = 0; bit < 8; bit++) {            
      if (crc & 0x8000) {                
        crc = (crc << 1) ^ 0x1021;            
      } else {                
        crc = crc << 1;   
      } 
    } 
  } 

  data[4] = (crc >> 8) & 0xFF; //send the high byte of the crc
  data[5] = crc & 0xFF; //send the low byte of the crc
}

void OdometryPublisher::run(const ros::TimerEvent& ev) {

  if ((ev.current_real - last_set_speed_time).toSec() > 1.0) {
    ROS_INFO("Did not get command for 1 second, stopping");

    //reset the integral term for the PID controller
    //self.left_integral = [0 for x in range(5)]
    //self.right_integral = [0 for x in range(5)]

    //roboclaw.ForwardM1(self.address, 0);
    //roboclaw.ForwardM2(self.address, 0);
  }
    
  //# TODO need find solution to the OSError11 looks like sync problem with serial
  //statusC, amp1, amp2 = None, None, None

  //send a message to the Arduino to request an encoder update
  std_msgs::Empty e;
  odom_req.publish(e);

    /*
    try:
        status1c, amp1, amp2 = roboclaw.ReadCurrents(self.address)
        self.updater.update()
    except ValueError:
        pass

    if (amp1 != None) & (amp2 != None):
        rospy.logdebug(" Currents %d %d" % (amp1, amp2))
        amps=Motors_currents()
        amps.motor1=float(amp1)/100.0
        amps.motor2=float(amp2)/100.0
        self.motors_currents_pub.publish(amps)
    else:
        rospy.logdebug("Error Reading Currents")*/

  /*
        r_time = rospy.Rate(30)
        while not rospy.is_shutdown():
            with self.lock:
                if (rospy.get_rostime() - self.last_set_speed_time).to_sec() > 1:
                    rospy.loginfo("Did not get command for 1 second, stopping")

                    #reset the integral term for the PID controller

                    self.left_integral = [0 for x in range(5)]
                    self.right_integral = [0 for x in range(5)]
                    try:
                        roboclaw.ForwardM1(self.address, 0)
                        roboclaw.ForwardM2(self.address, 0)
                        print ""
                    except OSError as e:
                        rospy.logerr("Could not stop")
                        rospy.logdebug(e)

                # TODO need find solution to the OSError11 looks like sync problem with serial
                statusC, amp1, amp2 = None, None, None

                #send a message to the Arduino to request an encoder update
                self.odom_req.publish(Empty())

                try:
                    status1c, amp1, amp2 = roboclaw.ReadCurrents(self.address)
                    self.updater.update()
                except ValueError:
                    pass

                if (amp1 != None) & (amp2 != None):
                    rospy.logdebug(" Currents %d %d" % (amp1, amp2))
                    amps=Motors_currents()
                    amps.motor1=float(amp1)/100.0
                    amps.motor2=float(amp2)/100.0
                    self.motors_currents_pub.publish(amps)
                else:
                    rospy.logdebug("Error Reading Currents")

            r_time.sleep()
            */
}

int main(int argc, char** argv) {
  ros::init(argc, argv, "roboclaw_node");
  ros::NodeHandle n;
  nh = &n;

  OdometryPublisher odom_pub;
  ros::AsyncSpinner s(4); //use 4 threads;

  //since the encoder callback is part of the odom_pub object
  //bind the callback using boost:bind and boost:function
  boost::function<void(const ros::TimerEvent&)> encoder_request_callback;
  encoder_request_callback=boost::bind(&OdometryPublisher::run,&odom_pub,_1);

  ROS_INFO("Starting motor drive");
  //send a encoder request at 30Hz (.03333 seconds)
  ros::Timer read_enc_timer = nh->createTimer(ros::Duration(.03333), encoder_request_callback);

  s.start();
  ros::waitForShutdown();
}

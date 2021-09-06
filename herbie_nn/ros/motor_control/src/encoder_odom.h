#ifndef ENCODER_H
#define ENCODER_H

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Int32MultiArray.h>
#include <std_msgs/Int8.h>
#include <std_msgs/ByteMultiArray.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>

#include <ros/console.h>
#include <serial/serial.h>
#include <boost/thread.hpp>
#include <iostream>
#include <cmath>
#include <queue>

#define INTEGRAL_ARRAY_SIZE 5
#define RECEIVE_PACKET_SIZE 13

struct packet {
   int size;
   unsigned char data[24];
};

class OdometryPublisher {
  const unsigned short crctable[256] =
{
   0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
   0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
   0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
   0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
   0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
   0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
   0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
   0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
   0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
   0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
   0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
   0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
   0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
   0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
   0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
   0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
   0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
   0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
   0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
   0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
   0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
   0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
   0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
   0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
   0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
   0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
   0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
   0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
   0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
   0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
   0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
   0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

  std::string dev_name;
  int baud_rate, address;
  ros::Time last_enc_time; //time of the last encoder reading
  ros::Time current_time;
  double MAX_ABS_LINEAR_SPEED, MAX_ABS_ANGULAR_SPEED, TICKS_PER_METER, BASE_WIDTH, ACC_LIM;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  tf2_ros::TransformBroadcaster odom_broadcaster;
  boost::mutex actual_vel_mutex;
  boost::mutex last_set_speed_time_mutex;
  boost::mutex desired_vel_mutex;
  boost::mutex setmotor_mutex;
  boost::mutex update_encoder_mutex;
  boost::mutex planner_mutex;
  boost::mutex herbie_board_queue_mutex;
  std::queue<struct packet> board_queue; //queue of messages to send to the Herbie control board
  serial::Serial *my_serial;

  int rtabmap_started = 0;
  int left_counter=0, right_counter=0;
  ros::Time last_set_speed_time;
  int last_left_error=0, last_right_error=0;
  int last_enc_left=0, last_enc_right=0; //last encoder counts
  double last_left_vel=0, last_right_vel=0; //last wheel velocities
  double left_tick_vel=0, right_tick_vel=0; //current wheel velocities
  double cur_x=0, cur_y=0, cur_theta=0;
  int desired_vl=0, desired_vr=0; //desired wheel velocities
  int cur_left_motor=0, cur_right_motor=0; //current motor command
  int update_encoders=1, stop=0, planner=0;
  int loop_closure = 0, proximity = 0;
  double left_integral[INTEGRAL_ARRAY_SIZE];
  double right_integral[INTEGRAL_ARRAY_SIZE];
  int debug_odometry = 0;
  int current_counter = 0;

  public:
    OdometryPublisher(); 
    void publish_odometry_message(double vx, double vth); //publish a new Odometry message
    void encoder_message_callback(int left_encoder, int right_encoder);
    void cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void planner_cmd_vel_callback(const geometry_msgs::Twist::ConstPtr& twist);
    void autonomous_mode_callback(const std_msgs::Int8::ConstPtr&);
    void safety_callback(const ros::TimerEvent& ev);
    void control_board_callback(const std_msgs::Int32MultiArray::ConstPtr& board_msg);
    void camera_error_callback(const std_msgs::Empty::ConstPtr& board_msg);

    //motor control and odometry functions
    void run_pid();
    void compute_pid(double left_desired, double left_actual, double right_desired, double right_actual);
    void update_odometry(int enc_left, int enc_right, double* vel_x, double* vel_theta);
    double normalize_angle(double angle);

    //Roboclaw functions
    void setmotor(int duty_cyclel, int dutycycler);
    void serial_loop();
    int check_receive_crc(unsigned char* data, int len);
    inline unsigned short compute_crc(unsigned char* data, int len) __attribute__((always_inline));
    void create_control_board_msg(int num, void* arg);
};

#endif

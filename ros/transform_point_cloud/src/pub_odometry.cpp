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

class OdometryPublisher
{
  ros::NodeHandle nh;
  ros::Subscriber sub_;
  ros::Publisher pub_;
  int _PreviousLeftEncoderCounts;
  int _PreviousRightEncoderCounts;
  ros::Time last_time;
  double x,y,th;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  void odometryCallBack(const std_msgs::Int32MultiArray::ConstPtr& msg) {
    int tick_x = 0;
    int tick_y = 0;

    ros::Time current_time = ros::Time::now();
    double DistancePerCount = (3.14159265 * 0.13) / 2626; 
    double lengthBetweenTwoWheels = 0.25;

    //extract the wheel velocities from the tick signals count
    int deltaLeft = tick_x - _PreviousLeftEncoderCounts;
    int deltaRight = tick_y - _PreviousRightEncoderCounts;

    double omega_left = (deltaLeft * DistancePerCount) / (current_time - last_time).toSec();
    double omega_right = (deltaRight * DistancePerCount) / (current_time - last_time).toSec();

    double v_left = omega_left * 0.065; //radius
    double v_right = omega_right * 0.065;

    double vx = ((v_right + v_left) / 2)*10;
    double vy = 0;
    double vth = ((v_right - v_left)/lengthBetweenTwoWheels)*10;

    double dt = (current_time - last_time).toSec();
    double delta_x = (vx * cos(th)) * dt;
    double delta_y = (vx * sin(th)) * dt;
    double delta_th = vth * dt;

    x += delta_x;
    y += delta_y;
    th += delta_th;

    tf2::Quaternion odom_quat;
    odom_quat.setRPY(0,0,0);

    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = current_time;
    odom_trans.header.frame_id = "robot_odom";
    odom_trans.child_frame_id = "base_link";

    odom_trans.transform.translation.x = x;
    odom_trans.transform.translation.y = y;
    odom_trans.transform.translation.z = 0.0;
    //odom_trans.transform.rotation = odom_quat;

    //send the transform
    //odom_broadcaster.sendTransform(odom_trans);

    //Odometry message
    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";

    //set the position
    odom.pose.pose.position.x = x;
    odom.pose.pose.position.y = y;
    odom.pose.pose.position.z = 0.0;
    //odom.pose.pose.orientation = odom_quat;

    //set the velocity
    odom.child_frame_id = "base_link";
    odom.twist.twist.linear.x = vx;
    odom.twist.twist.linear.y = vy;
    odom.twist.twist.angular.z = vth;

    //publish the message
    pub_.publish(odom);
    _PreviousLeftEncoderCounts = tick_x;
    _PreviousRightEncoderCounts = tick_y;

    last_time = current_time;
  }

    public:

  OdometryPublisher() :
      tf_listener_(tf_buffer_)
    {
      //publish Odometry messages to this topic
      pub_ = nh.advertise<nav_msgs::Odometry>("/roboclaw_odom", 1);

      //encoder Int32MultiArray messages are received from the Arduino on this topic
      sub_ = nh.subscribe("/encoder_service", 1, &OdometryPublisher::odometryCallBack, this);
      
      //listen for Twist messages on /cmd_vel
      //sub_cmd_vel("cmd_vel", 1, Twist, self.cmd_vel_callback, 1)

      //read_encoder_cmd is used to send messages to the Arduino to request an encoder update
      odom_req = nh.advertise<nav_msgs::Empty>("/read_encoder_cmd", 1, &OdometryPublisher::odometryCallBack, this);

      //wheel speed publisher
      //wheels_speeds_pub = nh.advertise<Wheel_speeds>("/motors/commanded_speeds", 1);
      //motor current publisher
      //motor_current_pub = nh.advertise<Motors_currents>("/motors/read_current", 1);

      ROS_INFO("Connecting to roboclaw");
      std::string dev_name, baud_rate;
      int baud_rate, address;
      nh.param("~dev", dev_name, "/dev/ttyACM0");
      nh.param("~baud", baud_rate, 38400);
      nh.param("~address", address, 128);
      
      if (address > 0x87 || self.address < 0x80) {
        ROS_INFO("Address out of range");
      }

        //try to open the Roboclaw device try:
            /* roboclaw.Open(dev_name, baud_rate) */
        /* except Exception as e: */
            /* rospy.logfatal("Could not connect to Roboclaw") */
            /* rospy.logdebug(e) */
            /* rospy.signal_shutdown("Could not connect to Roboclaw") */

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

        double MAX_ABS_LINEAR_SPEED, MAX_ABS_ANGULAR_SPEED, TICKS_PER_METER, BASE_WIDTH, ACC_LIM;
        nh.param("~max_abs_linear_speed", MAX_ABS_LINEAR_SPEED, 1.0);
        nh.param("~max_abs_angular_speed", MAX_ABS_ANGULAR_SPEED, 1.0);
        nh.param("~ticks_per_meter", TICKS_PER_METER, 6683);
        nh.param("~base_width", BASE_WIDTH, 0.315);
        nh.param("~acc_lim", ACC_LIM, 0.1);


        /* self.encodm = EncoderOdom(self.TICKS_PER_METER, self.BASE_WIDTH) */
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

        rospy.sleep(1)


    }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "roboclaw_node");
  OdometryPublisher odom_pub;
  ros::spin();
}

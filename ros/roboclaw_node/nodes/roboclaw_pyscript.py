#!/usr/bin/env python
from math import pi, cos, sin, copysign

import diagnostic_msgs
import diagnostic_updater
import roboclaw_driver.roboclaw_driver as roboclaw
import rospy
import tf
from roboclaw_node.msg import Motors_currents, Wheels_speeds
from geometry_msgs.msg import Quaternion, Twist
from nav_msgs.msg import Odometry
from std_msgs.msg import Empty, Int32MultiArray
import threading

__author__ = "Original: bwbazemore@uga.edu (Brad Bazemore); \
             Forked and modified 21/10/2016: guillaumedoisy@gmail.com"


# TODO need to find some better was of handling OSerror 11 or preventing it,
#  any ideas?

class EncoderOdom:
    def __init__(self, ticks_per_meter, base_width):
        self.TICKS_PER_METER = ticks_per_meter
        self.BASE_WIDTH = base_width

        #publish Odometry messages to this topic
        self.odom_pub = rospy.Publisher('/roboclaw_odom', Odometry,
                                        queue_size=1)
        self.cur_x = 0
        self.cur_y = 0
        self.cur_theta = 0.0
        self.last_enc_left = 0
        self.last_enc_right = 0
        self.last_enc_time = rospy.Time.now()
        self.left_tick_vel = 0
        self.right_tick_vel = 0

        #encoder Int32MultiArray messages are received from the Arduino on
        #this topic
        rospy.Subscriber("encoder_service", Int32MultiArray,
                         self.update_publish_cb, queue_size=1)

    @staticmethod
    def normalize_angle(angle):
        while angle > pi:
            angle -= 2.0 * pi
        while angle < -pi:
            angle += 2.0 * pi
        return angle

    def update_publish_cb(self, enc_msg):
        #this callback is called when a new encoder message is
        #received from the Arduino

        enc_left = enc_msg.data[0];
        enc_right = enc_msg.data[1];
        # enc_right = -1.0 * enc_msg.data[1];

        # 2106 per 0.1 seconds is max speed, error in the 16th bit is 32768
        # todo lets find a better way to deal with this error
        if abs(enc_left - self.last_enc_left) > 20000:
            rospy.logerr("Ignoring left encoder jump: cur %d, last %d" % (enc_left, self.last_enc_left))
        elif abs(enc_right - self.last_enc_right) > 20000:
            rospy.logerr("Ignoring right encoder jump: cur %d, last %d" % (enc_right, self.last_enc_right))
        else:
            #call the update function
            vel_x, vel_theta = self.update(enc_left, enc_right)
            self.publish_odom(self.cur_x, self.cur_y, self.cur_theta, vel_x, vel_theta)

    def update(self, enc_left, enc_right):
        #take the encoder counts and update the number of ticks traveled
        left_ticks = enc_left - self.last_enc_left
        right_ticks = enc_right - self.last_enc_right
        self.last_enc_left = enc_left
        self.last_enc_right = enc_right

        dist_left = left_ticks / self.TICKS_PER_METER
        dist_right = right_ticks / self.TICKS_PER_METER
        dist = (dist_right + dist_left) / 2.0

        current_time = rospy.Time.now()
        d_time = (current_time - self.last_enc_time).to_sec()
        self.last_enc_time = current_time

        last_left_vel = left_ticks / d_time #ticks/second
        last_right_vel = right_ticks / d_time
        if last_left_vel != 0 and last_right_vel != 0:
            self.left_tick_vel =  left_ticks / d_time #should be in ticks/second
            self.right_tick_vel = right_ticks / d_time

        # TODO find better what to determine going straight,
        # this means slight deviation is accounted
        if left_ticks == right_ticks:
            d_theta = 0.0
            self.cur_x += dist * cos(self.cur_theta)
            self.cur_y += dist * sin(self.cur_theta)
        else:
            #reverse the d_theta term
            d_theta = (dist_right - dist_left) / self.BASE_WIDTH
            r = dist / d_theta
            self.cur_x += r * (sin(d_theta + self.cur_theta) -
                               sin(self.cur_theta))
            self.cur_y -= r * (cos(d_theta + self.cur_theta) -
                               cos(self.cur_theta))
            self.cur_theta = self.normalize_angle(self.cur_theta - d_theta)

        if abs(d_time) < 0.000001:
            vel_x = 0.0
            vel_theta = 0.0
        else:
            vel_x = dist / d_time
            vel_theta = d_theta / d_time

        return vel_x, vel_theta

    def publish_odom(self, cur_x, cur_y, cur_theta, vx, vth):
        #publish a new odometry message

        quat = tf.transformations.quaternion_from_euler(0, 0, cur_theta)
        current_time = rospy.Time.now()

        br = tf.TransformBroadcaster()
        br.sendTransform((cur_x, cur_y, 0),
                         tf.transformations.quaternion_from_euler(0, 0, cur_theta),
                         current_time,
                         "base_link",
                         "odom")

        odom = Odometry()
        odom.header.stamp = current_time
        odom.header.frame_id = 'odom'

        odom.pose.pose.position.x = cur_x
        odom.pose.pose.position.y = cur_y
        odom.pose.pose.position.z = 0.0
        odom.pose.pose.orientation = Quaternion(*quat)

        odom.pose.covariance[0] = 0.01
        odom.pose.covariance[7] = 0.01
        odom.pose.covariance[14] = 99999
        odom.pose.covariance[21] = 99999
        odom.pose.covariance[28] = 99999
        odom.pose.covariance[35] = 0.01

        odom.child_frame_id = 'base_link'
        odom.twist.twist.linear.x = vx
        odom.twist.twist.linear.y = 0
        odom.twist.twist.angular.z = vth
        odom.twist.covariance = odom.pose.covariance

        self.odom_pub.publish(odom)

class Node:
    def __init__(self):
        self.lock = threading.Lock()
        self.ERRORS = {0x0000: (diagnostic_msgs.msg.DiagnosticStatus.OK, "Normal"),
                       0x0001: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "M1 over current"),
                       0x0002: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "M2 over current"),
                       0x0004: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Emergency Stop"),
                       0x0008: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Temperature1"),
                       0x0010: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Temperature2"),
                       0x0020: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Main batt voltage high"),
                       0x0040: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Logic batt voltage high"),
                       0x0080: (diagnostic_msgs.msg.DiagnosticStatus.ERROR, "Logic batt voltage low"),
                       0x0100: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "M1 driver fault"),
                       0x0200: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "M2 driver fault"),
                       0x0400: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "Main batt voltage high"),
                       0x0800: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "Main batt voltage low"),
                       0x1000: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "Temperature1"),
                       0x2000: (diagnostic_msgs.msg.DiagnosticStatus.WARN, "Temperature2"),
                       0x4000: (diagnostic_msgs.msg.DiagnosticStatus.OK, "M1 home"),
                       0x8000: (diagnostic_msgs.msg.DiagnosticStatus.OK, "M2 home")}

        rospy.init_node("roboclaw_node",log_level=rospy.DEBUG)
        rospy.on_shutdown(self.shutdown)
        rospy.loginfo("Connecting to roboclaw")
        dev_name = rospy.get_param("~dev", "/dev/ttyACM0")
        baud_rate = int(rospy.get_param("~baud", "38400"))
        self.wheels_speeds_pub = rospy.Publisher('/motors/commanded_speeds', Wheels_speeds, queue_size=1)
        self.motors_currents_pub = rospy.Publisher('/motors/read_currents', Motors_currents, queue_size=1)

        self.address = int(rospy.get_param("~address", "128"))
        if self.address > 0x87 or self.address < 0x80:
            rospy.logfatal("Address out of range")
            rospy.signal_shutdown("Address out of range")

        #try to open the Roboclaw device
        try:
            roboclaw.Open(dev_name, baud_rate)
        except Exception as e:
            rospy.logfatal("Could not connect to Roboclaw")
            rospy.logdebug(e)
            rospy.signal_shutdown("Could not connect to Roboclaw")

        self.updater = diagnostic_updater.Updater()
        self.updater.setHardwareID("Roboclaw")
        self.updater.add(diagnostic_updater.
                         FunctionDiagnosticTask("Vitals", self.check_vitals))

        try:
            version = roboclaw.ReadVersion(self.address)
        except Exception as e:
            rospy.logwarn("Problem getting roboclaw version")
            rospy.logdebug(e)
            pass

        if not version[0]:
            rospy.logwarn("Could not get version from roboclaw")
        else:
            rospy.logdebug(repr(version[1]))

        roboclaw.SpeedM1M2(self.address, 0, 0)
        #roboclaw.ResetEncoders(self.address)

        self.MAX_ABS_LINEAR_SPEED = float(rospy.get_param("~max_abs_linear_speed", \
                                                           "1.0"))
        self.MAX_ABS_ANGULAR_SPEED = float(rospy.get_param("~max_abs_angular_speed", \
                                                            "1.0"))
        self.TICKS_PER_METER = float(rospy.get_param("~ticks_per_meter", "6683"))
        self.BASE_WIDTH = float(rospy.get_param("~base_width", "0.315"))
        self.ACC_LIM = float(rospy.get_param("~acc_lim", "0.1"))

        self.encodm = EncoderOdom(self.TICKS_PER_METER, self.BASE_WIDTH)
        self.left_integral = [x for x in range(5)]
        self.right_integral = [x for x in range(5)]
        self.left_counter = 0
        self.right_counter = 0
        self.left_pwm = 0 #current PWM values sent to Roboclaw
        self.right_pwm = 0
        self.last_set_speed_time = rospy.get_rostime()

        #listen for Twist messages on /cmd_vel
        rospy.Subscriber("cmd_vel", Twist, self.cmd_vel_callback, queue_size=1)

        #/read_encoder_cmd is used to send messages to the Arduino to request
        #an encoder update
        self.odom_req = rospy.Publisher('/read_encoder_cmd', Empty, queue_size=1)

        rospy.sleep(1)

        rospy.logdebug("dev %s", dev_name)
        rospy.logdebug("baud %d", baud_rate)
        rospy.logdebug("address %d", self.address)
        rospy.logdebug("max_abs_linear_speed %f", self.MAX_ABS_LINEAR_SPEED)
        rospy.logdebug("max_abs_angular_speed %f", self.MAX_ABS_ANGULAR_SPEED)
        rospy.logdebug("ticks_per_meter %f", self.TICKS_PER_METER)
        rospy.logdebug("base_width %f", self.BASE_WIDTH)

    def run(self):
        rospy.loginfo("Starting motor drive")
        r_time = rospy.Rate(30)
        while not rospy.is_shutdown():
            with self.lock:
                if (rospy.get_rostime() - self.last_set_speed_time).to_sec() > 1:
                    rospy.loginfo("Did not get command for 1 second, stopping")
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

                #if (enc1 != None) and (enc2 != None):
                #    rospy.logdebug(" Encoders %d %d" % (enc1, enc2))
                #    self.encodm.update_publish(enc1, enc2)
                #    self.updater.update()
                #else:
                #    rospy.logdebug("Error Reading enc")

                if (amp1 != None) & (amp2 != None):
                    rospy.logdebug(" Currents %d %d" % (amp1, amp2))
                    amps=Motors_currents()
                    amps.motor1=float(amp1)/100.0
                    amps.motor2=float(amp2)/100.0
                    self.motors_currents_pub.publish(amps)
                else:
                    rospy.logdebug("Error Reading Currents")

            r_time.sleep()

    def compute_pid(self, left_desired, left_actual, right_desired, right_actual):
        kp = 5
        ki = .7
        left_error = left_desired - left_actual
        right_error = right_desired - right_actual

        #compute integral term
        left_sum = 0
        right_sum = 0
        for i in range(len(self.left_integral)):
            left_sum += self.left_integral[i]
            right_sum += self.right_integral[i]

        if left_sum > 10000 or left_sum < -10000:
            left_sum = 10000

        if right_sum > 10000 or right_sum < -10000:
            right_sum = 10000

        self.left_counter = (self.left_counter + 1) % len(self.left_integral)
        self.right_counter = (self.right_counter + 1) % len(self.right_integral)
        self.left_integral[self.left_counter] = left_error
        self.right_integral[self.right_counter] = right_error

        left_set_value = (kp * left_error) + (ki * left_sum)
        right_set_value = (kp * right_error) + (ki * right_sum)

        return left_set_value, right_set_value

    def cmd_vel_callback(self, twist):
        with self.lock:
            self.last_set_speed_time = rospy.get_rostime()

            linear_x = twist.linear.x
            angular_z = -twist.angular.z
            if abs(linear_x) > self.MAX_ABS_LINEAR_SPEED:
                linear_x = copysign(self.MAX_ABS_LINEAR_SPEED, linear_x)
            if abs(angular_z) > self.MAX_ABS_ANGULAR_SPEED:
                angular_z = copysign(self.MAX_ABS_ANGULAR_SPEED, angular_z)

            vr = linear_x - angular_z * self.BASE_WIDTH / 2.0  # m/s
            vl = linear_x + angular_z * self.BASE_WIDTH / 2.0

            vr_ticks = int(vr * self.TICKS_PER_METER)  # ticks/s
            vl_ticks = int(vl * self.TICKS_PER_METER)

            #publish the wheel speeds to /motors/commanded_speeds
            v_wheels= Wheels_speeds()
            #v_wheels.wheel1=vl
            #v_wheels.wheel2=vr
            pid_speed_l, pid_speed_r = self.compute_pid(vl_ticks, self.encodm.left_tick_vel, \
                                                        vr_ticks, self.encodm.right_tick_vel)
            v_wheels.wheel1=pid_speed_l
            v_wheels.wheel2=pid_speed_r
            self.wheels_speeds_pub.publish(v_wheels)

            rospy.logdebug("desired vl_ticks: %d desired vr_ticks: %d", vl_ticks, vr_ticks)
            rospy.logdebug("current left vel: %d current right vel: %d", \
                            self.encodm.left_tick_vel, self.encodm.right_tick_vel)
            rospy.logdebug("left error: %d right error: %d", \
                            vl_ticks - self.encodm.left_tick_vel, \
                            vr_ticks - self.encodm.right_tick_vel)

            left_pid_output = int(pid_speed_l)
            right_pid_output = int(pid_speed_r)
            rospy.logdebug("left pid output: %d right pid output: %d", \
                            left_pid_output, \
                            right_pid_output)

            try:
                #Send motor commands to the Roboclaw
                roboclaw.DutyM1(self.address, -left_pid_output)
                roboclaw.DutyM2(self.address, -right_pid_output)
            except OSError as e:
                rospy.logwarn("SpeedM1M2 OSError: %d", e.errno)
                rospy.logdebug(e)

    # TODO: Need to make this work when more than one error is raised
    def check_vitals(self, stat):
        try:
            status = roboclaw.ReadError(self.address)[1]
        except OSError as e:
            rospy.logwarn("Diagnostics OSError: %d", e.errno)
            rospy.logdebug(e)
            return
        state, message = self.ERRORS[status]
        stat.summary(state, message)
        try:
            stat.add("Main Batt V:", float(roboclaw.ReadMainBatteryVoltage(self.address)[1] / 10.0 + 0.1)) #JS offset battery by 0.1V
            stat.add("Logic Batt V:", float(roboclaw.ReadLogicBatteryVoltage(self.address)[1] / 10.0))
            stat.add("Temp1 C:", float(roboclaw.ReadTemp(self.address)[1] / 10.0))
            stat.add("Temp2 C:", float(roboclaw.ReadTemp2(self.address)[1] / 10.0))
        except OSError as e:
            rospy.logwarn("Diagnostics OSError: %d", e.errno)
            rospy.logdebug(e)
        return stat

    # TODO: need clean shutdown so motors stop even if new msgs are arriving
    def shutdown(self):
        rospy.loginfo("Shutting down")
        try:
            roboclaw.ForwardM1(self.address, 0)
            roboclaw.ForwardM2(self.address, 0)
        except OSError:
            rospy.logerr("Shutdown did not work trying again")
            try:
                roboclaw.ForwardM1(self.address, 0)
                roboclaw.ForwardM2(self.address, 0)
            except OSError as e:
                rospy.logerr("Could not shutdown motors!!!!")
                rospy.logdebug(e)

if __name__ == "__main__":
    try:
        node = Node()
        node.run()
    except rospy.ROSInterruptException:
        pass
    rospy.loginfo("Exiting")

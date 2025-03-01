// The implementation below is mostly based off of
// the document written by 5225A (πlons)
// Here is a link to the original document
// http://thepilons.ca/wp-content/uploads/2018/10/Tracking.pdf

//#include <atomic>
#include <cstdio>
#include <math.h>
#include <mutex>
// #include <mutex>
// #include <string.h>
//  #include "fmt/core.h"
#include "display.h"
// #include "fmt/format.h"
#include "pros/rtos.hpp"
#include "pros/serial.hpp"
#include "lemlib/util.hpp"
#include "lemlib/chassis/odom.hpp"
#include "lemlib/chassis/chassis.hpp"
#include "musty/cobs.h"
#include "lemlib/chassis/odom.hpp"
#include "ports.h"

#define MUSTY_BAUDRATE 460800

#define TRANSMIT_PACKET_SIZE 2
#define MAX_BUFFER_SIZE 256
#define RECEIVE_OTOS_PACKET_SIZE 21

#define COMMAND_READ_LIDAR 200
#define COMMAND_READ_ENCODERS 100

#define COMMAND_READ_OTOS 201
#define COMMAND_CALIBRATE_OTOS_RED 0
#define COMMAND_CALIBRATE_OTOS_BLUE 255
// #define COMMAND_CALIBRATE_OTOS 202

#define DEBUGGING_PACKETS false

bool red_alliance = false;

bool should_calibrate = false;

// tracking thread
pros::Task* trackingTask = nullptr;

// pros::Mutex* tracking_mutex = nullptr;
pros::Mutex tracking_mutex = pros::Mutex();

// global variables
std::unique_ptr<pros::Serial> s;
lemlib::Drivetrain drive(nullptr, nullptr, 0, 0, 0, 0); // the drivetrain to be used for odometry
lemlib::Pose odomPose(0, 0, 0); // the pose of the robot
lemlib::Pose odomSpeed(0, 0, 0); // the speed of the robot
lemlib::Pose odomLocalSpeed(0, 0, 0); // the local speed of the robot

lemlib::Pose offsetPose(0, 0, 0); // the offset of the robot

pros::Task* enable_task = nullptr;
int old_success_num = 0;
int success_read_num = 0;

void lemlib::setDrivetrain(lemlib::Drivetrain drivetrain) { drive = drivetrain; }

lemlib::Pose lemlib::getPose(bool radians) {
  if (radians) return odomPose;
  else return lemlib::Pose(odomPose.x, odomPose.y, radToDeg(odomPose.theta));
}

void lemlib::setPose(lemlib::Pose pose, bool radians) {
  if (radians) odomPose = pose;
  else odomPose = lemlib::Pose(pose.x, pose.y, degToRad(pose.theta));
  offsetPose = odomPose;
}

lemlib::Pose lemlib::getSpeed(bool radians) {
  if (radians) return odomSpeed;
  else return lemlib::Pose(odomSpeed.x, odomSpeed.y, radToDeg(odomSpeed.theta));
}

lemlib::Pose lemlib::getLocalSpeed(bool radians) {
  if (radians) return odomLocalSpeed;
  else return lemlib::Pose(odomLocalSpeed.x, odomLocalSpeed.y, radToDeg(odomLocalSpeed.theta));
}

lemlib::Pose lemlib::estimatePose(float time, bool radians) {
  // get current position and speed
  Pose curPose = getPose(true);
  Pose localSpeed = getLocalSpeed(true);
  // calculate the change in local position
  Pose deltaLocalPose = localSpeed * time;

  // calculate the future pose
  float avgHeading = curPose.theta + deltaLocalPose.theta / 2;
  Pose futurePose = curPose;
  futurePose.x += deltaLocalPose.y * sin(avgHeading);
  futurePose.y += deltaLocalPose.y * cos(avgHeading);
  futurePose.x += deltaLocalPose.x * -cos(avgHeading);
  futurePose.y += deltaLocalPose.x * sin(avgHeading);
  if (!radians) futurePose.theta = radToDeg(futurePose.theta);

  return futurePose;
}

void lemlib::update() {
  // calculate the local speed of the robot
  static Pose lastPose = getPose(true);
  Pose curPose = getPose(true);
  float deltaTime = 0.01; // 10 ms bc sleep call in tracking task

  float deltaX = curPose.x - lastPose.x;
  float deltaY = curPose.y - lastPose.y;
  float deltaTheta = curPose.theta - lastPose.theta;

  float localX = deltaX * cos(curPose.theta) + deltaY * sin(curPose.theta);
  float localY = -deltaX * sin(curPose.theta) + deltaY * cos(curPose.theta);

  odomLocalSpeed = Pose(localX / deltaTime, localY / deltaTime, deltaTheta / deltaTime);

  lastPose = curPose;

  uint8_t transmit_buffer[TRANSMIT_PACKET_SIZE] = {COMMAND_READ_OTOS, 127};
  uint8_t receive_buffer[MAX_BUFFER_SIZE] = {0};
  uint8_t decode_buffer[MAX_BUFFER_SIZE] = {0};

  uint8_t* temp_buf_ptr = receive_buffer;
  int num_read_bytes = 0;
  int num_waiting_bytes = 0;
  int timeout_counter = 0;
  bool read_success = false;

  cobs_decode_result res;

  float x, y, h;
  // print_text_at(4, fmt::format("0: {}, 1: {}", transmit_buffer[0], transmit_buffer[1]).c_str());
  if (should_calibrate) {
    should_calibrate = false;
    if (red_alliance) {
      transmit_buffer[1] = COMMAND_CALIBRATE_OTOS_RED;
    } else {
      transmit_buffer[1] = COMMAND_CALIBRATE_OTOS_BLUE;
    }
    // send the command to the Musty board
    // print_text_at(9, fmt::format("Calibrating OTOS: {}", transmit_buffer[1]).c_str());
    s->write(transmit_buffer, TRANSMIT_PACKET_SIZE);
    return;
  }

  // send the command to the Musty board
  // s->write(transmit_buffer, TRANSMIT_PACKET_SIZE);

  // read the data from the Musty board
  while (true) {
    num_waiting_bytes = s->get_read_avail();

    if (num_waiting_bytes > 0) {
      num_read_bytes = s->read(temp_buf_ptr, num_waiting_bytes);
      if (num_read_bytes != -1) { temp_buf_ptr += num_read_bytes; }
    }

    if (num_read_bytes == RECEIVE_OTOS_PACKET_SIZE) {
      read_success = true;
      break;

    } else if (num_read_bytes > RECEIVE_OTOS_PACKET_SIZE) {
      s->flush();
      break;

    } else {
      timeout_counter++;
      pros::delay(1);
      if (timeout_counter > 10) { break; }
    }
  }
  if (DEBUGGING_PACKETS) {
    // print_text_at(6, fmt::format("num_read_bytes: {}", num_read_bytes).c_str());
    printf("num_read_bytes: %d\n", num_read_bytes);
    // print_text_at(8, fmt::format("num_waiting_bytes: {}", num_waiting_bytes).c_str());
    printf("num_waiting_bytes: %d\n", num_waiting_bytes);
    // print_text_at(9, fmt::format("timeout_counter: {}", timeout_counter).c_str());
    printf("timeout_counter: %d\n", timeout_counter);
    // print_text_at(7, fmt::format("millis: {}", pros::millis()).c_str());
    printf("millis: %d\n", pros::millis());
    printf("read success ? : %d\n", read_success);
  }
  // printf("fish_mech current draw = %d\n", fish_mech.get_current_draw());

  if (read_success) {
    //printf("success_read_num = %d\n", success_read_num);
    success_read_num++;
    res = musty_cobs_decode(decode_buffer, MAX_BUFFER_SIZE, receive_buffer, RECEIVE_OTOS_PACKET_SIZE);
    // printf("res.status = %d\n", res.status);
    if (res.status == COBS_DECODE_OK) {
      x = *reinterpret_cast<float*>(&decode_buffer[0]);
      y = *reinterpret_cast<float*>(&decode_buffer[4]);
      h = *reinterpret_cast<float*>(&decode_buffer[8]);

      h *= -1;
      h += 90; // cartesian to vex coords

      // print_text_at(5, fmt::format("x: {}, y: {}, h: {}", x, y, h).c_str());
      odomPose = lemlib::Pose(-x, -y, degToRad(h)) + offsetPose;
      // Adding poses doesn't add the angle, so we need to add the angle separately
      odomPose.theta += degToRad(offsetPose.theta);
    }
  }
}

void lemlib::calibrate_otos(bool is_red_alliance) {
  should_calibrate = true;
  red_alliance = is_red_alliance;
  // print_text_at(8, "calibrating otos");

  // if red alliance set the odomPose to the red starting position
  if (red_alliance) {
    setPose(RED_STARTING_POSE);
  } else {
    setPose(BLUE_STARTING_POSE);
  }
}

void lemlib::init() {
  // TODO MOVE THE CALIBRATION THING HERE WITH NULL-POINTER CHECK OR WHATEVER
  if (trackingTask == nullptr) {
    pros::Serial s1(MUSTY_PORT,
                    MUSTY_BAUDRATE); // need to initialize the serial port twice, so here is the first time

    pros::delay(100); // Let VEX OS configure port

    s = std::make_unique<pros::Serial>(MUSTY_PORT, MUSTY_BAUDRATE); // create a serial port on #2 at 460800 baud
    pros::delay(100); // Let VEX OS configure port

    trackingTask = new pros::Task {[=] {
                                     while (true) {
                                        tracking_mutex.take();
                                        printf("updating\n");
                                        update();
                                        

                                       tracking_mutex.give();
                                       pros::delay(10);
                                     }
                                    },"odom task"};


                                  
    enable_task = new pros::Task { [=] {
      
      while (1) {
        

        //printf("success = %d, old_success = %d, dist = %d\n", success_read_num, old_success_num, (success_read_num - old_success_num));
        
        tracking_mutex.take();
        printf("checking my data state\n");
        //printf("heloo");
        if ((success_read_num - old_success_num) < 4){
          printf("success_read_num is %d\n", success_read_num);
          //pros::Serial s2(MUSTY_PORT, MUSTY_BAUDRATE);
          //pros::delay(100);
          s = std::make_unique<pros::Serial>(MUSTY_PORT, MUSTY_BAUDRATE);
          pros::delay(100); // let vex os configure port
          
          for (int i = 0; i < 20; i++){
            printf("qqqqqqqqqqqqqqqqqqqqqqqqqqqq\n");
          }
          
        }
        
        old_success_num = success_read_num;
        
        tracking_mutex.give();
        pros::delay(200);
        
      }
    }, "enable task"};
  }
}

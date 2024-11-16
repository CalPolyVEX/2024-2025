#include "main.h"
#include "cobs.h"
#include "pros/serial.hpp"
#include <queue>
#include <mutex>
#include <string.h>

void insert_mqueue(int command);

// Port to use for serial data
#define MUSTY_SERIALPORT 2
#define MUSTY_BAUDRATE 460800
pros::Serial *s;  //create a serial port on #2 at 460800 baud
std::queue<int> mqueue; //commands to Musty are inserted into this queue
pros::Mutex mutex_; //the mutex for the Must command queue


bool is_reading_encoders = false;
bool is_reading_lidar = false; //!is_reading_encoders;
bool is_reading_otos = true;


/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button() {
	static bool pressed = false;
	pressed = !pressed;
	if (pressed) {
    insert_mqueue(101); //toggle LED1
	} else {
    insert_mqueue(101);
	}
}

void on_right_button() { //when the right button is pressed
  insert_mqueue(102);
}



void insert_mqueue(int command) { //insert a command into the Musty Board command queue
  mutex_.take(); //lock the queue
  mqueue.push(command);
  mutex_.give(); //unlock the queue
}

int pop_mqueue() { //return the first command in the Musty Board command queue
  int value = 0;
  mutex_.take(); //lock the queue
  value = mqueue.front();
  mqueue.pop();
  mutex_.give(); //unlock the queue

  return value;
}

void add_commands(void* ignore) { //a debugging task the adds commands to the queue
  while(1){
    insert_mqueue(101); //toggle LED1
    pros::delay(rand() % 50);
  }
}

#define TRANSMIT_PACKET_SIZE 2 //size of packet in bytes sent to the Musty board
#define RECEIVE_ENCODER_PACKET_SIZE 19 // 19 = (4 encoders * 4 bytes) + 2 extra bytes + 1 COBS delimiter
#define MAX_BUFFER_SIZE 256 //buffer for receiving bytes from the Musty board
#define RECIEVE_LIDAR_PACKET_SIZE 5 // 4 byte float + COBS
#define RECIEVE_OTOS_PACKET_SIZE 21 // ((float) x, y, h) (float)(localXVelo, localYVelo) + 1-byte COBS delimiter


//process_musty_commands_task - this task sends commands to read the encoders and 
//takes any command in the Musty command queue and send them to the board 
void process_musty_commands_task(void* ignore) {
  pros::Serial s1(MUSTY_SERIALPORT, MUSTY_BAUDRATE);  //need to initialize the serial port twice, so here is the first time
  pros::delay(100); // Let VEX OS configure port

  s = new pros::Serial(MUSTY_SERIALPORT, MUSTY_BAUDRATE);  //create a serial port on #2 at 460800 baud
  pros::delay(100); // Let VEX OS configure port
    
	int receive_counter = 0;
  int send_counter = 0;
	pros::lcd::print(2, "  Enc1  |  Enc2  |  Enc3  |  Enc4\n");
      
  // Buffers to store serial data
  uint8_t transmit_buf[TRANSMIT_PACKET_SIZE]; //data transmitted from the VEX brain
  uint8_t receive_buf[MAX_BUFFER_SIZE]; //data receive buffer before COBS decoding
  uint8_t decode_buf[MAX_BUFFER_SIZE]; //buffer after COBS decoding

  uint8_t* temp_buf_ptr;
  cobs_decode_result res; //used to indicate COBS decoding status
  int num_waiting_bytes;
  int num_read_bytes;
  int last_time = pros::millis();
  int loop_counter = 0, cobs_error_count = 0, receive_timeout_counter = 0;
  int encoder1, encoder2, encoder3, encoder4;
  float lidar1;
  float x, y, h;

  
  int command_val = 100;

  if (is_reading_lidar){
    command_val = 200;
  } else if (is_reading_encoders){
    command_val = 100;
  } else if (is_reading_otos){
    command_val = 201;
  }

  transmit_buf[0] = command_val; // send the header byte to request sensor readings (value: 100)
  transmit_buf[1] = 255; // send 1 other dummy value

  while (true) {
    receive_timeout_counter = 0;
    temp_buf_ptr = receive_buf; //reset temp pointer to point to the start of the receive buffer
    num_read_bytes = 0;

    //check command queue
    mutex_.take(); //lock the command queue
    if (mqueue.size() != 0) {
      mutex_.give(); //unlock the command queue
      int q_value = pop_mqueue();
      transmit_buf[0] = q_value; // send the header byte for the command
      s->write(transmit_buf,TRANSMIT_PACKET_SIZE); //send 2 bytes to request data from Musty board
      transmit_buf[0] = command_val; // set the header byte to request readings (value: 100 for encoders)
    } else { // queue is empty
      mutex_.give(); //unlock the command queue for us to use
    }
    //transmit buf is equal to : {command_val, 255} here
    s->write(transmit_buf, TRANSMIT_PACKET_SIZE); //send 2 bytes to request data from Musty board

    while (1) { //loop until a full data packet received from Musty board
      num_waiting_bytes = s->get_read_avail(); //check if there are bytes available

      if (num_waiting_bytes > 0) { //if there are bytes waiting to be read
        num_read_bytes += s->read(temp_buf_ptr, num_waiting_bytes);
        temp_buf_ptr += num_read_bytes;
      }

      bool has_completed_reading_a_task = (num_read_bytes == RECEIVE_ENCODER_PACKET_SIZE && is_reading_encoders) 
      || (num_read_bytes == RECIEVE_LIDAR_PACKET_SIZE && is_reading_lidar);

      bool has_sync_error = (num_read_bytes > RECEIVE_ENCODER_PACKET_SIZE && is_reading_encoders) 
      || (num_read_bytes > RECIEVE_LIDAR_PACKET_SIZE && is_reading_lidar);

      if (has_completed_reading_a_task) { //received a full packet
        break;
      } else if (has_sync_error) { //synchronization error
        s->flush();
        break;
      } else { //did not receive a full packet yet
        receive_timeout_counter++;
        pros::delay(1);

        if (receive_timeout_counter > 10) { //if no data has been received for 10ms
          receive_timeout_counter = 0;
          break;
        }
      }
    }

    if (num_read_bytes == RECEIVE_ENCODER_PACKET_SIZE && is_reading_encoders) { //received the correct packet size
      res = cobs_decode(decode_buf, MAX_BUFFER_SIZE, receive_buf, RECEIVE_ENCODER_PACKET_SIZE);
      pros::lcd::print(2, "cobs res status : " + res.status);
      if (res.status == COBS_DECODE_OK) { //if the packet checksums ok
        encoder1 = (decode_buf[3] << 24) | (decode_buf[2] << 16) | (decode_buf[1] << 8) | decode_buf[0];
        encoder2 = (decode_buf[7] << 24) | (decode_buf[6] << 16) | (decode_buf[5] << 8) | decode_buf[4];
        encoder3 = (decode_buf[11] << 24) | (decode_buf[10] << 16) | (decode_buf[9] << 8) | decode_buf[8];
        encoder4 = (decode_buf[15] << 24) | (decode_buf[14] << 16) | (decode_buf[13] << 8) | decode_buf[12];

        pros::lcd::print(3, "%7d |%7d |%7d |%7d\n", encoder1, encoder2, encoder3, encoder4);
        pros::lcd::print(6, "receive packets: %d", receive_counter);

        receive_counter++;
        loop_counter++;
      } else {
        cobs_error_count++;
        pros::lcd::print(5, "COBS error in encoders: %d", cobs_error_count);
      }
    } else if (num_read_bytes == RECIEVE_LIDAR_PACKET_SIZE && is_reading_lidar){ //recieved the correct packet size and doing lidar
      
      res = cobs_decode(decode_buf, MAX_BUFFER_SIZE, receive_buf, RECIEVE_LIDAR_PACKET_SIZE);

      //pros::lcd::print(5, "res status:" + res.status);
      //pros::lcd::print(5, "COBS error in lidar: %d", cobs_error_count);
      if (res.status == COBS_DECODE_OK){ //if the packet checksums ok
        lidar1 = (decode_buf[0] << 24) | (decode_buf[1] << 16) | (decode_buf[2] << 8) | decode_buf[3];
        memcpy(&lidar1, decode_buf, sizeof(float));
        pros::lcd::print(4, "Lidar Dist = %5.2f mm", lidar1);
        pros::lcd::print(6, "receive packets: %d", receive_counter);

        receive_counter++;
        loop_counter++;
      } else {
        cobs_error_count++;
        
        pros::lcd::print(5, "COBS error in lidar: %d", cobs_error_count);
        pros::lcd::print(6, "res status:" + res.status);
      }
    } else if (num_read_bytes == RECIEVE_OTOS_PACKET_SIZE){
      
      res = cobs_decode(decode_buf, MAX_BUFFER_SIZE, receive_buf, RECIEVE_OTOS_PACKET_SIZE);
      if (res.status == COBS_DECODE_OK){
       
        memcpy(&x, &decode_buf[0], sizeof(float));
        memcpy(&y, &decode_buf[4], sizeof(float));
        memcpy(&h, &decode_buf[8], sizeof(float));

        
        
        pros::lcd::print(4, "x =  %5.2f, y = %5.2f, h = %5.2f\n", x, y, h);
        pros::lcd::print(6, "receive packets: %d\n", receive_counter);

        receive_counter++;
        loop_counter++;

      } else {
        cobs_error_count++;
        pros::lcd::print(5, "COBS error in otos: %d\n", cobs_error_count);
      }
    }




    // update the counter display rate
    if (pros::millis() > (last_time + 3000)) {
      //this prints the rate at which encoder readings are sent to the VEX brain
      pros::lcd::print(7, "update rate: %.2f Hz  \n", loop_counter/3.0);
      loop_counter = 0;
      last_time = pros::millis();
    }
    
    pros::delay(2);
  }
}



/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::lcd::initialize();
  pros::delay(300);
	pros::lcd::set_text(0, "Musty Board PROS Test");

	pros::lcd::register_btn1_cb(on_center_button);
	//pros::lcd::register_btn2_cb(on_right_button);
  

	//JS
  pros::Task my_task(process_musty_commands_task, (void*) "args", "test task");
  //pros::Task my_task2(add_commands, (void*) "args", "test task 2");
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
  while(1) {
    pros::delay(20);
  }
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
  while(1) {
    pros::delay(20);
  }

	pros::Controller master(pros::E_CONTROLLER_MASTER);
	pros::MotorGroup left_mg({1, -2, 3});    // Creates a motor group with forwards ports 1 & 3 and reversed port 2
	pros::MotorGroup right_mg({-4, 5, -6});  // Creates a motor group with forwards port 5 and reversed ports 4 & 6


	while (true) {
		pros::lcd::print(0, "%d %d %d", (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
		                 (pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
		                 (pros::lcd::read_buttons() & LCD_BTN_RIGHT) >> 0);  // Prints status of the emulated screen LCDs

		// Arcade control scheme
		int dir = master.get_analog(ANALOG_LEFT_Y);    // Gets amount forward/backward from left joystick
		int turn = master.get_analog(ANALOG_RIGHT_X);  // Gets the turn left/right from right joystick
		left_mg.move(dir - turn);                      // Sets left motor voltage
		right_mg.move(dir + turn);                     // Sets right motor voltage
		pros::delay(20);                               // Run for 20 ms then update
	}
}
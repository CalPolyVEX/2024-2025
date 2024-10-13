#include "main.h"
#include "cobs.h"
#include "pros/serial.hpp"

// Prototypes for hidden vex functions to bypass PROS bug
//extern "C" int32_t vexGenericSerialReceive( uint32_t index, uint8_t *buffer, int32_t length );
//extern "C" int32_t vexGenericSerialTransmit( uint32_t index, uint8_t *buffer, int32_t length );
//extern "C" void vexGenericSerialEnable(  uint32_t index, uint32_t nu );
//extern "C" void vexGenericSerialBaudrate(  uint32_t index, uint32_t rate );

// Port to use for serial data
#define SERIALPORT 2

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
		pros::lcd::set_text(2, "I was pressed!");
	} else {
		pros::lcd::clear_line(2);
	}
}

#define RECEIVE_PACKET_SIZE 19 // (4 encoders * 4 bytes) + 2 extra bytes + 1 COBS delimiter
#define MAX_BUFFER_SIZE 256

void test_musty_task(void* ignore) {
  pros::Serial s(2, 460800);  //create a serial port on #2 at 460800 baud

  // Let VEX OS configure port
  pros::delay(50);
    
	int receive_counter = 0;
  int send_counter = 0;
	pros::lcd::print(2, "  Enc1  |  Enc2  |  Enc3  |  Enc4");
      
  // Buffers to store serial data
  uint8_t transmit_buf[4]; //data transmitted from the VEX brain
  uint8_t receive_buf[MAX_BUFFER_SIZE]; //data receive buffer before COBS decoding

  //this is the buffer where the data is stored after COBS decoding
  //make sure this buffer is 4-byte aligned since int pointers are
  //used to access the encoder data
  uint8_t decode_buf[MAX_BUFFER_SIZE] __attribute__((aligned(4))); //buffer after COBS decoding

  uint8_t* temp_buf_ptr;
  cobs_decode_result res; //used to indicate COBS decoding status
  int* encoder_ptr;
  int num_waiting_bytes;
  int num_read_bytes;
  int last_time = pros::millis();
  int loop_counter = 0, cobs_error_count = 0, receive_timeout_counter = 0;

  transmit_buf[0] = 100; // send the header byte to request encoder readings (value: 100)
  transmit_buf[1] = 101; // send 3 other dummy values
  transmit_buf[2] = 102; 
  transmit_buf[3] = 103; 

  while (true) {
      s.write(transmit_buf,4); //send 4 bytes to request data from Musty board
      temp_buf_ptr = receive_buf; //reset temp pointer to point to the start of the receive buffer
      num_read_bytes = 0;
      //receive_timeout_counter = 0;

      while (1) { //loop until a full data packet received from Musty board
        num_waiting_bytes = s.get_read_avail(); //check if there are bytes available

        if (num_waiting_bytes > 0) { //if there are bytes waiting to be read
          num_read_bytes += s.read(temp_buf_ptr, num_waiting_bytes);
          temp_buf_ptr += num_read_bytes;
        }

        if (num_read_bytes == RECEIVE_PACKET_SIZE) { //received a full packet
          break;
        } else if (num_read_bytes > RECEIVE_PACKET_SIZE) { //synchronization error
          s.flush();
          break;
        } else { //did not receive a full packet yet
          receive_timeout_counter++;
          pros::delay(1);

          if (receive_timeout_counter > 200) { //if no data has been received for 20ms
            receive_timeout_counter = 0;
            break;
          }
        }
      }

      if (num_read_bytes == RECEIVE_PACKET_SIZE) { //received the correct packet size
        res = cobs_decode(decode_buf, MAX_BUFFER_SIZE, receive_buf, RECEIVE_PACKET_SIZE);

        if (res.status == COBS_DECODE_OK) { //if the packet checksums ok
          //encoder data is sent LSB first, so we can access using an int pointer
          encoder_ptr = (int*) decode_buf;
          int encoder1 = *encoder_ptr;
          encoder_ptr++;
          int encoder2 = *encoder_ptr;
          encoder_ptr++;
          int encoder3 = *encoder_ptr;
          encoder_ptr++;
          int encoder4 = *encoder_ptr;
          // int encoder1 = (decode_buf[0] << 24) | (decode_buf[1] << 16) | (decode_buf[2] << 8) | decode_buf[3];
          // int encoder2 = (decode_buf[4] << 24) | (decode_buf[5] << 16) | (decode_buf[6] << 8) | decode_buf[7];
          // int encoder3 = (decode_buf[8] << 24) | (decode_buf[9] << 16) | (decode_buf[10] << 8) | decode_buf[11];
          // int encoder4 = (decode_buf[12] << 24) | (decode_buf[13] << 16) | (decode_buf[14] << 8) | decode_buf[15];

          pros::lcd::print(3, "%7d |%7d |%7d |%7d", encoder1, encoder2, encoder3, encoder4);
          pros::lcd::print(6, "receive packets: %d", receive_counter);
          receive_counter++;
          loop_counter++;
        } else {
          cobs_error_count++;
          pros::lcd::print(5, "COBS error: %d", cobs_error_count);
        }
      }

      // update the counter display rate
      if (pros::millis() > (last_time + 3000)) {
        pros::lcd::print(7, "update rate: %.2f Hz  \n", loop_counter/3.0);
        loop_counter = 0;
        last_time = pros::millis();
      }

      pros::delay(1);
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

	//JS
  pros::Task my_task(test_musty_task, (void*) "args", "test task");
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
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

#define RECEIVE_PACKET_SIZE 19

void test_serial2() {
  pros::Serial s(2, 460800);  //create a serial port on #2 at 460800 baud

  // Let VEX OS configure port
  pros::delay(10);
    
	int receive_counter = 0;
  int send_counter = 0;
	pros::lcd::print(2, "  Enc1  |  Enc2  |  Enc3  |  Enc4");
      
  // Buffer to store serial data
  uint8_t output[10];
  uint8_t buffer[64]; //buffer before COBS decoding
  uint8_t decode[64]; //buffer after COBS decoding
  uint8_t* temp;
  cobs_decode_result res;
  int num_waiting_bytes;
  int num_read_bytes;
  int last_time = pros::millis();
  int loop_counter = 0;

  while (true) {
      output[0] = 100; //send 'd'
      output[1] = 101; //send 'e'
      output[2] = 102; //send 'f'
      
      temp = buffer;
      num_read_bytes = 0;

      while (1) {
        num_waiting_bytes = s.get_read_avail();
        //pros::lcd::print(4, "available bytes: %d", num_waiting_bytes);

        if (num_waiting_bytes > 0) {
          num_read_bytes += s.read(temp, num_waiting_bytes);
          temp += num_waiting_bytes;
        }

        if (num_read_bytes == RECEIVE_PACKET_SIZE) { //received a full packet
          break;
        } else if (num_read_bytes > RECEIVE_PACKET_SIZE) { //synchronization error
          s.flush();
          break;
        } else { //did not receive a full packet yet
          pros::delay(1);
          //pros::lcd::print(5, "read bytes: %d", num_read_bytes);
        }
      }

      // pros::lcd::print(5, "avail: %d", num_waiting_bytes);
      // if (num_waiting_bytes) {
      //   if (num_waiting_bytes != 19) {
      //     pros::lcd::print(5, "avail: %d", num_waiting_bytes);
      //   }

      //   num_read_bytes = s.read(buffer, RECEIVE_PACKET_SIZE);
      //   s.flush();
      // }
      
      if (num_read_bytes == RECEIVE_PACKET_SIZE) {
        res = cobs_decode(decode, 64, buffer, RECEIVE_PACKET_SIZE);

        if (res.status == COBS_DECODE_OK) {
          s.write(output,4); //send 4 bytes of test data

          int encoder1 = (decode[0] << 24) | (decode[1] << 16) | (decode[2] << 8) | decode[3];
          int encoder2 = (decode[4] << 24) | (decode[5] << 16) | (decode[6] << 8) | decode[7];
          int encoder3 = (decode[8] << 24) | (decode[9] << 16) | (decode[10] << 8) | decode[11];
          int encoder4 = (decode[12] << 24) | (decode[13] << 16) | (decode[14] << 8) | decode[15];

          pros::lcd::print(3, "%7d |%7d |%7d |%7d", encoder1, encoder2, encoder3, encoder4);
          //pros::lcd::print(6, "receive packets: %d", receive_counter);
          receive_counter++;
          loop_counter++;
        }
      }

      //s.write(output,1);
      //pros::lcd::print(7, "transmit packets: %d\n", send_counter);
      send_counter++;

      // Delay to let serial data arrive
      //pros::delay(2);
      if (pros::millis() > (last_time + 2000)) {
        pros::lcd::print(7, "update rate: %.2f Hz\n", loop_counter/2.0);
        loop_counter = 0;
        last_time = pros::millis();
      } else {
        //loop_counter++;
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
	pros::lcd::set_text(0, "Musty Board PROS Test");

	pros::lcd::register_btn1_cb(on_center_button);

	//JS
	test_serial2();
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
void autonomous() {}

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
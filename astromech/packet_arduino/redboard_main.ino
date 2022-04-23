// Main file for redboard firmware

// Include header files
#include "crc_decoder.h"

// Initialize board
void setup()
{
	// Setup Motors
	motor_setup();

	// Set USB
	SerialUSB.begin(38400);
}

// loop
void loop()
{
	// Get USB input
	get_input();
}

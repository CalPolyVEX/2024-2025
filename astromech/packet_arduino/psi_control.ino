#include "psi_control.h"

void sendPSICommand(byte command)
{
    // The Bytes in Binary Coded Decimal
    byte bcd[2] = {0}; // 1 = 10's, 0 = 1's

    // Transform Command into Binary Coded Decimal
    bcd[0] = command % 10; // Get 1's Digit
    command -= bcd[0];     // Remove 1's Digit from Command
    bcd[1] = command / 10; // Get 10's Digit (If Command < 10, This Will be 0)

    // Transform BCD Into ASCII Form
    bcd[0] = 0x30 + bcd[0];
    if (bcd[1] != 0)
      bcd[1] = 0x30 + bcd[1];

    Wire.beginTransmission(PSI_ADDRESS);
    Wire.write('0');    // Writing to All PSI Pros
    Wire.write('T');    // T
    if (bcd[1] != 0)    // Write 10's Digit of Command if Command > 9
      Wire.write(bcd[1]);
    Wire.write(bcd[0]); // Write 1's Digit of Command Allways
    Wire.write('\r');   // End Character for Command
    Wire.endTransmission();
}
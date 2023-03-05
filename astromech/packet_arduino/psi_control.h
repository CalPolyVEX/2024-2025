#ifndef PSI_CONTROL_H
#define PSI_CONTROL_H

#include "Wire.h"

// The I2C Address for Both PSI Pros
#define PSI_ADDRESS 22

void sendPSICommand(byte command);

#endif

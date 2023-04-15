#include "debug_mode.h"

TCA9534 ioex;

void debug_loop() {
    /* main loop for debug mode 
    controlling is facilitated through
    onboard debug buttons DB1-DB3 */
}

uint8_t btn_read(int num) {
    /* reads the value of debug
    button DB<num>*/
    return ioex.input(num);
}
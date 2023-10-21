#include "rc_tsunami.h"

#include "tsunami_control.h"

#define TSUNAMI_SELECT_CHNL 8  // the channel to select which sound to play (SD)
#define TSUNAMI_TRIGGER_CHNL 0 // the channel to trigger playing a sound (SI)
#define TSUNAMI_ALT_CHNL 10    // one way switch to access alternate sounds
#define TSUNAMI_MIN_SELECT 174
#define TSUNAMI_MID_SELECT 995

#define define TSUNAMI_DEBUG

// TSUNAMI_CONTROL_STATE state = WAIT_FOR_BUTTON_DOWN;

void handle_tsunami_audio(uint16_t *channel) {
    static TSUNAMI_CONTROL_STATE state = WAIT_FOR_BUTTON_DOWN;
    static unsigned long button_click_time = 0;
    static unsigned long first_down_up_time = 0;
    static unsigned long wait_state_start_time = 0;

    bool trigger_button_down = channel[TSUNAMI_TRIGGER_CHNL] > 1000;

    switch (state) {
    case WAIT_FOR_BUTTON_DOWN:
        /* code */
        SerialUSB.println("in WAIT_FOR_BUTTON_DOWN state");
        if (trigger_button_down) {
            button_click_time = millis();
            state = WAIT_FOR_BUTTON_UP;
            SerialUSB.println("changing state to WAIT_FOR_BUTTON_UP");
        }
        break;

    case WAIT_FOR_BUTTON_UP:
        if (!trigger_button_down) {
            if (millis() - button_click_time < 1000) {
                first_down_up_time = millis();
                state = WAIT_FOR_DOUBLE_BUTTON_DOWN;
                SerialUSB.println("changing state to WAIT_FOR_DOUBLE_BUTTON_DOWN");
            } else {
                // long press
                handle_long_click_sound(channel);
                SerialUSB.println("changing state to WAIT");
                state = WAIT;
            }
        }
        break;
    case WAIT_FOR_DOUBLE_BUTTON_DOWN:
        if (millis() - first_down_up_time < 500) {
            if (trigger_button_down) {
                // play double click sound
                handle_double_click_sound(channel);
                wait_state_start_time = millis();
                state = WAIT;
                SerialUSB.println("changing state to WAIT");
                break;
            }
        } else {
            // play short click sound
            handle_short_click_sound(channel);
            wait_state_start_time = millis();
            state = WAIT;
        }
        break;

    case WAIT:
        if (millis() - wait_state_start_time > 1000) {
            state = WAIT_FOR_BUTTON_DOWN;
            SerialUSB.println("changing state to WAIT");
        }
        break;

    default:
        break;
    }
}

void handle_long_click_sound(uint16_t *channel) {
    // alt switch flipped
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;

    if (alt_control) {
        // play sound
    } else {
        // play other sound
    }
}
void handle_double_click_sound(uint16_t *channel) {
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;
    stopTracks(); // prevent 2 long songs at the same time

    if (alt_control) {
        // play sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(1, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(2, 10);
        } else {
            playTsunamiSound(3, 10);
        }
    } else {
        // play other sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(7, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(8, 10);
        } else {
            playTsunamiSound(9, 10);
        }
    }
}
void handle_short_click_sound(uint16_t *channel) {
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;

    if (alt_control) {
        // play sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(4, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(5, 10);
        } else {
            playTsunamiSound(6, 10);
        }
    } else {
        // play other sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(10, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(11, 10);
        } else {
            playTsunamiSound(12, 10);
        }
    }
}

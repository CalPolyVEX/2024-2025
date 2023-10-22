#include "rc_tsunami.h"

#include "tsunami_control.h"

/** RC receiver and TSUNAMI */
#define TSUNAMI_SELECT_CHNL 8  // the channel to select which sound to play (SD)
#define TSUNAMI_TRIGGER_CHNL 0 // the channel to trigger playing a sound (SI)
#define TSUNAMI_ALT_CHNL 10    // one way switch to access alternate sounds
#define TSUNAMI_MIN_SELECT 174
#define TSUNAMI_MID_SELECT 995
#define TSUNAMI_VOLUME_CHNL 1   // the channel to change the volume
#define TSUNAMI_VOLUME_RANGE 18 // difference between max and min volumes
#define TSUNAMI_MIN_VOLUME 32   // value subtracted to determine the minimum volume
#define TSUNAMI_MIN_VAL 174
#define TSUNAMI_MAX_VAL 1815
#define TSUNAMI_NUM_SOUNDS 3

/** Durations */
#define LONG_CLICK_DURATION 1000
#define DOUBLE_CLICK_DETECT_DURATION 500

/** Songs */
#define MAIN_THEME 1
#define CANTINA_BAND 2
#define THRONE_ROOM 3
#define DUEL_OF_THE_FATES 7
#define IMPERIAL_MARCH 8
#define MANDALORIAN 9

// #define RC_TSUNAMI_DEBUG

static bool is_playing = false;

/** only prints if RC_TSUNAMI_DEBUG macro defined */
void debug_println(char *s) {
#ifdef RC_TSUNAMI_DEBUG
    SerialUSB.println(s);
#endif
}

void handle_tsunami(uint16_t *channel) {
    handle_tsunami_volume(channel);
    handle_tsunami_audio(channel);
}

void handle_tsunami_volume(uint16_t *channel) {
    static uint16_t current_volume = 0;

    if (abs(current_volume - channel[TSUNAMI_VOLUME_CHNL]) > 5) {
        current_volume = channel[TSUNAMI_VOLUME_CHNL];
        int volume = ((channel[TSUNAMI_VOLUME_CHNL] - 461) / (1068 / TSUNAMI_VOLUME_RANGE)) - TSUNAMI_MIN_VOLUME;
        setTsunamiMasterVolume(volume);
        SerialUSB.print(volume);
        SerialUSB.print("\n");
    }
}

void handle_tsunami_audio(uint16_t *channel) {
    static TSUNAMI_CONTROL_STATE state = WAIT_FOR_BUTTON_DOWN;
    static unsigned long button_click_time = 0;
    static unsigned long first_down_up_time = 0;
    static unsigned long wait_state_start_time = 0;
    static unsigned long second_click_down_time = 0;

    bool trigger_button_down = channel[TSUNAMI_TRIGGER_CHNL] > 1000;

    switch (state) {
    case WAIT_FOR_BUTTON_DOWN:
        /* code */
        if (trigger_button_down) {
            button_click_time = millis();
            state = WAIT_FOR_BUTTON_UP;
            debug_println("changing state to WAIT_FOR_BUTTON_UP");
        }
        break;

    case WAIT_FOR_BUTTON_UP:
        if (!trigger_button_down) {
            first_down_up_time = millis();
            state = WAIT_FOR_DOUBLE_BUTTON_DOWN;
            debug_println("changing state to WAIT_FOR_DOUBLE_BUTTON_DOWN");
        } else if (millis() - button_click_time > LONG_CLICK_DURATION) {
            // long press
            if (is_playing) {
                stopTracks();
            } else {
                handle_long_click_sound(channel);
            }
            debug_println("changing state to WAIT");
            state = WAIT;
        }
        break;

    case WAIT_FOR_DOUBLE_BUTTON_DOWN:
        if (millis() - first_down_up_time < DOUBLE_CLICK_DETECT_DURATION) {
            if (trigger_button_down) {
                // regular double click or double click + hold
                second_click_down_time = millis();
                state = WAIT_FOR_DOUBLE_BUTTON_UP;
                debug_println("changing state to WAIT_FOR_DOUBLE_BUTTON_UP");
            }
        } else {
            // play short click sound
            handle_short_click_sound(channel);
            wait_state_start_time = millis();
            state = WAIT;
        }
        break;

    case WAIT_FOR_DOUBLE_BUTTON_UP:
        if (!trigger_button_down) {
            handle_double_click_sound(channel);
            state = WAIT;
        } else if (millis() - second_click_down_time > LONG_CLICK_DURATION) {
            // long press
            handle_double_click_long_sound(channel);
            state = WAIT;
        }
        break;

    case WAIT:
        if (millis() - wait_state_start_time > LONG_CLICK_DURATION) {
            // only change to the next state if black button is not pressed
            if (!trigger_button_down) {
                state = WAIT_FOR_BUTTON_DOWN;
                debug_println("changing state to WAIT");
            }
        }
        break;

    default:
        break;
    }
}

void handle_long_click_sound(uint16_t *channel) {
    // alt switch flipped
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;
    stopTracks();

    if (alt_control) {
        // play sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(MAIN_THEME, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(CANTINA_BAND, 10);
        } else {
            playTsunamiSound(THRONE_ROOM, 10);
        }
    } else {
        // play other sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(DUEL_OF_THE_FATES, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(IMPERIAL_MARCH, 10);
        } else {
            playTsunamiSound(MANDALORIAN, 10);
        }
    }
}
void handle_double_click_sound(uint16_t *channel) {
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;
    stopTracks(); // prevent 2 long songs at the same time

    if (alt_control) {
        // play sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(MAIN_THEME, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(CANTINA_BAND, 10);
        } else {
            playTsunamiSound(THRONE_ROOM, 10);
        }
    } else {
        // play other sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(DUEL_OF_THE_FATES, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(IMPERIAL_MARCH, 10);
        } else {
            playTsunamiSound(MANDALORIAN, 10);
        }
    }
}

/** sound effects */
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

void handle_double_click_long_sound(uint16_t *channel) {
    bool alt_control = channel[TSUNAMI_ALT_CHNL] < 1000;
    stopTracks(); // prevent 2 long songs at the same time

    if (alt_control) {
        // play sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(MAIN_THEME, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(CANTINA_BAND, 10);
        } else {
            playTsunamiSound(THRONE_ROOM, 10);
        }
    } else {
        // play other sound
        if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MIN_SELECT) {
            playTsunamiSound(DUEL_OF_THE_FATES, 10);
        } else if (channel[TSUNAMI_SELECT_CHNL] == TSUNAMI_MID_SELECT) {
            playTsunamiSound(IMPERIAL_MARCH, 10);
        } else {
            playTsunamiSound(MANDALORIAN, 10);
        }
    }
}
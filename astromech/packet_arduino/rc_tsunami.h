#ifndef RC_TSUNAMI_H_
#define RC_TSUNAMI_H_

typedef enum {
    WAIT_FOR_BUTTON_DOWN,
    WAIT_FOR_BUTTON_UP,
    WAIT_FOR_DOUBLE_BUTTON_DOWN,
    WAIT_FOR_DOUBLE_BUTTON_UP,
    WAIT
} TSUNAMI_CONTROL_STATE;

void handle_tsunami_audio(uint16_t *channel);
void handle_long_click_sound(uint16_t *channel);
void handle_double_click_sound(uint16_t *channel);
void handle_short_click_sound(uint16_t *channel);

#endif

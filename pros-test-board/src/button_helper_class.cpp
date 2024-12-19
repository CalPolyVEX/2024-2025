// button_helper_class.cpp

#include "button_helper_class.h"

bool button_expanded::just_pressed() {
    return this->has_changed() && !this->is_pressed();
}

void button_expanded::update(bool new_val) {
    this->prev_value = this->value;
    this->value = new_val;

    if (this->value && has_changed()) {
        this->toggled = !this->toggled;
    }
}

bool button_expanded::is_pressed() {
    return this->value;
}

bool button_expanded::just_released() {
    return this->has_changed() && this->is_pressed();
}

bool button_expanded::has_changed() {
    return this->value != this->prev_value;
}

bool button_expanded::is_toggled() {
    return this->toggled;
}

// button_helper_class.h

#ifndef BUTTON_HELPER_CLASS_H
#define BUTTON_HELPER_CLASS_H

class button_expanded {
  private:
    bool value = false;
    bool prev_value = false;
    bool toggled = false;
  public:
    // Methods
    bool just_pressed();
    void update(bool new_val);
    bool is_pressed();
    bool just_released();
    bool has_changed();
    bool is_toggled();
};

#endif // BUTTON_HELPER_CLASS_H

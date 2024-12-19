// header file for display.cpp
#include "lemlib/api.hpp" // IWYU pragma: keep

void initialize_screen();

void update_robot_position_on_screen(lemlib::Pose pose);

void print_text_at(int index, const char* text);
//void set_rando_text(char* i);
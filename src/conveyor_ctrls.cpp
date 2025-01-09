// #include "hardware_map.h"
#ifndef HARDWARE_MAP_H
#include "hardware_map.h"
#endif
#include "display.h"

#define IS_USING_COLOR_SENSOR true

void score_with_fish_mech() { fish_mech.move_absolute(230.0, 100); }

// this is a delta movement
void set_conveyor_target_in_inches(float inches, int speed = 300) {
  // create the Ticks/Inch ratio as a variable. this is stated as an expression so that it is easily edited.

  // circumference is pi * diameter, in case that wasn't extemely clear.

  // conveyor.set_zero_position(conveyor.get_position());

  float encoder_ticks_per_inch =
      (1800.0 * (60.0 / 36.0)) / (1.3 * M_PI); //   (( ticks per rotation (blue)  *   (gear ratio = (output/input)))
                                               //   /   (circumference of sprocket) == ticks/inch

  // ^this amounts to 734.561275809

  // move the amount of ticks that was necessitated by the input in inches
  conveyor.move_relative((inches * encoder_ticks_per_inch), speed);
}

void conveyor_deposit_and_intake() {
  // make the roller go fast. that's pretty much all this does lol.

  roller_intake.move_velocity(600);

  // make the conveyor go fast. that's pretty much all this does lol.
  conveyor.move_velocity(600);
}

void move_conveyor_backward() {
  // when L2 pressed in Joseph's code

  // make the roller go fast backward. that's pretty much all this does lol.

  roller_intake.move_velocity(-600);

  // make the conveyor go fast backward. that's pretty much all this does lol.
  conveyor.move_velocity(-600);
}

bool load_for_fish_mech() {
  double thresh = 5.0;

  bool has_set_target = false;
  // get rgb data from opti sensor
  pros::c::optical_rgb_s_t color = conveyor_color_detector.get_rgb();

  // get prox data from opti sensor
  uint8_t prox = conveyor_color_detector.get_proximity();

  // these variables track redness & blueness of the ring
  bool red_ring = false;
  // these variables track redness & blueness of the ring
  bool blue_ring = false;

  print_text_at(4, "fishy fishy fish fish time");

  print_text_at(7, fmt::format("prox is {}", prox).c_str());

  if (IS_USING_COLOR_SENSOR) {
    if (prox <= 120) { //  NO RING  NO RING  NO RING  NO RING  NO RING  NO RING

      // if we see no ring, keep moving

      // speed = 50% of 600. for the conveyor to go slow enough to get a prox read
      // and honestly this is here because it was in Joseph's opcontrol vex block code.
      conveyor.move_velocity(100);

    } else {
      if (!has_set_target) {
        has_set_target = true;
        set_conveyor_target_in_inches(3.2);
      }

      if (std::abs(conveyor.get_target_position() - conveyor.get_position()) >= thresh) {
        // Do nothing, the ring isnt in place yet here.

      } else if (color.red > color.blue) { // THERE IS A RED RING

        // there is a red ring here. do red ring things
        red_ring = true;
      } else if (color.blue > color.red) { // THERE IS A BLUE RING

        // redundant but nobody cares
        // there is a blue ring here. do blue ring things.
        blue_ring = true;
      }
    }

    // print_text_at(6, fmt::format("is red or blue ring: {} | is red: {} | is blue: {}", red_ring || blue_ring,
    // red_ring, blue_ring).c_str());

    if (blue_ring || red_ring) {
      // there is a ring. do ring things

      // float num_rotations = 1.5;
      //  evaluated to 3.676 inches
      // advance the ring to fish mech pos
      print_text_at(4, "done fishin");

      return true;
      // conveyor.brake();
    }
    return false;

  } else {
    throw std::invalid_argument(
        "Why are you not using the color sensor? no other sensor is implemented. please define the macro to true.");
  }
}

void deposit_with_fish_mech() { fish_mech.move_absolute(230.0, 100); }

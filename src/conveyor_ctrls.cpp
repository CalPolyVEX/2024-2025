// #include "hardware_map.h"
#include <cstdlib>
#ifndef HARDWARE_MAP_H
#include "hardware_map.h"
#endif
#include "display.h"

#define IS_USING_COLOR_SENSOR true

bool scoring_opposite = false;

bool has_red_ring() {
  bool has_ring = conveyor_color_detector.get_proximity() > 120;
  bool is_red = conveyor_color_detector.get_rgb().red > conveyor_color_detector.get_rgb().blue &&
                conveyor_color_detector.get_rgb().red > conveyor_color_detector.get_rgb().green;
  if (is_red && has_ring) {
    print_text_at(6, "red ring");
    return true;
  }
  return false;
}

bool has_blue_ring() {
  bool has_ring = conveyor_color_detector.get_proximity() > 120;
  bool is_blue = conveyor_color_detector.get_rgb().blue > conveyor_color_detector.get_rgb().red &&
                 conveyor_color_detector.get_rgb().green > conveyor_color_detector.get_rgb().red;
  if (is_blue && has_ring) {
    print_text_at(6, "blue ring");
    return true;
  }
  return false;
}

void score_with_fish_mech() { fish_mech.move_absolute(230.0, 100); }

// this is a delta movement
void set_conveyor_target_in_inches(float inches, int speed = 300) {
  // create the Ticks/Inch ratio as a variable. this is stated as an expression so that it is easily edited.

  // circumference is pi * diameter, in case that wasn't extemely clear.
  // effectively, circumference is inches per revolution

  float encoder_ticks_per_inch = (1800.0 * (60.0 / 36.0)) / (1.3 * M_PI);
  //   (( ticks per rotation (blue)  *   (gear ratio = (output/input)))   /   (circumference of sprocket) == ticks/inch

  // ^this amounts to 734.561275809

  // move the amount of ticks that was necessitated by the input in inches
  conveyor.move_relative((inches * encoder_ticks_per_inch), speed);
}

void conveyor_deposit_and_intake(int speed = 600) {
  // make the roller go fast. that's pretty much all this does lol.
  roller_intake.move_velocity(speed);

  // make the conveyor go fast. that's pretty much all this does lol.
  conveyor.move_velocity(speed);

  if (has_blue_ring() && alliance_color == true && scoring_opposite == false) {
    rejector.extend();
    pros::delay(500);
    // basically 8.17 inches
  } else {
    rejector.retract();
  }

  if (has_red_ring() && alliance_color == false && scoring_opposite == false) {
    rejector.extend();
    pros::delay(500);
  } else {
    rejector.retract();
  }

  if (conveyor.get_actual_velocity() == 0) {
    pros::delay(200);
    if (conveyor.get_actual_velocity() == 0) { conveyor.move_velocity(-600); }
  }
}

void move_conveyor_backward() { conveyor_deposit_and_intake(-600); }

bool has_set_target = false;

bool load_for_fish_mech() {
  double thresh = 5.0;

  print_text_at(4, "fishy fishy fish fish time");

  if (IS_USING_COLOR_SENSOR) {
    if (!(has_blue_ring() or has_red_ring())) { //  NO RING  NO RING  NO RING  NO RING  NO RING  NO RING

      // if we see no ring, keep moving

      // speed = 50% of 600. for the conveyor to go slow enough to get a prox read
      // and honestly this is here because it was in Joseph's opcontrol vex block code.
      conveyor.move_velocity(300);

    } else {
      if (!has_set_target) {
        has_set_target = true;
        set_conveyor_target_in_inches(3.2);
      }

      if (std::abs(conveyor.get_target_position() - conveyor.get_position()) < thresh) {
        has_set_target = false;
        return true;
      }

      // the ring is in place. stop the conveyor.
      conveyor.move_velocity(0);
      return false;
    }

    return false;

  } else {
    throw std::invalid_argument(
        "Why are you not using the color sensor? no other sensor is implemented. please define the macro to true.");
  }
}

void deposit_with_fish_mech() { fish_mech.move_absolute(230.0, 100); }
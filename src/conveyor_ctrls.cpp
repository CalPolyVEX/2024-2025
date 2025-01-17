// #include "hardware_map.h"
#ifndef HARDWARE_MAP_H
#include "hardware_map.h"
#endif
#include "display.h"

#define IS_USING_COLOR_SENSOR true

bool scoring_opposite = false;

uint8_t prox_thresh = 130;

bool has_red_ring() {
  conveyor_color_detector.set_led_pwm(100);
  pros::delay(2);
  bool has_ring = conveyor_color_detector.get_proximity() > prox_thresh;
  bool is_red = conveyor_color_detector.get_rgb().red > conveyor_color_detector.get_rgb().blue &&
                conveyor_color_detector.get_rgb().red > conveyor_color_detector.get_rgb().green;
  conveyor_color_detector.set_led_pwm(0);
  if (is_red && has_ring) {
    print_text_at(6, "red ring");
    return true;
  }
  return false;
}

bool has_blue_ring() {
  conveyor_color_detector.set_led_pwm(100);
  pros::delay(2);
  bool has_ring = conveyor_color_detector.get_proximity() > prox_thresh;
  bool is_blue = conveyor_color_detector.get_rgb().blue > conveyor_color_detector.get_rgb().red &&
                 conveyor_color_detector.get_rgb().green > conveyor_color_detector.get_rgb().red;
  conveyor_color_detector.set_led_pwm(0);
  if (is_blue && has_ring) {
    print_text_at(6, "blue ring");
    return true;
  }
  return false;
}

void zero_fish_mech() {
  if (fish_mech.get_current_draw() < 1000) {
    fish_mech.move_voltage(-1000);
  } else {
    fish_mech.move_velocity(0);
    fish_mech.tare_position();
  }
}

void score_with_fish_mech() { fish_mech.move_absolute(250.0, 100); }

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
  // color sorting
  bool is_blue_alliance = not alliance_color;
  bool is_red_alliance = alliance_color;

  if ((has_blue_ring() and is_red_alliance) or (has_red_ring() and is_blue_alliance) and not scoring_opposite) {
    rejector.extend();
    pros::delay(100);
    //   basically 8.17/5 inches
  } else {
    rejector.retract();
  }

  if (conveyor.get_actual_velocity() == 0) {
    // JAAAAAAM (maybe)

    pros::delay(200);

    if (conveyor.get_actual_velocity() == 0) {
      // actually jammed
      conveyor.move_velocity(-600);
    }
  }
}

void move_conveyor_backward() { conveyor_deposit_and_intake(-600); }

bool has_set_target = false;

bool fish_mech_is_loaded() {
  double thresh = 5.0;

  print_text_at(4, "fishy fishy fish fish time");

  if (IS_USING_COLOR_SENSOR) {
    if (!(has_blue_ring() or has_red_ring())) { //  NO RING  NO RING  NO RING  NO RING  NO RING  NO RING

      // if we see no ring, keep moving

      // speed = 50% of 600. for the conveyor to go slow enough to get a prox read
      // and honestly this is here because it was in Joseph's opcontrol vex block code.

      // TODO test higher speed
      conveyor.move_velocity(300);

    } else {
      return true;
    }

    return false;

  } else {
    throw std::invalid_argument(
        "Why are you not using the color sensor? no other sensor is implemented. please define the macro to true.");
  }
}

// void deposit_with_fish_mech() { fish_mech.move_absolute(250.0, 100); }
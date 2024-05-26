//In the FastLED library, updated src/platforms/arm/d51/fastpin_arm_d51.h to map the digital pins to the ledstrip pins
//using this line in the Feather M4 Express config:  _FL_DEFPIN( 0, 3, 1); _FL_DEFPIN( 1, 2, 1);

#include <FastLED.h>
#define NUM_LEDS 60
CRGB leds[NUM_LEDS];

void led_setup() { FastLED.addLeds<NEOPIXEL, 0>(leds, NUM_LEDS); }  //add 60 LEDs to pin 0 (LEDSTRIP 1)

void led_show() {
  leds[0] = CRGB::White; FastLED.show(); delay(30);
  leds[0] = CRGB::Black; FastLED.show(); delay(30);
}

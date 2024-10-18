//In the FastLED library, updated src/platforms/arm/d51/fastpin_arm_d51.h to map the digital pins to the ledstrip pins
//using this line in the Feather M4 Express config, replace:
//
//  _FL_DEFPIN( 0, 17, 1); _FL_DEFPIN( 1, 16, 1);
//
//  with:
//
//  _FL_DEFPIN( 0, 3, 1); _FL_DEFPIN( 1, 2, 1);

#include <FastLED.h>
#define NUM_LEDS 10
#define BRIGHTNESS 2
CRGB leds[NUM_LEDS];

void led_setup() {
  FastLED.addLeds<WS2812B, 0, GRB>(leds, NUM_LEDS);  //add 10 LEDs to pin 0 (LEDSTRIP 1)
  FastLED.setBrightness(BRIGHTNESS );
}

void led_show() {
  int delay_time = 30;
  
  for (int i=0; i<10; i++) {
    leds[i] = CRGB::Red; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
  
  for (int i=8; i>0; i--) {
    leds[i] = CRGB::Red; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }

  for (int i=0; i<10; i++) {
    leds[i] = CRGB::Green; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
  
  for (int i=8; i>0; i--) {
    leds[i] = CRGB::Green; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }

  for (int i=0; i<10; i++) {
    leds[i] = CRGB::Blue; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
  
  for (int i=8; i>0; i--) {
    leds[i] = CRGB::Blue; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }

    for (int i=0; i<10; i++) {
    leds[i] = CRGB::Yellow; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
  
  for (int i=8; i>0; i--) {
    leds[i] = CRGB::Yellow; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }

     for (int i=0; i<10; i++) {
    leds[i] = CRGB::White; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
  
  for (int i=8; i>0; i--) {
    leds[i] = CRGB::White; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
}

void led_blip() {
  int delay_time = 40;
  
  for (int i=0; i<1; i++) {
    leds[i] = CRGB::Red; FastLED.show(); delay(delay_time);
    leds[i] = CRGB::Black; FastLED.show(); delay(delay_time);
  }
}

void led_on(int num) {
  if (num == 1) {
    digitalWrite(12, HIGH);    // turn LED1 on 
  } else if (num == 2) {
    digitalWrite(13, HIGH);    // turn LED2 on 
  }
}

void led_off(int num) {
  if (num == 1) {
    digitalWrite(12, LOW);    // turn LED1 off
  } else if (num == 2) {
    digitalWrite(13, LOW);    // turn LED2 off
  }
}

void init_led_timer() {
  // Enable the clock for TC3
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_TC3;  // Enable TC3 peripheral bus
  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;  // Use GCLK0

  // Wait for synchronization
  while (GCLK->PCHCTRL[TC3_GCLK_ID].reg & GCLK_PCHCTRL_CHEN) {}

  // Reset TC3
  TC3->COUNT32.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC3->COUNT32.CTRLA.bit.SWRST) {}  // Wait for reset to complete

  // Set the counter mode to 16-bit (can also be 8-bit or 32-bit)
  TC3->COUNT32.CTRLA.reg |= TC_CTRLA_MODE_COUNT32;  // 16-bit counter
  TC3->COUNT32.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024;  // Set prescaler to 256

  // Set the compare match value
  TC3->COUNT32.CC[0].reg = 10 * (120000000 / 1024) / 1000;  // Set to 1ms intervals at 48MHz clock
  while (TC3->COUNT32.SYNCBUSY.bit.CC0) {}  // Wait for synchronization

  // Enable TC3 interrupt for compare match (OVF interrupt)
  TC3->COUNT32.INTENSET.reg = TC_INTENSET_MC0;  // Enable interrupt on compare match 0
  NVIC_EnableIRQ(TC3_IRQn);  // Enable TC3 interrupt in NVIC

  // Enable the TC3 module
  TC3->COUNT32.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC3->COUNT32.SYNCBUSY.bit.ENABLE) {}  // Wait for synchronization
}

// Interrupt Service Routine (ISR) for TC3
void TC3_Handler(void) {
  static boolean x = false;
  
  if (TC3->COUNT32.INTFLAG.bit.MC0) {
    // Clear the interrupt flag
    TC3->COUNT32.INTFLAG.reg = TC_INTFLAG_MC0;

    // Do something - for example, toggle an LED or set a flag
    if (x == false) {
      //led_on(2);
      x = true;
    } else {
      //led_off(2);
      x = false;
    }

    float counts_per_ms = (120000000/1024.0) / 1000.0;
    TC3->COUNT32.CC[0].reg += 500 * counts_per_ms;  // Set to 1ms intervals at 120MHz clock
    //while (TC3->COUNT32.SYNCBUSY.bit.CC0) {}
  }
}

void beep() {
  //the buzzer is connected to pin A2
  tone(A2, 400);
  delay(100);
  noTone(A2);
}

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

#define melodyPin 3
//Mario main theme melody
int melody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7,
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
  NOTE_G6, 0, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,

  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,

  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0
};
//Mario main them tempo
int tempo[] = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};

void play_mario_theme() {
  // iterate over the notes of the melody:
  Serial.println(" 'Mario Theme'");
  int size = sizeof(melody) / sizeof(int);
  for (int thisNote = 0; thisNote < size; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / tempo[thisNote];

    tone(A2, melody[thisNote]); //play the note
    delay(noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    // stop the tone playing:
    noTone(A2);
  }
}

float mario_full[] = {
659.25,167,
659.25,167,
0,167,
659.25,167,
0,167,
523.25,167,
659.25,333,
783.99,333,
0,333,
392.00,333,
0,333,
523.25,333,
0,167,
392.00,333,
0,167,
329.63,333,
0,167,
440.00,333,
493.88,333,
466.16,167,
440.00,333,
392.00,222,
659.85,222,
783.99,222,
880.00,333,
698.46,167,
783.99,167,
0,167,
659.25,333,
523.25,167,
587.33,167,
493.88,167,
0,167,
523.25,333,
0,167,
392.00,333,
0,167,
329.63,333,
0,167,
440.00,333,
493.88,333,
466.16,167,
440.00,333,
392.00,222,
659.85,222,
783.99,222,
880.00,333,
698.46,167,
783.99,167,
0,167,
659.25,333,
523.25,167,
587.33,167,
493.88,167,
0,167,
0,333,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
0,167,
415.30,167,
440.00,167,
523.25,167,
0,167,
440.00,167,
523.25,167,
493.88,167,
0,333,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
0,167,
1046.50,333,
1046.50,167,
1046.50,333,
392.00,333,
261.63,333,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
 0,167,
415.30,167,
440.00,167,
523.25,167,
0,167,
440.00,167,
523.25,167,
493.88,167,
0,333,
622.25,333,
0,167,
587.33,333,
0,167,
523.25,333,
0,167,
392.00,167,
392.00,333,
261.63,333,
261.33,167,
0,167,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
0,167,
415.30,167,
440.00,167,
523.25,167,
0,167,
440.00,167,
523.25,167,
493.88,167,
0,333,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
0,167,
1046.50,333,
1046.50,167,
1046.50,333,
392.00,333,
261.63,333,
783.99,167,
739.99,167,
698.46,167,
622.25,333,
659.25,167,
 0,167,
415.30,167,
440.00,167,
523.25,167,
0,167,
440.00,167,
523.25,167,
493.88,167,
0,333,
622.25,333,
0,167,
587.33,333,
0,167,
523.25,333,
0,167,
392.00,167,
392.00,333,
261.63,333,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,333,
659.25,167,
523.25,167,
0,167,
440.00,167,
392.00,333,
196.00,333,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,167,
659.25,167,
392.00,333,
0,167,
261.63,167,
196.00,333,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,333,
659.25,167,
523.25,167,
0,167,
440.00,167,
392.00,333,
196.00,333,
659.25,167,
659.25,167,
0,167,
659.25,167,
0,167,
523.25,167,
659.25,333,
783.99,333,
0,333,
392.00,333,
0,333,
523.25,333,
0,167,
392.00,333,
0,167,
329.63,333,
0,167,
440.00,333,
493.88,333,
466.16,167,
440.00,333,
392.00,222,
659.85,222,
783.99,222,
880.00,333,
698.46,167,
783.99,167,
0,167,
659.25,333,
523.25,167,
587.33,167,
493.88,167,
0,167,
523.25,333,
0,167,
392.00,333,
0,167,
329.63,333,
0,167,
440.00,333,
493.88,333,
466.16,167,
440.00,333,
392.00,222,
659.85,222,
783.99,222,
880.00,333,
698.46,167,
783.99,167,
0,167,
659.25,333,
523.25,167,
587.33,167,
493.88,167,
0,167,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,222,
880.00,222,
880.00,222,
880.00,222,
783.99,222,
698.46,222,
659.25,167,
523.25,333,
440.00,167,
392.00,167,
 0,167,
349.23,167,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,167,
698.46,333,
698.46,167,
698.46,222,
659.25,222,
587.33,222,
392.00,167,
329.63,333,
329.63,167,
261.63,333,
0,333,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,222,
880.00,222,
880.00,222,
880.00,222,
783.99,222,
698.46,222,
659.25,167,
523.25,333,
440.00,167,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,167,
698.46,333,
698.46,167,
698.46,222,
659.25,222,
587.33,222,
392.00,167,
329.63,333,
329.63,167,
261.63,333,
0,333,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,333,
659.25,167,
523.25,167,
0,167,
440.00,167,
392.00,333,
196.00,333,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,167,
659.25,167,
0,333,
659.25,167,
783.99,167,
1318.51,167,
1046.50,167,
1174.66,167,
1567.98,167,
523.25,167,
 523.25,167,
0,167,
523.25,167,
0,167,
523.25,167,
587.33,333,
659.25,167,
523.25,167,
0,167,
440.00,167,
392.00,333,
196.00,333,
659.25,167,
659.25,167,
0,167,
659.25,167,
0,167,
523.25,167,
659.25,333,
783.99,333,
0,333,
392.00,333,
0,333,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,222,
880.00,222,
880.00,222,
880.00,222,
783.99,222,
698.46,222,
659.25,167,
523.25,333,
440.00,167,
392.00,167,
0 ,167,
349.23,167,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,167,
698.46,333,
698.46,167,
698.46,222,
659.25,222,
587.33,222,
392.00,167,
329.63,333,
329.63,167,
261.63,333,
0,333,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,222,
880.00,222,
880.00,222,
880.00,222,
783.99,222,
698.46,222,
659.25,167,
523.25,333,
440.00,167,
392.00,167,
 0,167,
349.23,167,
659.25,167,
523.25,333,
392.00,167,
0,333,
415.30,333,
440.00,167,
698.46,333,
698.46,167,
440.00,333,
0,333,
493.88,167,
698.46,333,
698.46,167,
698.46,222,
659.25,222,
587.33,222,
392.00,167,
329.63,333,
329.63,167,
261.63,333,
0,333,
523.25,333,
0,167,
392.00,333,
0,167,
329.63,222,
440.00,222,
493.88,222,
440.00,222,
415.30,222,
466.16,222,
415.30,222,
329.63,666
};


void play_mario_theme2() {
  // iterate over the notes of the melody:
  Serial.println(" 'Mario Theme'");
  int size = sizeof(mario_full) / sizeof(float);
  for (int thisNote = 0; thisNote < size; thisNote += 2) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    tone(A2, mario_full[thisNote]); //play the note
    delay(mario_full[thisNote+1]/2.0);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = mario_full[thisNote+1]/2.0 * 1.30;
    delay(pauseBetweenNotes);

    // stop the tone playing:
    noTone(A2);
  }
}

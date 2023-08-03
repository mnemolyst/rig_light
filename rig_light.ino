#include "FastLED.h"

#define BRIGHTNESS 255
#define N_LEDS 240

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW

#define STEP 4

enum MODE {
  HUE,
  SAT,
  VAL
};

MODE mode = HUE;

CRGB leds[N_LEDS];

int16_t hue = 0;
int16_t sat = 255;
int16_t val = 64;

uint8_t state_a;
uint8_t state_b;
uint8_t state_c;
uint8_t oldstate_a;
bool clk = false;
uint8_t code_p = 0;

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, 9>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setCorrection(TypicalSMD5050);
  for (uint16_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();

  pinMode (ENCODER_OUT_A, INPUT);
  pinMode (ENCODER_OUT_B, INPUT);
  Serial.begin (9600);
  //Read First Position of Channel A
  oldstate_a = digitalRead(ENCODER_OUT_A);
}

void loop() {
  state_a = digitalRead(ENCODER_OUT_A);
  state_b = digitalRead(ENCODER_OUT_B);
  state_c = digitalRead(ENCODER_OUT_C);
  if (state_c == 0) {
    if (!clk) {
      clk = true;
      switch (mode) {
        case HUE:
          mode = SAT;
          break;
        case SAT:
          mode = VAL;
          break;
        case VAL:
          mode = HUE;
          break;
      }
      if (code_p == 0) {
        code_p = 1;
      } else if (code_p == 1) {
        code_p = 2;
      } else if (code_p == 4) {
        code_p = 5;
      } else {
        code_p = 0;
      }
    }
  } else {
    clk = false;
  }
  if (state_a != oldstate_a && state_a == 0) {
    if (state_b != state_a) {
      Serial.println('-');
      if (code_p == 2) {
        code_p = 3;
      } else {
        code_p = 0;
      }
      switch (mode) {
        case HUE:
          hue -= STEP;
          if (hue < 0) {
            hue += 256;
          }
          break;
        case SAT:
          sat -= STEP;
          if (sat < 0) {
            sat = 0;
          }
          break;
        case VAL:
          val -= STEP;
          if (val < 0) {
            val = 0;
          }
          break;
      }
    } else {
      Serial.println('+');
      if (code_p == 3) {
        code_p = 4;
      } else {
        code_p = 0;
      }
      switch (mode) {
        case HUE:
          hue += STEP;
          if (hue > 255) {
            hue -= 256;
          }
          break;
        case SAT:
          sat += STEP;
          if (sat > 255) {
            sat = 255;
          }
          break;
        case VAL:
          val += STEP;
          if (val > 255) {
            val = 255;
          }
          break;
      }
    }
  
    for (uint16_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(hue, sat, val);
    }
    FastLED.show();
  }
  oldstate_a = state_a;
}

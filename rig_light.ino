#include "FastLED.h"

#define BRIGHTNESS 255
#define N_LEDS 240

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 9

#define STEP 4

enum MODE {
  SOLID,
  SCROLL,
  STROBE
};

enum KNOB_CHG {
  HUE,
  SAT,
  VAL,
  SPEED,
  SCALE
};

MODE mode = SOLID;
KNOB_CHG knob_chg = VAL;

CRGB leds[N_LEDS];

int16_t hue = 20;
int16_t sat = 255;
int16_t val = 136;

uint8_t state_a;
uint8_t state_b;
uint8_t state_c;
uint8_t oldstate_a;
bool clk = false;
uint16_t t_m = millis();
uint16_t old_t_m = millis();

float scroll_pos = 0;
float scroll_speed_x = 0.2;
float scroll_speed = pow(scroll_speed_x, 3) * 0.2;
float scroll_scale = pow(1.1, 23);
float scroll_leds_scale = 255 / N_LEDS;

float strobe_speed = 0.002;
float strobe_acc = 0;
boolean strobe_on = false;

void setup() {
  delay(1000);
  random16_set_seed(analogRead(0));

  FastLED.addLeds<NEOPIXEL, LED_OUT>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setCorrection(TypicalSMD5050);
  for (uint8_t i = 0; i < N_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);
  }
  FastLED.show();

  pinMode(ENCODER_OUT_A, INPUT);
  pinMode(ENCODER_OUT_B, INPUT);
  // Read first position of channel A
  oldstate_a = digitalRead(ENCODER_OUT_A);

  Serial.begin(9600);

  t_m = millis();
  old_t_m = t_m;
}

void loop_solid() {
  if (state_c == 0) {
    if (!clk) {
      clk = true;
      switch (knob_chg) {
        case HUE:
          knob_chg = SAT;
          break;
        case SAT:
          knob_chg = VAL;
          break;
        case VAL:
          knob_chg = HUE;
          break;
        default:
          knob_chg = HUE;
          break;
      }
    }
  } else {
    clk = false;
  }
  if (state_a != oldstate_a && state_a == 0) {
    if (state_b != state_a) {
      if (clk) {
        mode = STROBE;
        old_t_m = t_m = millis();
        return;
      }
      switch (knob_chg) {
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
      if (clk) {
        mode = SCROLL;
        knob_chg = SPEED;
        old_t_m = t_m = millis();
        return;
      }
      switch (knob_chg) {
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
  
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(hue, sat, val);
    }
    FastLED.show();
  }
}

void loop_scroll() {
  t_m = millis();
  scroll_pos += (t_m - old_t_m) * scroll_speed;
  old_t_m = t_m;

  if (state_c == 0) {
    if (!clk) {
      clk = true;
      switch (knob_chg) {
        case SPEED:
          knob_chg = SCALE;
          break;
        case SCALE:
          knob_chg = SPEED;
          break;
        default:
          knob_chg = SCALE;
          break;
      }
    }
  } else {
    clk = false;
  }

  for (uint8_t i = 0; i < N_LEDS; i++) {
    uint16_t this_hue = scroll_pos + scroll_scale * i * scroll_leds_scale;
    while (this_hue > 255) {
      this_hue -= 256;
    }
    leds[i] = CHSV(this_hue, sat, val);
  }
  FastLED.show();

  if (state_a != oldstate_a && state_a == 0) {
    if (state_b != state_a) {
      if (clk) {
        mode = SOLID;
        knob_chg = HUE;
        return;
      }
      switch (knob_chg) {
        case SPEED:
          scroll_speed_x -= 0.2;
          scroll_speed = pow(scroll_speed_x, 3) * 0.2;
          break;
        case SCALE:
          scroll_scale /= 1.1;
          break;
      }
    } else {
      if (clk) {
        mode = STROBE;
        old_t_m = t_m = millis();
        return;
      }
      switch (knob_chg) {
        case SPEED:
          scroll_speed_x += 0.2;
          scroll_speed = pow(scroll_speed_x, 3) * 0.2;
          break;
        case SCALE:
          scroll_scale *= 1.1;
          break;
      }
    }
  }
}

void loop_strobe() {
  t_m = millis();
  strobe_acc += (t_m - old_t_m) * strobe_speed;
  old_t_m = t_m;

  if (state_c == 0) {
    if (!clk) {
      clk = true;
    }
  } else {
    clk = false;
  }

  if (state_a != oldstate_a && state_a == 0) {
    if (state_b != state_a) {
      if (clk) {
        mode = SCROLL;
        old_t_m = t_m = millis();
        return;
      }
      strobe_speed /= 1.1;
    } else {
      if (clk) {
        mode = SOLID;
        return;
      }
      strobe_speed *= 1.1;
    }
  }

  if (strobe_on) {
    strobe_on = false;
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  }

  if (strobe_acc >= 1) {
    strobe_on = true;
    strobe_acc = 0;
    for (uint8_t i = 0; i < N_LEDS; i++) {
      leds[i] = CHSV(hue, sat, val);
    }
    FastLED.show();
  }
}

void loop() {
  state_a = digitalRead(ENCODER_OUT_A);
  state_b = digitalRead(ENCODER_OUT_B);
  state_c = digitalRead(ENCODER_OUT_C);

  Serial.println(mode);
  if (mode == SOLID) {
    loop_solid();
  } else if (mode == SCROLL) {
    loop_scroll();
  } else if (mode == STROBE) {
    loop_strobe();
  }
  oldstate_a = state_a;
}

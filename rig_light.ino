#include "FastLED.h"

#define BRIGHTNESS 100
#define N_LEDS 240

#define ENCODER_OUT_A 6 // CLK
#define ENCODER_OUT_B 7 // DT
#define ENCODER_OUT_C 8 // SW
#define LED_OUT 9

#define STEP 4

#define STROBE_NOISE_N 149

enum MODE {
  SOLID,
  SCROLL,
  STROBE,
};

enum KNOB_CHG {
  HUE,
  SAT,
  VAL,
  SPEED,
  SCALE
};

enum STROBE_PHASE {
  C1,
  C12,
  C2,
  C21
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

uint16_t strobe_t_1 = 100;
uint16_t strobe_t_12 = 50;
uint8_t strobe_t_12_8frac = 255 / strobe_t_12;
uint16_t strobe_t_2 = 200;
uint16_t strobe_t_21 = 25;
uint8_t strobe_t_21_8frac = 255 / strobe_t_21;
uint8_t strobe_t_frac;
uint8_t strobe_fade_frac;
float strobe_c1_hue = 255;
float strobe_c1_sat = 0;
float strobe_c1_val = 255;
float strobe_c2_hue = 180;
float strobe_c2_sat = 255;
float strobe_c2_val = 0;
unsigned long strobe_acc = 0;
float strobe_fade = 0;
float strobe_phase_start_t;
STROBE_PHASE strobe_phase = C1;
uint8_t strobe_noise[STROBE_NOISE_N];
uint8_t strobe_noise_idx = 0;
uint8_t get_strobe_noise() {
  while (strobe_noise_idx >= STROBE_NOISE_N) {
    strobe_noise_idx -= STROBE_NOISE_N;
  }
  return strobe_noise[strobe_noise_idx++];
}

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

  for (uint8_t i = 0; i < STROBE_NOISE_N; i++) {
    strobe_noise[i] = random8();
  }
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

  for (uint8_t i = 0; i < N_LEDS; i++) {
    uint16_t this_hue = scroll_pos + scroll_scale * i * scroll_leds_scale;
    while (this_hue > 255) {
      this_hue -= 256;
    }
    leds[i] = CHSV(this_hue, sat, val);
  }
  FastLED.show();
}

void loop_strobe() {
  t_m = millis();
  strobe_acc += t_m - old_t_m;
  old_t_m = t_m;
  uint8_t fade_floor = 3;

  switch (strobe_phase) {
    case C1:
      for (uint8_t i = 0; i < N_LEDS; i++) {
        uint8_t j = get_strobe_noise();
        if (j >= 3) { // 13 ~= 128 / 10
          leds[i] = CHSV(strobe_c1_hue, strobe_c1_sat, strobe_c1_val);
        } else {
          leds[i] = CHSV(strobe_c2_hue, strobe_c2_sat, strobe_c2_val);
        }
      }
      if (strobe_acc >= strobe_t_1) {
        strobe_acc -= strobe_t_1;
        strobe_phase = C12;
      }
      break;
    case C12:
      strobe_t_frac = strobe_acc * 255 / strobe_t_12;
      strobe_fade_frac = blend8(fade_floor, 255 - fade_floor, strobe_t_frac);
      for (uint8_t i = 0; i < N_LEDS; i++) {
        uint8_t j = get_strobe_noise();
        if (j >= strobe_fade_frac) {
          leds[i] = CHSV(strobe_c1_hue, strobe_c1_sat, strobe_c1_val);
        } else {
          leds[i] = CHSV(strobe_c2_hue, strobe_c2_sat, strobe_c2_val);
        }
      }
      if (strobe_acc >= strobe_t_12) {
        strobe_acc -= strobe_t_12;
        strobe_phase = C2;
      }
      break;
    case C2:
      for (uint8_t i = 0; i < N_LEDS; i++) {
        uint8_t j = get_strobe_noise();
        if (j >= fade_floor) {
          leds[i] = CHSV(strobe_c2_hue, strobe_c2_sat, strobe_c2_val);
        } else {
          leds[i] = CHSV(strobe_c1_hue, strobe_c1_sat, strobe_c1_val);
        }
      }
      if (strobe_acc >= strobe_t_2) {
        strobe_acc -= strobe_t_2;
        strobe_phase = C21;
      }
      break;
    case C21:
      strobe_t_frac = strobe_acc * 255 / strobe_t_21;
      strobe_fade_frac = blend8(255 - fade_floor, fade_floor, strobe_t_frac);
      for (uint8_t i = 0; i < N_LEDS; i++) {
        uint8_t j = get_strobe_noise();
        if (j >= strobe_fade_frac) {
          leds[i] = CHSV(strobe_c1_hue, strobe_c1_sat, strobe_c1_val);
        } else {
          leds[i] = CHSV(strobe_c2_hue, strobe_c2_sat, strobe_c2_val);
        }
      }
      if (strobe_acc >= strobe_t_21) {
        strobe_acc -= strobe_t_21;
        strobe_phase = C1;
      }
      break;
  }
  FastLED.show();

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
        knob_chg = SPEED;
        old_t_m = t_m = millis();
        return;
      }
    } else {
      if (clk) {
        mode = SOLID;
        knob_chg = HUE;
        return;
      }
    }
  }
  delay(10);
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

/* 
 * Defines the system settings
 * Copyright (C) 2013  Richard Meadows
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "LPC11xx.h"
#include "audio/wm8737.h"

/**
 * ======== Tuning ========
 */

#define LEFT_TARGET_FREQ	23
#define RIGHT_TARGET_FREQ	23
#define LEFT_TUNED_BIN		7	/* 21kHz - 24kHz */
#define RIGHT_TUNED_BIN		7	/* 21kHz - 24KHz */

/**
 * ======== Gain ========
 */

// -8dB = 0xB3, 0dB = 0xC3, 8db = 0xD3, 16dB = 0xE3, 24dB = 0xF3, 30dB = 0xFF
#define LEFT_PGA_GAIN		0xBB 	/*  -4 dB */
#define RIGHT_PGA_GAIN		0xBB	/*  -4 dB */
#define LEFT_MICBOOST		0	/* +33 dB */
#define RIGHT_MICBOOST		0	/* +33 dB */

/**
 * ======== Tuning ========
 */
uint32_t get_em_record_flags(void) {
  return LEFT_TARGET_FREQ << 26 |
    LEFT_MICBOOST << 24 |
    LEFT_PGA_GAIN << 16 |
    RIGHT_TARGET_FREQ << 10 |
    RIGHT_MICBOOST << 8 |
    RIGHT_PGA_GAIN;
}
uint32_t get_battery_record_flags(void) {
  return 60 << 26;
}
uint32_t get_rssi_record_flags(void) {
  return 61 << 26;
}
uint32_t get_time_jump_record_flags(void) {
  return 62 << 26;
}
uint32_t get_envelope_record_flags(void) {
  return 63 << 26 |
    LEFT_MICBOOST << 24 |
    LEFT_PGA_GAIN << 16 |
    63 << 10 |
    RIGHT_MICBOOST << 8 |
    RIGHT_PGA_GAIN;
}
uint8_t get_left_tuned_bin(void) {
  return LEFT_TUNED_BIN;
}
uint8_t get_right_tuned_bin(void) {
  return RIGHT_TUNED_BIN;
}

/**
 * ======== Gain ========
 */
uint8_t get_left_pga_gain(void) {
  return LEFT_PGA_GAIN;
}
uint8_t get_right_pga_gain(void) {
  return RIGHT_PGA_GAIN;
}
uint8_t get_left_micboost(void) {
  switch (LEFT_MICBOOST) {
    case 0: return PATH_33DB_MICBOOST;
    case 1: return PATH_28DB_MICBOOST;
    case 2: return PATH_18DB_MICBOOST;
    case 3: return PATH_13DB_MICBOOST;
    default: return PATH_33DB_MICBOOST;
  }
}
uint8_t get_right_micboost(void) {
  switch (RIGHT_MICBOOST) {
    case 0: return PATH_33DB_MICBOOST;
    case 1: return PATH_28DB_MICBOOST;
    case 2: return PATH_18DB_MICBOOST;
    case 3: return PATH_13DB_MICBOOST;
    default: return PATH_33DB_MICBOOST;
  }
}

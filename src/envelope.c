/* 
 * Finds the greatest magnitude value in a sample
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

/**
 * Returns the maximum of `current_max` and the maximum magnitude of any
 * of the first 32 data points in `data`
 */
uint16_t get_envelope_32(uint16_t current_max, int16_t data[]) {
  uint8_t i;
  uint16_t mag;

  /* For each data point */
  for (i = 0; i < 32; i++) {
    /* Get the magnitude */
    if (*data < 0) {
      mag = 0 - *data;
    } else {
      mag = *data;
    }

    /* Update the maximum */
    if (mag > current_max) {
      current_max = mag;
    }

    /* Move the pointer along */
    data++;
  }

  return current_max;
}

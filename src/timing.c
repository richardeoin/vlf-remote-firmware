/* 
 * Functions for managing our internal representation of time
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

#include <string.h>
#include "console.h"
#include "timing.h"

struct time_64_t current_time;
struct time_64_t init_time_val;

void time_init(void) {
  current_time.high = 0;
  current_time.low = 0;
  current_time.us = 0;
  current_time.valid = 0;
}
void set_time(struct time_64_t time) {
  /* If we haven't set the time before */
  if (current_time.valid != 0xFF) {
    /* Work out what time we were initialised at */
    init_time_val.high = time.high - current_time.high;
    init_time_val.low = time.low - current_time.low;
    if (init_time_val.low > time.low) { init_time_val.high--; }

    current_time.valid = 0xFF;
  } else { /* If we have set the time before */
    /* Print out the correction we're making to our debug console. */
    console_printf("Time correction: %d\n", time.low - current_time.low);
  }

  /* Actually set the new time */
  current_time.high = time.high;
  current_time.low = time.low;
}
uint8_t is_time_valid(void) {
  return current_time.valid;
}
struct time_64_t get_time(void) {
  return current_time;
}
void increment_us(uint32_t inc) {
  current_time.us += inc;

  while (current_time.us > 1000*1000) {
    current_time.low++; current_time.us -= 1000*1000;

    /* If we must have just wrapped over an epoch */
    if (current_time.low == 0) {
      current_time.high++;
    }
  }
}

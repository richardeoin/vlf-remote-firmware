/* 
 * Communications Routine
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
#include "radio/radio.h"
#include "console.h"
#include "sleeping.h"
#include "timing.h"
#include "upload.h"
#include "pwrmon.h"
#include "debug.h"

/**
 * Defines how many logging intervals elapse between attempts to
 * update the time.
 */
#define TIME_UPDATE_INTERVAL		1

uint16_t time_update_counter = 0xFFFF;

void comms(void) {
  /* Wake up the radio */
  radio_wake();

  /* If we need to try to update the time */
  if (++time_update_counter >= TIME_UPDATE_INTERVAL) {
    time_update_counter = 0;

    /* Get the time */
    radio_transmit((uint8_t*)"T\n\0", 3, BASE_STATION_ADDR, 1);
  }

  upload();

  console_printf("Going to Sleep!\n");

  /* Put the radio back to sleep */
  radio_sleep();
}

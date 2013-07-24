/* 
 * Responds to packets recieved by the radio.
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

#include <stdlib.h>
#include <time.h>
#include "radio/radio.h"
#include "console.h"
#include "mem/invalidate.h"
#include "timing.h"

/**
 * The current time has been received.
 */
static void radio_time_frame(uint8_t* data) {
  struct time_64_t time_now;

  time_now.high = data[8]; time_now.high <<= 8;
  time_now.high |= data[7]; time_now.high <<= 8;
  time_now.high |= data[6]; time_now.high <<= 8;
  time_now.high |= data[5];

  time_now.low = data[4]; time_now.low <<= 8;
  time_now.low |= data[3]; time_now.low <<= 8;
  time_now.low |= data[2]; time_now.low <<= 8;
  time_now.low |= data[1];

  set_time(time_now);
}
/**
 * A checksum for a block of data in memory has been received.
 */
static void radio_checksum_frame(uint8_t* data) {
  uint32_t address; uint32_t checksum;

  address = data[1];
  address |= data[2] << 8;
  address |= data[3] << 16;
  address |= data[4] << 24;

  checksum = data[5];
  checksum |= data[6] << 8;
  checksum |= data[7] << 16;
  checksum |= data[8] << 24;

  console_printf("Got Ack: 0x%08x\n", address);

  /* If this address and checksum match the block at address will be erased */
  check_and_invalidate(address, checksum);
}
/**
 * Called when any data is received.
 */
void radio_rx_callback(uint8_t* data, uint8_t length,
		 uint8_t energy_detect, uint16_t source) {
  UNUSED(energy_detect);
  UNUSED(source);

  switch (data[0]) {
    case 'T': /* Time frame */
      radio_time_frame(data);
      return;
    case 'A': /* Checksum */
      radio_checksum_frame(data);
      return;
    case 'D': /* This is just a response to a debug packet, ignore */
      return;
    default:
      data[length] = 0;
      console_printf("Unknown radio frame '%s' received\n", data); return;
  }
}

/* 
 * Manages uploads
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

#include "upload.h"
#include "radio/rf212.h"
#include "radio/radio.h"
#include "mem/btree.h"
#include "mem/flash.h"
#include "console.h"

enum {
  /**
   * The number of bytes of upload header we put at the start of each frame
   */
  HEADER_SIZE =			5,
  /**
   * The number of records that are uploaded before we give up if the
   * other end hasn't responded.
   */
  UPLOADS_WITHOUT_ACK =		1,
  /**
   * The number of records we can upload in one go
   */
  MAX_UPLOADS_AT_ONCE =		200,
};

uint8_t upload_frame_buffer[HEADER_SIZE + RECORD_SIZE];
uint32_t up_count = 0;

/**
 * Uploads a record from memory.
 */
void do_upload(uint32_t record_addr, uint32_t leaf_addr, uint8_t ack) {
  up_count++;

  /* Set the frame header */
  upload_frame_buffer[0] = 'U';

  /* Set the leaf address */
  upload_frame_buffer[1] = leaf_addr & 0xFF; leaf_addr >>= 8;
  upload_frame_buffer[2] = leaf_addr & 0xFF; leaf_addr >>= 8;
  upload_frame_buffer[3] = leaf_addr & 0xFF; leaf_addr >>= 8;
  upload_frame_buffer[4] = leaf_addr & 0xFF;

  /* Read the record into the frame from memory */
  ReadFlash(record_addr, upload_frame_buffer + HEADER_SIZE, RECORD_SIZE);

  /* Transmit the upload_frame */
  radif_query(upload_frame_buffer, HEADER_SIZE + RECORD_SIZE,
	      BASE_STATION_ADDR, ack, &rf212_radif);
}

/**
 * Carries out a number of uploads.
 */
void upload(void) {
  uint32_t leaf_marker, upload_addr;
  uint8_t records_done_this_upload = 0;

  /* Start at the beginning of the memory space */
  leaf_marker = first_root();

  do {
    /* Get the address of the next readable leaf */
    upload_addr = next_record(&leaf_marker, MEM_VALID, NO_WRAP);

    /* If there's nothing more to read, return */
    if (upload_addr == 0xFFFFFFFF) { return; }

    if (records_done_this_upload < UPLOADS_WITHOUT_ACK) {
      do_upload(upload_addr, leaf_marker, 1);
    } else {
      if (rf212_radif.last_trac_status == TRAC_NO_ACK) { break; }
      do_upload(upload_addr, leaf_marker, 0);
    }
  } while (++records_done_this_upload < MAX_UPLOADS_AT_ONCE);
}

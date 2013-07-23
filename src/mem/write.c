/* 
 * Manages writes to memory
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

#include "mem/btree.h"
#include "mem/flash.h"
#include "mem/checksum.h"
#include "timing.h"
#include "debug.h"

/**
 * Each sample taken is stored in a record that is structured as follows:
 *
 * WORD NR	USE
 * 0:		RECORD_FLAGS
 * 1: 		UNIX_TIME LSB
 * 2: 		UNIX_TIME MSB
 * 3: 		LEFT CHANNEL READING
 * 4:		RIGHT CHANNEL READING
 * 5:		CHECKSUM
 *
 * There are an integer number of records stored in each 64
 * KByte. This value is called RECORDS_PER_PAGE.
 *
 */

/**
 * We use a 32-bit buffer so we can write 32-bit values straight to it.
 */
uint32_t write_block[RECORD_SIZE/4];
/**
 * We store the write_leaf_address for quickly finding empty blocks next time.
 */
uint32_t write_leaf_address;

/**
 * Writes a sample to memory with the specified record_flags.
 * The time_ago parameter is used to specify how many seconds ago the reading is from, which is
 * useful if the reading is averaged over say n seconds then is is from n/2 seconds ago.
 */
void write_sample_to_mem(uint32_t record_flags, uint32_t left_data, uint32_t right_data, uint32_t time_ago) {
  uint32_t record_address;

  /* Get the address of the next writable leaf */
  record_address = next_record(&write_leaf_address, MEM_ERASED, WRAP);

  /* If there's no more writable blocks, return */
  if (record_address == 0xFFFFFFFF) {
    return;
  }

  /* Populate the write_block */
  write_block[0] = record_flags; /* Record flags */

  struct time_64_t time = get_time(); /* Unix time */
  if (time_ago > time.low) { time.high--; }
  time.low -= time_ago; /* Subtract time_ago */
  write_block[1] = time.low;
  write_block[2] = time.high;

  write_block[3] = left_data; /* Data */
  write_block[4] = right_data;

  write_block[5] = calculate_checksum((uint8_t*)write_block); /* Checksum */

  WriteFlashByte(write_leaf_address, 0x52); /* Mark the leaf as valid */
  StartWriteFlash(record_address, (uint8_t*)write_block, RECORD_SIZE); /* Write the record */
}
/**
 * Blocks until any pending flash write completes.
 */
void wait_for_write_complete(void) {
  uint32_t i = 0;

  while (writeflash_active) {
    if (i++ > 100000) { /* Timeout */
      debug_printf("Timeout waiting for write to complete!\n");
      break;
    }
  }
}
/**
 * Init.
 */
void init_write(void) {
  /* Start at the beginning of the memory */
  write_leaf_address = first_root();
}

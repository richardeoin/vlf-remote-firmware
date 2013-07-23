/* 
 * Manages the invalidation of records in memory.
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
#include "mem/checksum.h"
#include "mem/flash.h"
#include "console.h"

uint32_t check_block[RECORD_SIZE/4];

/**
 * Invalidates a leaf so the corresponding record can be erased and
 * then overwritten in the future.
 */
void invalidate(uint32_t leaf_addr) {
  /* If we're not in the root and we're not in the data */
  if ((leaf_addr & 0x0000F000) && (leaf_addr & 0x000F0000) == 0) {
    /* Invalidate the leaf */
    WriteFlashByte(leaf_addr, 0);
  } else {
    console_puts("Warning: Attempt to invalidate something that is not a leaf blocked.");
  }
}
/**
 * Checks if the record corresponding to the leaf address specified
 * matches the checksum given, and if so invalidates the leaf.
 */
void check_and_invalidate(uint32_t leaf_addr, uint32_t radio_checksum) {
  uint32_t checksum;
  uint32_t record_addr = leaf_addr_to_record_addr(leaf_addr);

  /* Read the checksum from this address */
  ReadFlash(record_addr + (RECORD_SIZE - 4), (uint8_t*)&checksum, 4);

  /* Checksum matches, all good */
  if (checksum == radio_checksum) {
    invalidate(leaf_addr);
  } else { /* It doesn't match! */
    /* Read in this block */
    ReadFlash(record_addr, (uint8_t*)check_block, RECORD_SIZE);
    /* If the checksum is wrong, invalidate the block */
    if (evaluate_checksum((uint8_t*)check_block) == CHECKSUM_FAIL) {
      invalidate(leaf_addr);
    }
  }
}

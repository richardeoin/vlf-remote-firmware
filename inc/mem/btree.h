/* 
 * Manages the tree of records stored in flash memory
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

#ifndef BTREE_H
#define BTREE_H

#include "LPC11xx.h"

enum {
  /**
   * This is the number of bytes stored in memory for each
   * reading. 6*4 = 24
   */
  RECORD_SIZE			= 24,
  /**
   * This is the integer number of blocks that fit in a page. 65536/24
   * = 2730.67 => 2730
   */
  MAX_RECORDS_PER_BRANCH	= 2730,
  /**
   * This could be anywhere between 2 and 1024, but keeping it short
   * will probably give the best performance.
   */
  ROOT_SIZE			= 32
};

/**
 * Record status flags
 */
enum {
  MEM_INVALID	=	1,
  MEM_VALID	= 	2,
  MEM_ERASED	= 	4
};

void activate_branch_on_root(uint32_t address);
void deactivate_branch_on_root(uint32_t address);

uint32_t leaf_addr_to_record_addr(uint32_t leaf_addr);
uint32_t first_root(void);

/**
 * Returns the address of the next record following the marker_addr
 * that's in a given state. If there are no leaves that are in the
 * given state, the function returns 0xFFFFFFFF.
 */
uint32_t next_record(uint32_t* leaf_marker_addr, uint8_t state, uint8_t wrap);

#endif /* BTREE_H */

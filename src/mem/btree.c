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

#include "mem/flash.h"
#include "mem/btree.h"

/* -------- BRANCH FUNCTIONS -------- */

/**
 * 4 KB Branches have up to 1024 leaves.
 */

/**
 * Translates a leaf address to its corresponding record address.
 */
uint32_t leaf_addr_to_record_addr(uint32_t leaf_addr) {
  return (leaf_addr & 0xFFF00000) |
    ((leaf_addr & 0x0000F000) << 4) |
    ((leaf_addr & 0x00000FFF) * RECORD_SIZE);
}
/**
 * Returns the status of the leaf at the given address on a branch.
 */
uint8_t get_leaf_status(uint32_t address) {
  switch(ReadFlashByte(address)) {
    case 0x00:
      return MEM_INVALID;
    case 0xFF:
      return MEM_ERASED;
    default:
      return MEM_VALID;
  }
}
/**
 * Erase both the sector containing the given branch and the
 * corresponding page containing the records.
 */
void erase_branch(uint32_t address) {
  address &= 0xFFFFF000;

  deactivate_branch_on_root(address); /* Remove this branch from the root */

  SectorErase(address); /* Erase the sector */

  /* Convert the sector address to the corresponding page address */
  address = leaf_addr_to_record_addr(address);

  WaitForBusyClear(address); /* Wait for the sector erase to finish */

  PageErase(address); /* Erase the corresponding page */
  WaitForBusyClear(address); /* Wait for the page erase to finish */
}
/**
 * Returns the address of the next leaf with the desired state on a
 * given branch. If no leaf has this state, the function will return
 * 0xFFFFFFFF. TODO Replace this function with a single flash read.
 */
uint32_t traverse_current_branch(uint32_t address, uint8_t state) {
  uint16_t i;
  address &= 0xFFF0FFFF; /* Make sure we're looking at the first page of the chip */

  /* For each leaf */
  for (i = (address & 0x00000FFF); i < MAX_RECORDS_PER_BRANCH; i++, address++) {
    /* If it's in the desired state */
    if (get_leaf_status(address) == state) {
      return address; /* Return the index */
    }
  }

  return 0xFFFFFFFF;
}
/**
 * Returns the address of the first leaf with the desired state on a
 * given branch. If no leaf has this state, the function will return
 * 0xFFFFFFFF. TODO Replace this function with a single flash read.
 */
uint32_t traverse_entire_branch(uint32_t address, uint8_t state) {
  uint16_t i;
  uint8_t leaf_status, branch_status = 0;
  address &= 0xFFFFF000;

  for (i = 0; i < MAX_RECORDS_PER_BRANCH; i++, address++) { /* For each leaf */
    leaf_status = get_leaf_status(address);
    branch_status |= leaf_status;
    if (leaf_status == state) { /* If this leaf is in the desired state */
      return address; /* Return the index */
    }
  }

  if (branch_status == MEM_INVALID) { /* If all the leaves are invalid */
    /* Erase the branch */
    erase_branch(address);

    /* Mark the branch as inactive in the root */
    deactivate_branch_on_root(address);

    /* If we were looking for something that was erased */
    if (state == MEM_ERASED) {
      return address & 0xFFFFF000; /* Then the first leaf is now fine */
    }
  } else if (branch_status == MEM_ERASED) { /* If all the leaves are erased */
    /* Mark this branch as inactive in the root */
    deactivate_branch_on_root(address);
  }

  return 0xFFFFFFFF;
}

/* -------- ROOT FUNCTIONS -------- */

/**
 * Returns the address of the first root in the memory space.
 */
uint32_t first_root(void) {
  return NextPage(0xFFFFFFFF);
}
/**
 * Returns the address of the next active branch on the given
 * root. Returns 0xFFFFFFFF if there is no next active branch.
 */
uint32_t next_active_branch(uint16_t root, uint32_t current_branch_address) {
  uint8_t current_branch = (current_branch_address & 0x0000F000) >> 12;

  while (current_branch < 15) {
    if (root & (1 << current_branch++)) {
      return (current_branch_address & 0xFFF00000) | (current_branch << 12);
    }
  }

  return 0xFFFFFFFF;
}
/**
 * Returns the address of the next branch on the given root. Returns
 * 0xFFFFFFFF if there is no next branch.
 */
uint32_t next_branch(uint32_t current_branch_address) {
  uint8_t current_branch = (current_branch_address & 0x0000F000) >> 12;

  if (current_branch++ < 15) {
    return (current_branch_address & 0xFFF00000) | (current_branch << 12);
  }

  return 0xFFFFFFFF;
}
/**
 * Tidies up the root, maintaining the word that lives there.
 */
void tidy_root(uint32_t address, uint16_t current_offset) {
  uint16_t root;
  address &= 0xFFF00000;

  /* Get the current value of the root */
  root = ReadFlashWord(address+current_offset);
  /* Erase the root sector */
  SectorErase(address);
  WaitForBusyClear(address);
  /* Write the root back to the start of the root sector */
  WriteFlashWord(address, root);
}
/**
 * Returns the offset of the root from the beginning of the chip in
 * bytes. If no root can be found, the function will return
 * 0xFFFF. TODO Replace this function with a single flash read.
 *
 * The first word with MSB = 1 is the valid root. TODO Error checking.
 */
uint16_t get_offset_of_root(uint32_t address) {
  uint16_t i;
  address &= 0xFFF00000;

  for (i = 0; i < ROOT_SIZE; i++, address += 2) {
    if (ReadFlashWord(address) & 0x8000) { /* If MSB = 1 */
      if (i == ROOT_SIZE-1) {
	tidy_root(address, i);
	return 0; /* tidy_root restores the root to the start of the root sector */
      }
      return i*2;
    }
  }

  return 0xFFFF;
}
/**
 * Returns the value of the root for a given chip.
 * 1 = Active Branch
 * 0 = Inactive Branch
 */
uint16_t get_root(uint32_t address) {
  uint16_t current_offset;
  address &= 0xFFF00000; /* Move the address to the start of a chip */

  current_offset = get_offset_of_root(address);
  if (current_offset < 0x1000) { /* If we're still within the root area */
    return ReadFlashWord(address+current_offset);
  } else {
    return 0xFFFF; /* Assume all branches are activated */
  }
}
/**
 * Marks the branch in the address has being active in the root.
 */
void activate_branch_on_root(uint32_t address) {
  uint16_t current_offset, root;
  uint8_t branch = (address & 0x0000F000) >> 12;
  uint16_t bit;
  address &= 0xFFF00000;

  if (branch != 0) { /* If we were passed a branch, not a root */
    current_offset = get_offset_of_root(address);

    if (current_offset >= 0x1000) { /* Invalid Offset */
      SectorErase(address); /* Initialise the root */
      WaitForBusyClear(address);
      current_offset = 0;
    }
    root = ReadFlashWord(address+current_offset);

    bit = 1 << (branch-1);

    if ((root & bit) == 0) { /* If branch is currently inactive */
      root |= bit;

      WriteFlashWord(address+current_offset, 0); /* Write the new root back further along */
      WriteFlashWord(address+current_offset+2, root);
    }
  }
}
/**
 * Marks a branch as being inactive in the root.
 */
void deactivate_branch_on_root(uint32_t address) {
  uint16_t current_offset, root;
  uint8_t branch = (address & 0x0000F000) >> 12;
  address &= 0xFFF00000;

  if (branch != 0) { /* If we were passed a branch, not a root */
    current_offset = get_offset_of_root(address);

    if (current_offset >= 0x1000) { /* Invalid Offset */
      SectorErase(address); /* Initialise the root */
      WaitForBusyClear(address);
      current_offset = 0;
    }
    root = ReadFlashWord(address+current_offset);

    root &= ~(1 << (branch-1));

    WriteFlashWord(address+current_offset, root); /* It's ok to write straight back as only ones go to zeros */
  }
}

/* -------- LOCATING LEAVES -------- */

/**
 * Returns the address of the next record following the marker_addr
 * that's in a given state. If there are no leaves that are in the
 * given state, the function returns 0xFFFFFFFF.
 */
uint32_t next_record(uint32_t* leaf_marker_addr, uint8_t state, uint8_t wrap) {
  uint16_t root; /* TODO pass root in args */
  uint32_t new_addr = *leaf_marker_addr + 1;
  uint32_t first_chip = 0xFFFFFFFF;

  /* First search on the current branch */

  if (new_addr & 0x0000F000) { /* If we're on a branch */
    /* Traverse along the current branch */
    new_addr = traverse_current_branch(new_addr, state);

    if (new_addr != 0xFFFFFFFF) { /* If we found a new leaf */
      *leaf_marker_addr = new_addr; /* Set the marker to this new address */
      /* Return the address of the record that corresponds to this */
      return leaf_addr_to_record_addr(*leaf_marker_addr);
    }

    /* Reset new_addr */
    new_addr = *leaf_marker_addr + 1;
  }

  /* Then look on other branches */

  while(1) {
    /* Get the root if the chip we're currently on */
    root = get_root(*leaf_marker_addr);

    while (1) {
      if (state == MEM_ERASED) {
	new_addr = next_branch(*leaf_marker_addr); /* Any branch will do */
      } else {
        /* We only want active branches */
	new_addr = next_active_branch(root, *leaf_marker_addr);
      }

      /* If there are no suitable branches on this chip, break */
      if (new_addr == 0xFFFFFFFF) { break; }

      *leaf_marker_addr = new_addr;

      /* Look for leaves in the correct state */
      new_addr = traverse_entire_branch(*leaf_marker_addr, state);

      if (new_addr != 0xFFFFFFFF) { /* If we found a leaf in the correct state */
	if (state == MEM_ERASED) { /* Activate the current branch if needed */
	  activate_branch_on_root(new_addr);
	}
	*leaf_marker_addr = new_addr; /* Set the marker to this new address */
	/* Return the address of the record that corresponds to this */
	return leaf_addr_to_record_addr(*leaf_marker_addr);
      }
    }

    /* Move to the next chip */
    new_addr = NextChip(*leaf_marker_addr, wrap);

    /* If there are no more chips */
    if (new_addr == 0xFFFFFFFF) {
      break;
    }
    /* If we're back on the chip we started on */
    if (new_addr == first_chip) {
      break;
    }
    /* Set the first chip if we haven't already */
    if (first_chip == 0xFFFFFFFF) {
      first_chip = new_addr;
    }

    *leaf_marker_addr = new_addr;
  }

  return 0xFFFFFFFF;
}

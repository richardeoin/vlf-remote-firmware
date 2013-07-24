/* 
 * Manages the flash memory chips.
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
#include <stdlib.h>
#include "mem/flash.h"
#include "spi.h"
#include "debug.h"

/**
 *  Chip Enable lines are on
 * PIO1[6]
 * PIO1[7]
 * PIO1[8]
 * PIO2[0]
 */

void flash_init(void) {
  uint32_t i;
  writeflash_active = WRITEFLASH_INACTIVE;

  /* Setup Chip Enable lines */
  LPC_GPIO1->DIR |= (1<<6); /* Output */
//LPC_GPIO1->DIR |= (1<<7); /* Output */
  LPC_GPIO1->DIR |= (1<<8); /* Output */
  LPC_GPIO2->DIR |= (1<<0); /* Output */
  /* Deselect all chips */
  for (i = 0; i < 0x100; i++) { ChipSelectFlash(i<<24, FLASH_SSEL_DISABLE); }

  /* Setup Reset Line P0[3], Active Low. Send it low to keep the chips off the bus! */
  LPC_GPIO0->DIR |= (1<<3); /* Output */
  SetFlashReset(0);
}

/* -------- CHIP IDENTIFICATION AND ENUMERATION -------- */

void flash_setup() {
  uint16_t i; uint32_t total_mem = 0;

  SetFlashReset(1); /* Take the chips out of reset */

  for (i = 0; i < 0x100; i ++) {
    struct flashinfo info = ReadChipInfo(i<<24);
    /* Print the size and some debug information */
    total_mem += (flash_sizes[i] = IdentifyChip(info, i));
  }

  debug_printf("Total Memory = %d bytes\n\n", total_mem);
}

uint32_t IdentifyChip(struct flashinfo info, uint16_t num) {
  if (num == 2 || num == 3) { return 0; }
  if (info.man_id == 0 || info.man_id == 0xFF) {
    /* Ignore empty sockets */
  } else {
    debug_printf("Socket %d: ", num);

    if (info.man_id == 0xBF) { // SST
      debug_printf("SST ");
      if (info.dev_id == 0x5) {
	debug_puts("SST25WF080 (8 MBit)");
	return 8 * 0x100000 / 8;
      } else {
	debug_printf("Unknown  ");
      }
    } else {
      debug_printf("Unknown  ");
    }

    debug_printf("JEDEC Manufacturer's ID: %d JEDEC Memory Type: %d JEDEC Memory Size: %d\n",
		 info.jedec_man_id, info.jedec_mem_type, info.jedec_mem_capacity);
  }

  return 0;
}

struct flashinfo ReadChipInfo(uint32_t address) {
  struct flashinfo info;

  ChipSelectFlash(address, FLASH_SSEL_ENABLE);

  WriteCommandAddress(FLASH_READ_ID, 0); spi_write(0); spi_write(0);
  /* Dump the first four bytes received */
  spi_dump_bytes(4);
  /* Read in the device info */
  info.man_id = spi_read();
  info.dev_id = spi_read();

  ChipSelectFlash(address, FLASH_SSEL_DISABLE);
  ChipSelectFlash(address, FLASH_SSEL_ENABLE);

  spi_write(FLASH_JEDEC_ID); spi_write(0); spi_write(0); spi_write(0);
  /* Dump the first byte received */
  spi_dump_bytes(1);
  /* Read in the device info */
  info.jedec_man_id = spi_read();
  info.jedec_mem_type = spi_read();
  info.jedec_mem_capacity = spi_read();

  ChipSelectFlash(address, FLASH_SSEL_DISABLE);

  return info;
}

/* -------- READ AND WRITE -------- */

uint8_t ReadFlashByte(uint32_t address) {
  uint8_t value;

  ChipSelectFlash(address, FLASH_SSEL_ENABLE);

  WriteCommandAddress(FLASH_READ, address); spi_write(0);
  /* Dump the first four bytes received */
  spi_dump_bytes(4);
  /* Read in the data */
  value = spi_read();

  ChipSelectFlash(address, FLASH_SSEL_DISABLE);

  return value;
}
void WriteFlashByte(uint32_t address, uint8_t data) {
  if (WriteUnprotect(address) > 0) { /* If write enable was successful */
    /* Unlock write mode */
    SingleCommand(address, FLASH_WRITE_ENABLE);

    ChipSelectFlash(address, FLASH_SSEL_ENABLE);

    WriteCommandAddress(FLASH_BYTE_WRITE, address); spi_write(data);
    /* Dump the whole response (there wasn't one, the flash output was high impedance) */
    spi_dump_bytes(5);

    ChipSelectFlash(address, FLASH_SSEL_DISABLE);

    /* Wait for the signal that the write has completed */
    WaitForBusyClear(address);
  }
}
/**
 * Returns the bitwise AND of size bytes starting from address
 */
uint8_t ReadFlashAND(uint32_t address, uint32_t size) { /* This will wrap-around */
  if (size > 0) {
    uint32_t index = 0; uint8_t result = 0xFF;
    ChipSelectFlash(address, FLASH_SSEL_ENABLE);

    WriteCommandAddress(FLASH_SPEED_READ, address); spi_write(0); spi_write(0);
    /* Dump the first five bytes received */
    spi_dump_bytes(5);
    /* Read in the data */
    while (index++ < size) {
      /* Put another byte in the TxFIFO if required */
      if (index < size) { spi_write(0); }
      /* Read from the RxFIFO */
      result &= spi_read();
    }

    ChipSelectFlash(address, FLASH_SSEL_DISABLE);
    return result;
  } else {
    return 0;
  }
}
/**
 * Reads a block of memory from flash.
 */
uint8_t ReadFlash(uint32_t address, uint8_t* buffer, uint32_t size) { /* This will wrap-around */
  if (size > 0) {
    uint32_t index = 0;
    ChipSelectFlash(address, FLASH_SSEL_ENABLE);

    WriteCommandAddress(FLASH_SPEED_READ, address); spi_write(0); spi_write(0);
    /* Dump the first five bytes received */
    spi_dump_bytes(5);
    /* Read in the data */
    while (index < size) {
      /* Put another byte in the TxFIFO if required */
      if (index + 1 < size) { spi_write(0); }
      /* Read from the RxFIFO */
      buffer[index++] = spi_read();
    }

    ChipSelectFlash(address, FLASH_SSEL_DISABLE);
    return 1;
  } else {
    return 0;
  }
}

/* -------- WORD READ/WRITE -------- */

/**
 * Writes a 16-bit word to flash.
 */
void WriteFlashWord(uint32_t address, uint16_t word) {
  uint8_t* byte_ptr = (uint8_t*)&word;

  WriteFlashByte(address++, *(byte_ptr++));
  WriteFlashByte(address, *(byte_ptr));
}
/**
 * Reads a 16-bit word from flash.
 */
uint16_t ReadFlashWord(uint32_t address) {
  uint16_t word;

  ReadFlash(address, (uint8_t*)&word, 2); //TODO sizeof(word)

  return word;
}

/* -------- AUTOMATIC WRITE -------- */

void StartWriteFlash(uint32_t address, uint8_t* record, uint32_t len) { /* Async */
  /* Wait for any current write to complete */
  while (writeflash_active);

  if (WriteUnprotect(address) > 0) { /* If write enable was successful */
    /* Unlock write mode */
    SingleCommand(address, FLASH_WRITE_ENABLE);

    /* Write out the address and the first two bytes of the header */
    ChipSelectFlash(address, FLASH_SSEL_ENABLE);
    WriteCommandAddress(FLASH_AUTO_WRITE, address); spi_write(record[0]); spi_write(record[1]);
    spi_dump_bytes(6);
    ChipSelectFlash(address, FLASH_SSEL_DISABLE);

    /* Put all the values into global variables for access during the interrupt */
    writeflash_active = WRITEFLASH_ACTIVE;
    writeflash_address = address;
    writeflash_len = len;
    writeflash_index = 2; /* We've already done two */
    writeflash_record = record;

    /* Configure TMR32B1 to trigger every 25µS */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 10); /* Connect the clock to TMR32B1 */

    LPC_CT32B1->TCR = 0x2; /* Put the counter into reset */
    LPC_CT32B1->PR = 25; /* Trigger after 25µS */
    LPC_CT32B1->MR0 = 48;
    LPC_CT32B1->MCR |= (1<<0)|(1<<2); /* Interrupt and stop on MR0 */
    LPC_CT32B1->IR |= 0x3F; /* Clear all the timer interrupts */

    NVIC_SetPriority(TIMER_32_1_IRQn, 2);
    NVIC_EnableIRQ(TIMER_32_1_IRQn);

    LPC_CT32B1->TCR = 0x1; /* Start the counter */
  }
}
void TIMER32_1_IRQHandler(void) {
  LPC_CT32B1->IR |= 0x3F; /* Clear all the timer interrupts */

  LPC_CT32B1->TCR = 0x2; /* Put the timer into reset */

  /* If there's a buffer being written out at the moment */
  if (writeflash_active == WRITEFLASH_ACTIVE) {
    /* Write out another two bytes from the buffer */
    ChipSelectFlash(writeflash_address, FLASH_SSEL_ENABLE);
    spi_write(FLASH_AUTO_WRITE);
    spi_write(writeflash_record[writeflash_index++]);
    spi_write(writeflash_record[writeflash_index++]);
    spi_dump_bytes(3);
    ChipSelectFlash(writeflash_address, FLASH_SSEL_DISABLE);

    /* If we've reached the end of this record */
    if (writeflash_index >= writeflash_len) {
      /* We've finished writing out. Set the flag */
      writeflash_active = WRITEFLASH_FINISHING; /* Finishing */
    }

    /* Start the timer */
    LPC_CT32B1->TCR = 0x1;
  } else { /* We're Done */
    EndWriteFlash();
    writeflash_active = WRITEFLASH_INACTIVE; /* Inactive */
  }
}
void EndWriteFlash() {
  /* Disable the interrupt */
  NVIC_DisableIRQ(TIMER_32_1_IRQn);

  LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 10); /* Disconnect the clock from TMR32B1 */

  /* Exit the auto write mode */
  SingleCommand(writeflash_address, FLASH_WRITE_DISABLE);
}

/* ---- LOCATION HELPERS ---- */

/**
 * Advances to the next chip.
 * If wrap is 0 and there are no more chips left this function returns 0xFFFFFFFF.
 */
uint32_t NextChip(uint32_t address, uint8_t wrap) {
  uint8_t chip = (address>>24) & 0xFF; /* Work out which chip we're in */

  do {
    address = ((address & 0xFF000000) + 0x01000000); chip++;
    /* If we've wrapped when wrapping is disabled */
    if (chip == 0 && wrap == NO_WRAP) {
      return 0xFFFFFFFF; /* No more chips remaining */
    }
  } while (flash_sizes[chip] == 0);

  return address;
}
uint32_t NextPage(uint32_t address) {
  /* Move along the memory by one page */
  address = (address & 0xFFFF0000) + 0x00010000;

  uint8_t chip = (address>>24) & 0xFF; /* Work out which chip we're in */
  uint32_t index = address & 0xFFFFFF; /* And our position within this chip */

  /* If we've gone over the edge of this chip */
  while (index >= flash_sizes[chip]) {
    /* Go to the next chip or wrap around the chips */
    address = ((address & 0xFF000000) + 0x01000000); index = 0; chip++;
  }

  return address;
}

/* ---- ERASE FUNCTIONS ---- */

/**
 * Erases a 4 KByte sector. Remember this may take up to 30ms.
 */
void SectorErase(uint32_t address) {
  if (WriteUnprotect(address) > 0) { /* If write enable was successful */
    SingleCommand(address, FLASH_WRITE_ENABLE);

    __NOP();

    ChipSelectFlash(address, FLASH_SSEL_ENABLE);
    WriteCommandAddress(FLASH_4KB_ERASE, address); spi_dump_bytes(4);
    ChipSelectFlash(address, FLASH_SSEL_DISABLE);
  }
}
/**
 * Erases a 64 KByte page. Remember this may take up to 30ms.
 */
void PageErase(uint32_t address) {
  if (WriteUnprotect(address) > 0) { /* If write enable was successful */
    SingleCommand(address, FLASH_WRITE_ENABLE);

    __NOP();

    ChipSelectFlash(address, FLASH_SSEL_ENABLE);
    WriteCommandAddress(FLASH_64KB_ERASE, address); spi_dump_bytes(4);
    ChipSelectFlash(address, FLASH_SSEL_DISABLE);
  }
}
/**
 * Erases a whole chip. Remember this may take up to 60ms.
 */
void ChipErase(uint32_t address) {
  if (WriteUnprotect(address) > 0) { /* If write enable was successful */
    SingleCommand(address, FLASH_WRITE_ENABLE);

    __NOP();

    SingleCommand(address, FLASH_CHIP_ERASE);
  }
}

/* ---- STATUS REGISTERS AND WRITE PROTECTION ---- */

void WriteProtect(uint32_t address) {
  WriteStatusRegister(address, 0x1C);
}
uint8_t WriteUnprotect(uint32_t address) {
  if ((ReadFlashStatus(address) & 0x80) == 0) {
    WriteStatusRegister(address, 0); return 0xFF;
  } else {
    debug_puts("Chip is locked from any further writes. Send !WP! high or reset."); return 0;
  }
}
/**
 * Locks the chip from any further writes (until !WP! goes high or a reset)
 */
void WriteLock(uint32_t address) {
  WriteStatusRegister(address, 0x80 | 0x1C);
}

uint8_t ReadFlashStatus(uint32_t address) {
  ChipSelectFlash(address, FLASH_SSEL_ENABLE);
  spi_write(FLASH_READ_STATUS); spi_write(0); spi_dump_bytes(1);
  uint8_t result = spi_read();
  ChipSelectFlash(address, FLASH_SSEL_DISABLE);

  return result;
}
void WriteStatusRegister(uint32_t address, uint8_t status) {
  SingleCommand(address, FLASH_WRITE_ENABLE);

  __NOP();

  ChipSelectFlash(address, FLASH_SSEL_ENABLE);
  spi_write(FLASH_WRITE_STATUS); spi_write(status); spi_dump_bytes(2);
  ChipSelectFlash(address, FLASH_SSEL_DISABLE);
}
void WaitForBusyClear(uint32_t address) {
  uint32_t i = 0;
  while (ReadFlashStatus(address) & 1) { i++; } /* While the busy bit is set */
}

/* ---- HELPER FUNCTIONS */

/**
 * Writes a flash command and a 24 bit address to the SPI TxFIFO
 */
void WriteCommandAddress(uint8_t command, uint32_t address) {
  spi_write(command);
  spi_write((address >> 16) & 0xFF);
  spi_write((address >> 8) & 0xFF);
  spi_write(address & 0xFF);
}
void SingleCommand(uint32_t address, uint8_t command) {
  ChipSelectFlash(address, FLASH_SSEL_ENABLE);

  spi_write(command); spi_dump_bytes(1);

  ChipSelectFlash(address, FLASH_SSEL_DISABLE);
}
/**
 * Sets the state of the active-low flash reset line.
 */
void SetFlashReset(uint8_t state) {
  /* Reduce the input to binary */
  uint8_t value = state > 0 ? 1 : 0;

  /* Flash Reset is on P0[3] */
  LPC_GPIO0->MASKED_ACCESS[1<<3] = (value<<3);
}
/**
 * Changes the state of the chip select line for a given chip.
 * State is active low, set to 0 to communicate with the chip!
 */
void ChipSelectFlash(uint32_t address, uint8_t state) {
  /* Reduce the input to binary */
  uint8_t value = (state == FLASH_SSEL_ENABLE) ? 0 : 1;

  if (state == FLASH_SSEL_ENABLE) {
    /* Use the SPI bus for flash. */
    flash_spi_init();
  }

  /* The top 8 bits of the address are the chip specifier */
  switch (address & 0xFF000000) {
    case 0x00000000:
      //LPC_GPIO1->MASKED_ACCESS[1<<7] = (value<<7);
      break;
    case 0x01000000:
      LPC_GPIO1->MASKED_ACCESS[1<<6] = (value<<6);
      break;
    case 0x02000000:
      LPC_GPIO2->MASKED_ACCESS[1<<0] = (value<<0);
      break;
    case 0x03000000:
      LPC_GPIO1->MASKED_ACCESS[1<<8] = (value<<8);
      break;
    default: break;
  }

  if (state != FLASH_SSEL_ENABLE) {
    /* Revert the SPI bus to working for the radio. */
    radio_spi_init();
  }
}

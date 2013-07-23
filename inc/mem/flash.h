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

#ifndef FLASH_H
#define FLASH_H

#include "LPC11xx.h"

/**
 * The sizes of the memory chips in bytes
 */
uint32_t flash_sizes[0x100];

/**
 * Globals needed for access during interrupt
 */
uint32_t writeflash_address;
uint32_t writeflash_len;
uint32_t writeflash_index;
uint8_t* writeflash_record;
uint8_t writeflash_active; /* 0 = Inactive, 1 = Active, 2 = Finishing */

enum {
  WRITEFLASH_INACTIVE	= 0,
  WRITEFLASH_ACTIVE	= 1,
  WRITEFLASH_FINISHING	= 2,
};

struct flashinfo {
  uint8_t man_id;
  uint8_t dev_id;
  uint8_t jedec_man_id;
  uint8_t jedec_mem_type;
  uint8_t jedec_mem_capacity;
};

/**
 * Flash Chip Registers
 */
enum {
  FLASH_READ			= 0x03, /* Limited to 33MHz clock */
  FLASH_SPEED_READ		= 0x0B,
  FLASH_4KB_ERASE		= 0x20, /* 4K Byte Sector */
  FLASH_32KB_ERASE		= 0x52, /* 32K Byte Block */
  FLASH_64KB_ERASE		= 0xD8, /* 64K Byte Block */
  FLASH_CHIP_ERASE		= 0xC7,
  FLASH_BYTE_WRITE		= 0x02,
  FLASH_AUTO_WRITE		= 0xAD,
  FLASH_READ_STATUS		= 0x05,
  FLASH_WRITE_STATUS		= 0x01,
  FLASH_WRITE_ENABLE		= 0x06,
  FLASH_WRITE_DISABLE		= 0x04,
  FLASH_READ_ID			= 0xAB,
  FLASH_BUSY_ENABLE		= 0x70,
  FLASH_BUSY_DISABLE		= 0x80,
  FLASH_JEDEC_ID		= 0x9F,
  FLASH_ENABLE_HOLD		= 0xAA,
};
/**
 * SSEL
 */
enum {
  FLASH_SSEL_ENABLE	= 0,
  FLASH_SSEL_DISABLE	= 1,
};
/**
 * Wrap
 */
enum {
  WRAP		= 1,
  NO_WRAP	= 0,
};

void flash_init();

/* ---- CHIP IDENTIFICATION AND ENUMERATION ---- */
void flash_setup();
uint32_t IdentifyChip(struct flashinfo info, uint16_t num);
struct flashinfo ReadChipInfo(uint32_t address);

/* ---- READ AND WRITE ---- */
uint8_t ReadFlashByte(uint32_t address);
void WriteFlashByte(uint32_t address, uint8_t data);
uint8_t ReadFlashAND(uint32_t address, uint32_t size);
uint8_t ReadFlash(uint32_t address, uint8_t* buffer, uint32_t size);

/* ---- WORD READ/WRITE ---- */
void WriteFlashWord(uint32_t address, uint16_t word);
uint16_t ReadFlashWord(uint32_t address);

/* ---- AUTOMATIC WRITE ---- */
void StartWriteFlash(uint32_t address, uint8_t* record, uint32_t len);
extern void PIOINT0_IRQHandler(void);
void EndWriteFlash();

/* ---- LOCATION HELPERS ---- */
uint32_t NextChip(uint32_t address, uint8_t wrap);
uint32_t NextPage(uint32_t address);

/* ---- ERASE FUNCTIONS ---- */
void SectorErase(uint32_t address);
void PageErase(uint32_t address);
void ChipErase(uint32_t address);

/* ---- STATUS REGISTERS AND WRITE PROTECTION ---- */
void WriteProtect(uint32_t address);
uint8_t WriteUnprotect(uint32_t address);
void WriteLock(uint32_t address);
uint8_t ReadFlashStatus(uint32_t address);
void WriteStatusRegister(uint32_t address, uint8_t status);
void WaitForBusyClear(uint32_t address);

/* ---- HELPER FUNCTIONS ---- */
void WriteCommandAddress(uint8_t command, uint32_t address);
void SingleCommand(uint32_t address, uint8_t commmand);
void SetFlashReset(uint8_t state);
void ChipSelectFlash(uint32_t address, uint8_t state);

#endif /* FLASH_H */

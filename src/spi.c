/* 
 * SPI0 module driver
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
#include "spi.h"

uint16_t spi_xfer_16(uint16_t data) {
  LPC_SPI0->DR = data;

  /* Wait until the Busy bit is cleared */
  while ((LPC_SPI0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE);

  return LPC_SPI0->DR;
}
uint8_t spi_xfer(uint8_t data) {
  LPC_SPI0->DR = data;

  /* Wait until the Busy bit is cleared */
  while ((LPC_SPI0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE);

  return LPC_SPI0->DR;
}
void spi_write(uint16_t data) {
  /* Wait until there's space in the TxFIFO.
   * The TxFIFO Not Full (TNF) flag goes high when the buffer is not full. */
  while ((LPC_SPI0->SR & SSPSR_TNF) == 0);

  LPC_SPI0->DR = data;
}
uint16_t spi_read(void) {
  /* Wait until there's something in the RxFIFO.
   * The RxFIFO Not Empty flag goes high when there's something in the buffer. */
  while ((LPC_SPI0->SR & SSPSR_RNE) == 0);

  return LPC_SPI0->DR;
}
void spi_dump_bytes(uint32_t dumpCount) {
  while (dumpCount-- > 0) {
    spi_read();		/* Clear the RxFIFO */
  }
}
void spi_flush(void) {
  uint8_t i, Dummy=Dummy;
  for (i = 0; i < FIFOSIZE; i++) {
    Dummy = LPC_SPI0->DR;		/* Clear the RxFIFO */
  }
}

void general_spi_init(void) {
  /* Reset the SSP0 peripheral */
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);

  /* Enable the clock to the module */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);
  LPC_SYSCON->SSP0CLKDIV = 0x01; /* Full 48MHz clock to the module */

  /*  SSP I/O configuration */
  LPC_IOCON->PIO0_8 &= ~0x07;		/* Leave the pull-ups on */
  LPC_IOCON->PIO0_8 |= 0x01;		/* SSP MISO */
  LPC_IOCON->PIO0_9 &= ~0x07;		/* SSP MOSI */
  LPC_IOCON->PIO0_9 |= 0x01;
  LPC_IOCON->SCK_LOC = 0x02; 		/* SSP CLK */
  LPC_IOCON->PIO0_6 &= ~0x07;		/* Locate on P0[6] */
  LPC_IOCON->PIO0_6 |= 0x02;

  /* Device select as master, SSP Enabled */
  LPC_SPI0->CR1 = SSPCR1_MASTER | SSPCR1_SSE;

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  LPC_SPI0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
}
void flash_spi_init(void) {
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);

  /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 0 */
  LPC_SPI0->CR0 = 0x0007;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SPI0->CPSR = 0x2;
  /* This gives a clock rate equal to the AHB Clock (24MHz)*/
}
void radio_spi_init(void) {
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);

  /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 7 */
  LPC_SPI0->CR0 = 0x0707;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SPI0->CPSR = 0x4;
}
void wm8737_spi_init(void) {
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);

  /* Set DSS data to 16-bit, Frame format SPI, CPOL = 1, CPHA = 1, and SCR is 0 */
  LPC_SPI0->CR0 = 0x00CF;
  /* This mode is rising-edge sampled, which is what the ADC wants. */

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SPI0->CPSR = 0x2;
  /* This gives a clock rate of 6MHz! */

  spi_flush();
}
void spi_shutdown(void) {
  /* Cutoff power to the module to save power */
  //LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<11); // TODO: This breaks things!
}

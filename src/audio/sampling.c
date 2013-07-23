/* 
 * Routine to collect a series of datapoints from the ADC.
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
#include "audio/sampling.h"
#include "radio/rf212_functions.h"
#include "audio/wm8737.h"
#include "spi.h"
#include "debug.h"

uint32_t temp;

typedef void (*sampling_func)(void);
void sampling(void);

void prepare_sampling(void) {
  /* Ready the ADC to capture data */
  wm8737_clock_on();
  wm8737_power_on();
}
void do_sampling(void) {
  /* ADC LR Clock on P0[2], rising edge is trigger */
  LPC_IOCON->PIO0_2 &= ~0x07; /* GPIO */
  LPC_GPIO0->MASKED_ACCESS[1<<2] = (0<<2); /* Low to start */
  LPC_GPIO0->DIR |= (1<<2); /* Output */

  /* Setup SPI for the ADC transfer */
  wm8737_spi_init();
  wm8737_spi_on();

  /* We need to use a function pointer to jump our execution to RAM */
  sampling_func sampling_ptr = sampling;
  /* Prepare for the sampling loop */
  sampling_index = 0;

  /**
   *  Disable all interrupts - Be careful that the watchdog
   *  calibration timer doesn't occur during this period
   */
  __disable_irq();

  /* Let RAM accesses settle down before we jump */
  __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();

  /* Actually take the sample */
  sampling_ptr();

  LPC_GPIO0->MASKED_ACCESS[1<<2] = (1<<2); /* P0[2] = ADCLRCLK */

  /* Re-enable all interrupts */
  __enable_irq();

  /* Tidy up from the sampling run */
  spi_flush();
  /* Return the SPI to how it was before */
  wm8737_spi_off();

  /**
   * Revert the SPI bus to working for the radio.
   */
  radio_spi_init();
}
void shutdown_sampling(void) {
  /* Cut the ADC clock and put it into standby */
  wm8737_power_standby();
  wm8737_clock_off();
}

/**
 * Put this function at the beginning of the RAM block
 */
__attribute__ ((long_call, section (".ramfunctions")))
void sampling(void) {
  /**
   * WARNING: The following loop is highly time dependent. DO NOT MODIFY
   * All interrupt handlers must be disabled before this is run.  The
   * core should be running at 250 times the sample rate, and the SPI
   * bus at 62.5 times the sample rate.
   */

  /**
   * This actually takes the sample. It is blocking and will take 
   * 250 * NSAMPLES * (1/CoreClockMHz) µSec.
   */
  /* With NSAMPLES = 32 @ 12MHz this will take 666µS. */

  while (1) {
    /* Put in three 16-bit words into our output buffer */
    LPC_SPI0->DR = 0xAAAA;
    LPC_SPI0->DR = 0xAAAA;
    LPC_SPI0->DR = 0xAAAA;

    /* 12 NOPs to get the rising edge of LR clock aligned with the second word */
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP();

    /* Send the LR clock high as the second word reaches the bus. */
    LPC_GPIO0->MASKED_ACCESS[1<<2] = (1<<2); /* P0[2] = ADCLRCLK */

    /* 7 NOPs to make this whole loop up to 250 instructions */
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();

    LPC_GPIO0->MASKED_ACCESS[1<<2] = (0<<2); /* P0[2] = ADCLRCLK */

    if (++sampling_index == NSAMPLES) { break; }
    /* Use these NOPs instead of the line above for continuous testing */
    /*__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
      __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); */

    /* The first word we read is from before we sent ADCLRCLK high, discard */
    temp = LPC_SPI0->DR;
    samples_left[sampling_index] = LPC_SPI0->DR;
    samples_right[sampling_index] = LPC_SPI0->DR;
  }
}

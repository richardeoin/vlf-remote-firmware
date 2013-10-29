/* 
 * Manages the radio, and the processor's interface to it
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
#include "string.h"
#include "at86rf212.h"
#include "spi.h"

/**
 * Declare a global struct to hold the current state of the radio.
 */
struct radif rf212_radif;

/**
 * The slave select pin   P1[0]
 */
#define RF212_SSEL_PORT		LPC_GPIO1
#define RF212_SSEL_PIN		0
/**
 * The reset pin          P1[2]
 */
#define RF212_RESET_PORT	LPC_GPIO1
#define RF212_RESET_PIN		2
/**
 * The sleep trigger pin  P1[1]
 */
#define RF212_SLPTR_PORT	LPC_GPIO1
#define RF212_SLPTR_PIN		1

/**
 * Slave Select: Active Low
 */
void rf212_spi_enable(void) {
  RF212_SSEL_PORT->MASKED_ACCESS[(1 << RF212_SSEL_PIN)] = 0;
}
void rf212_spi_disable(void) {
  RF212_SSEL_PORT->MASKED_ACCESS[(1 << RF212_SSEL_PIN)] = (1 << RF212_SSEL_PIN);
}
/**
 * Reset: Active Low
 */
void rf212_reset_enable(void) {
  RF212_RESET_PORT->MASKED_ACCESS[(1 << RF212_RESET_PIN)] = 0;
}
void rf212_reset_disable(void) {
  RF212_RESET_PORT->MASKED_ACCESS[(1 << RF212_RESET_PIN)] = (1 << RF212_RESET_PIN);
}
/**
 * Sleep Trigger: Active High
 */
void rf212_slptr_enable(void) {
  RF212_SLPTR_PORT->MASKED_ACCESS[(1 << RF212_SLPTR_PIN)] = (1 << RF212_SLPTR_PIN);
}
void rf212_slptr_disable() {
  RF212_SLPTR_PORT->MASKED_ACCESS[(1 << RF212_SLPTR_PIN)] = 0;
}
/**
 * A function that sets up the I/O required for the radio.
 */
void radio_io_init(void) {
  /* Configure pins as GPIOs */
  LPC_IOCON->R_PIO1_0 &= ~0x7;		/* SSEL   P1[0]		*/
  LPC_IOCON->R_PIO1_0 |= 0x1;
  LPC_IOCON->R_PIO1_1 &= ~0x7;		/* SLP_TR P1[1]		*/
  LPC_IOCON->R_PIO1_1 |= 0x1;
  LPC_IOCON->R_PIO1_2 &= ~0x7;		/* RESET  P1[2]		*/
  LPC_IOCON->R_PIO1_2 |= 0x1;

  /* Configure GPIOs as outputs */
  RF212_SSEL_PORT->DIR |= 1 << RF212_SSEL_PIN;
  RF212_RESET_PORT->DIR |= 1 << RF212_RESET_PIN;
  RF212_SLPTR_PORT->DIR |= 1 << RF212_SLPTR_PIN;

  /* Keep SSEL low */
  rf212_spi_disable();

  /* The interrupt pin is P1[4]  */
  LPC_GPIO1->IS &= ~(1 << 4);		/* Edge sensitive		*/
  LPC_GPIO1->IEV |= (1 << 4);		/* Rising edge 			*/
  LPC_GPIO1->IE |= (1 << 4);		/* Interrupt enabled		*/

  NVIC_SetPriority(PIOINT1_IRQn, 1);	/* 2nd Highest Priority interrupt */
  NVIC_ClearPendingIRQ(PIOINT1_IRQn);
  NVIC_EnableIRQ(PIOINT1_IRQn);
}

uint32_t protect_level = 0;
/**
 * Interrupt Disable.
 */
void rf212_enter_protect(void) {
  NVIC_DisableIRQ(PIOINT1_IRQn);
  protect_level++;
}
/**
 * Interrupt Enable
 */
void rf212_exit_protect(void) {
  if (protect_level > 0) {
    protect_level--;

    if (protect_level == 0) {
      NVIC_EnableIRQ(PIOINT1_IRQn);
    }
  }
}
/**
 * Interrupt Handler
 */
void PIOINT1_IRQHandler(void) {
  /* Clear the interrupt */
  LPC_GPIO1->IC |= (1 << 4);
  
  at86rf212_interrupt(&rf212_radif);
}

/**
 * A function that waits for `us` microseconds before returning.
 * TODO: 
 *   Make this re-entrant, so you can keep on calling the thing
 *   from higher interrput levels.
 */
void radio_delay_us(uint32_t us) {
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 8); /* Connect the clock to TMR16B1 */

  LPC_CT16B1->TCR = 0x2;	/* Put the counter into reset */
  LPC_CT16B1->PR = 12;		/* 1µs on a 12MHz clock */
  LPC_CT16B1->MR0 = us;
  LPC_CT16B1->MCR |= (1<<0)|(1<<2); /* Interrupt and stop on MR0 */
  LPC_CT16B1->IR |= 0x3F;	/* Clear all the timer interrupts */
  LPC_CT16B1->TCR = 0x1;	/* Start the counter */

  while ((LPC_CT16B1->IR & 0x1) == 0);

  LPC_CT16B1->TCR = 0x2;	/* Put the counter back into reset */
  LPC_SYSCON->SYSAHBCLKCTRL &= ~(1 << 8); /* Disconnect the clock from TMR16B1 */
}

/**
 * Initialise the radio with the given receive callback.
 */
void radio_init(rx_callback_func callback) {
  /* Clear the radif */
  memset((void*)&rf212_radif, 0, sizeof(struct radif));

  /* Connect various hardware functions the interface needs to operate */
  rf212_radif.spi_start = rf212_spi_enable;
  rf212_radif.spi_xfer = spi_xfer;
  rf212_radif.spi_stop = rf212_spi_disable;
  rf212_radif.slptr_set = rf212_slptr_enable;
  rf212_radif.slptr_clear = rf212_slptr_disable;
  rf212_radif.reset_set = rf212_reset_enable;
  rf212_radif.reset_clear = rf212_reset_disable;
  rf212_radif.delay_us = radio_delay_us;

  /* Connect the receive callback */
  rf212_radif.rx_callback = callback;

  /* Connect the functions for protected hardware access */
  rf212_radif.enter_protected = rf212_enter_protect;
  rf212_radif.exit_protected = rf212_exit_protect;

  /* Initialise the radio I/O */
  radio_io_init();

  /* Initialise the radio */
  while (at86rf212_reset(&rf212_radif) != RADIO_SUCCESS);

  /* Modulation: 200kbit/s */
  at86rf212_set_modulation(RADIF_OQPSK_400KCHIPS_200KBITS_S, &rf212_radif);
  
  /* Frequency: 868.3 MHz  */
  at86rf212_set_freq(8683, &rf212_radif);
  
  /* Power: +5dBm, EU2 profile */
  at86rf212_set_power(0xe8, &rf212_radif);

  /* PAN ID: 0x1234; Short Address: 0x0002 */
  at86rf212_set_address(0x1234, 0x0002, &rf212_radif);

  /* Make the radio interface operational */
  at86rf212_startup(&rf212_radif);
}
/**
 * Transmits a packet over the radio interface.
 */
void radio_transmit(uint8_t* data, uint8_t length, uint16_t destination,
		    uint8_t ack) {

  at86rf212_tx(data, length, destination, ack, &rf212_radif);
}
/**
 * Sends the radio to sleep.
 */
void radio_sleep(void) {
  at86rf212_sleep(&rf212_radif);
}
/**
 * Wakes the radio up.
 */
void radio_wake(void) {
  at86rf212_wake(&rf212_radif);
}
/**
 * Returns the value of the last TRAC status.
 */
uint16_t radio_get_trac_status(void) {
  return rf212_radif.last_trac_status;
}

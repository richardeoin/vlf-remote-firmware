/* 
 * Functions for driving an AT86RF212.
 * Copyright (C) 2013  Richard Meadows <richard@linux-laptop>
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

#ifndef AT86RF212_H
#define AT86RF212_H

#include "radif.h"

/* -------- Set Radio Properties -------- */
void at86rf212_set_modulation(uint8_t modulation, struct radif* radif);
uint8_t at86rf212_set_freq(uint16_t freq, struct radif* radif);
void at86rf212_set_power(uint8_t power, struct radif* radif);
void at86rf212_set_clkm(uint8_t clkm, struct radif* radif);
void at86rf212_set_address(uint16_t pan_id, uint16_t short_address,
			   struct radif* radif);

/* -------- Wake & Sleep -------- */
void at86rf212_sleep(struct radif* radif);
void at86rf212_wake(struct radif* radif);

/* -------- Misc -------- */
uint8_t at86rf212_get_random(struct radif* radif);
uint8_t at86rf212_measure_energy(struct radif* radif);

/* -------- Reset -------- */
uint8_t at86rf212_reset(struct radif* radif);
void at86rf212_startup(struct radif* radif);

/* -------- Transmit & Receive -------- */
void at86rf212_tx(uint8_t* data, uint8_t length, uint16_t destination,
		  uint8_t ack, struct radif* radif);

/* -------- Interrupt -------- */
void at86rf212_interrupt(struct radif* radif);

#endif /* AT86RF212_H */

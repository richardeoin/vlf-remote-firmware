/* 
 * Manages the system's sequence of sleeping and waking
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

#ifndef SLEEPING_H
#define SLEEPING_H

/**
 * Defines how many WDCLK cycles it takes to read the watchdog timer
 * value register. According to the User Manual (p293) this can be up
 * to 6. 
 *
 * If the clock is running SLOW: DECREASE. If the clock is * running
 * FAST: INCREASE
 */
#define WDT_READ_LATENCY			4
/**
 * Defines how many cycles of the watchdog oscillator are needed to
 * vector the wakeup interrupt and switch back to the main oscillator
 * and start the µS counter.
 */
#define WOSC_WAKE_LATENCY			(48+12)
/**
 * Defines how many microseconds a single clock of the watchdog
 * oscillator during sleep lasts.
 */
#define WOSC_SLEEP_PERIOD			128
/**
 * Defines how many microseconds it takes to switch to the system PLL
 * while running on the slow watchdog oscillator.
 */
#define SYSPLL_SWITCH_LATENCY		200
/**
 * Define how many microseconds it takes to switch to the watchdog
 * oscillator and then __WFI()
 */
#define WOSC_SWITCH_LATNECY			200

typedef void (*logging_func)(void);

/**
 * PDWAKECFG and PDRUNCFG
 */
enum {
  PDCFG_NotUsed =	0xED00,
  IRCOUT_POWERDOWN =	(1 << 0),
  IRCOUT_POWERUP =	0,
  IRCOSC_POWERDOWN =	(1 << 1),
  IRCOSC_POWERUP =	0,
  FLASH_POWERDOWN =	(1 << 2),
  FLASH_POWERUP =	0,
  BOD_POWERDOWN = 	(1 << 3),
  BOD_POWERUP =		0,
  ADC_POWERDOWN =	(1 << 4),
  ADC_POWERUP =		0,
  SYSOSC_POWERDOWN =	(1 << 5),
  SYSOSC_POWERUP =	0,
  WDTOSC_POWERDOWN =	(1 << 6),
  WDTOSC_POWERUP =	0,
  SYSPLL_POWERDOWN =	(1 << 7),
  SYSPLL_POWERUP =	0,
};
/**
 * PDSLEEPCFG
 */
enum {
  PDSLEEPCFG_NotUsed	=	0x18B7,
  PDSLEEPCFG_BOD_On	=	0,
  PDSLEEPCFG_BOD_Off	=	(1 << 3),
  PDSLEEPCFG_WDT_On	=	0,
  PDSLEEPCFG_WDT_Off	=	(1 << 6),
};
/**
 * SCR: Sleep Control Register
 */
enum {
  SCR_SLEEPDEEP		=	(1 << 2),
};
/**
 * WDTOSCCTRL
 */
enum {
  WDTOSCCTRL_Div_2	= 	0,
  WDTOSCCTRL_Div_64	=	0x1F,
  WDTOSCCTRL_500kHz	= 	(1 << 5)
};
/**
 * Our WDT Settings
 */
enum {
  WDT_FAST	= 1,
  WDT_SLOW	= 0
};
/**
 * MAINCLKSEL
 */
enum {
  MAIN_OSC_CLKSEL = 1,
  WATCHDOG_OSC_CLKSEL = 2,
  SYSPLL_CLKSEL = 3
};

void configure_calibration(void);
void start_calibration(void);
void wait_for_calibration(void);

void transition_to_24_mhz(void);
void transition_to_12_mhz(void);

void configure_deep_sleep(void);
void do_deep_sleep(uint32_t sleep_time_halfseconds);

#endif /* SLEEPING_H */

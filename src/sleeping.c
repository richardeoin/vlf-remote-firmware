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

#include <string.h>
#include "LPC11xx.h"
#include "sleeping.h"
#include "console.h"
#include "timing.h"

#undef FAKE_SLEEP

#ifdef FAKE_SLEEP
volatile uint8_t asleep;
#endif

/**
 * Calibration
 *
 * We want to sleep for multiples of 0.5 seconds. If our clock is 7.8125 kHz
 * this is 3906.25 clock cycles.
 *
 * In the calibration run, we're going to see how long it takes to do
 * 3906.25 clock cycles. The watchdog timer that we are going to use
 * to do this has a divide by 4 counter built in, so we expect the output
 * value to be 977.
 *
 * At 250kHz, this is 3906.25 * (1/250000) = 0.015625 seconds = 15.625 ms
 *
 * This is 12000000 * 0.015625 = 1875000 cycles of our 12MHz reference clock.
 *
 * We'll use PC = 4 and MR = 46875;
 */

/**
 * This is the value the watchdog reached during the calibration run.
 * The theoretical value, if the watchdog oscillator was outputting
 * exactly 250kHz, is 1953.125.
 */
volatile uint32_t calibration_value = 1953;

/**
 * This is the value the timer actually reached during the sleep
 * sequence.
 */
uint32_t sleep_sequence_pc;

/**
 * Calibration done flag - 0 when no calibration has been completed or
 * a calibration is in progress.
 */
volatile uint8_t calibration_done = 0;

/**
 * Sets the output frequency (±40%) of the watchdog oscillator.
 * fast_slow = 1: Fast - 250kHz
 * fast_slow = 0: Slow - 7.8125kHz
 */
static void configure_watchdog_freq(uint8_t fast_slow) {
  if (fast_slow) {
    LPC_SYSCON->WDTOSCCTRL = WDTOSCCTRL_500kHz | WDTOSCCTRL_Div_2; // Fast - 250kHz
  } else {
    LPC_SYSCON->WDTOSCCTRL = WDTOSCCTRL_500kHz | WDTOSCCTRL_Div_64; // Slow - 7.8125kHz
  }
}

/* -------- Calibration -------- */

void configure_calibration(void) {
  /* Make sure the watchdog oscillator is powered up and clocked */
  LPC_SYSCON->PDRUNCFG &= ~WDTOSC_POWERDOWN;
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 15); /* Connect the clock to the WDT */

  /* Setup the WDT */
  LPC_SYSCON->WDTCLKDIV = 0x01; /* Watchdog divides its clock source by 1 */
  LPC_SYSCON->WDTCLKSEL = WATCHDOG_OSC_CLKSEL; /* Watchdog runs from watchdog oscillator */
  LPC_SYSCON->WDTCLKUEN = 0;
  LPC_SYSCON->WDTCLKUEN = 1;
  /**
   * Watchdog Timer Constant - This is the value the watchdog starts
   * from every time it's fed.
   */
  LPC_WDT->TC = 0xFFFFFF;
  /* Watchdog configured to run, clock source locked. Timeout does *not* cause chip reset */
  LPC_WDT->MOD = 0x1;

  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 7); /* Connect the clock to TMR16B0 */
  /* Configure TMR16B0 to time the calibration run */
  LPC_CT16B0->TCR = 0x2; /* Disable the timer and put it into reset */
  LPC_CT16B0->PR = 4-1; /* TC is driven at 12MHz / 4 = 3MHz */
  LPC_CT16B0->MR0 = 46875; /* Set the match register for a 15.625ms long timeout */
  LPC_CT16B0->MCR = 0x1; /* Interrupt on MR0 */

  NVIC_SetPriority(TIMER_16_0_IRQn, 0); /* High Priority */
  NVIC_ClearPendingIRQ(TIMER_16_0_IRQn);
  NVIC_EnableIRQ(TIMER_16_0_IRQn); /* Enable the interrupt in the NVIC */
}

/**
 * Starts the watchdog calibration process
 */
void start_calibration(void) {
  /* Make sure the watchdog oscillator is powered up*/
  LPC_SYSCON->PDRUNCFG &= ~WDTOSC_POWERDOWN;
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 15); /* Connect the clock to the WDT */

  /* Clear the flag */
  calibration_done = 0;

  /* Watchdog oscillator running at 250kHz */
  configure_watchdog_freq(WDT_FAST);

  /* Reset + Enable the watchdog */
  LPC_WDT->FEED = 0xAA; LPC_WDT->FEED = 0x55;

  /* Take the timer out of reset and enable it */
  LPC_CT16B0->TCR = 0x1;
}
/**
 * Called when the calibration finishes. Highest priority.
 *
 * NOTE: Do NOT call console, radio or memory from within this function
 */
void TIMER16_0_IRQHandler(void) {
  /* Read the value off the WDT */
  calibration_value = ((~LPC_WDT->TV) & 0xFFFFFF)*4 + WDT_READ_LATENCY;

  /* Clear the interrupt */
  LPC_CT16B0->IR |= (1 << 0);

  /* Disable the timer and put it into reset */
  LPC_CT16B0->TCR = 0x2;

  /* Flag the calibration as done */
  calibration_done = 0xFF;
}
void wait_for_calibration(void) {
  while (calibration_done == 0);
}

/* -------- TMR32B0 -------- */

/**
 * Initialises TMR32B0.
 */
void init_tmr32b0(void) {
  /* Connect the AHB bus clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);
  /* Disable the timer */
  LPC_CT32B0->TCR = 0x2;
}
/**
 * Sets up TMR32B0 as a wakeup timer for the system.
 * The wakeup delay is specified in half seconds.
 */
void setup_tmr32b0_wakeup_timer(uint32_t wakeup_delay_halfseconds) {
  LPC_CT32B0->TCR = 0x2;

  /**
   * Load MR2 with the value from the calibration (multiplied to give
   * the correct sleep time)
   */
  LPC_CT32B0->MR2 = (calibration_value - WOSC_WAKE_LATENCY) * wakeup_delay_halfseconds;
  LPC_CT32B0->PR = 0; /* No prescaler */

  /* Set CT32B0_MAT2 to clear on MR2 match */
  LPC_CT32B0->EMR |= (1 << 8);
  /* Set CT32B0_MAT2 high so a falling edge is generated on match */
  LPC_CT32B0->EMR |= (1 << 2);

  /* Select P0[1] as match output in the IOCONFIG Block */
  LPC_IOCON->PIO0_1 &= ~0x07;
  LPC_IOCON->PIO0_1 |= 0x02; /* Function CT32B0_MAT2 */

  /* Set CT32B0_MAT2 high so a falling edge is generated on match */
  LPC_CT32B0->EMR |= (1 << 2);
}
/**
 * Sets up TMR32B0 as a microsecond timer on a 12MHz clock.
 */
void setup_tmr32b0_12_mhz(void) {
  LPC_CT32B0->TCR = 0x2;

  LPC_CT32B0->TC = 0;
  LPC_CT32B0->MR2 = 0xFFFFFFFF;
  LPC_CT32B0->PR = 12-1; /* Increment at 1MHz on a 12MHz clock */
}
/**
 * Sets up TMR32BO as a microsecond timer on a 48 MHz clock.
 */
void setup_tmr32b0_24_mhz(void) {
  LPC_CT32B0->TCR = 0x2;

  LPC_CT32B0->TC = 0;
  LPC_CT32B0->MR2 = 0xFFFFFFFF;
  LPC_CT32B0->PR = 24-1; /* Increment at 1MHz on a 24MHz clock */
}
/**
 * Starts TMR32B0 running.
 */
void enable_tmr32b0(void) {
  LPC_CT32B0->TCR = 0x1;
}

/* -------- Clocking -------- */

void transition_to_24_mhz(void) {
  /* Prepare to transition to the system PLL */
  LPC_SYSCON->MAINCLKSEL = SYSPLL_CLKSEL;
  LPC_SYSCON->MAINCLKUEN = 0;
  /* Now we wait for the system PLL to stabilise */
  while (!(LPC_SYSCON->SYSPLLSTAT & 0x01));	      /* Wait Until PLL Locked    */

  /* Increment our system time by the value in our microsecond timer */
  increment_us(LPC_CT32B0->TC);

  LPC_SYSCON->MAINCLKUEN = 0;
  LPC_SYSCON->MAINCLKUEN = 1;

  /* Setup and enable the microsecond timer for the new frequency */
  setup_tmr32b0_24_mhz();
  enable_tmr32b0();
}
void transition_to_12_mhz(void) {
  /* Prepare to transition to the main oscillator @ 12MHz */
  LPC_SYSCON->MAINCLKSEL = MAIN_OSC_CLKSEL;
  LPC_SYSCON->MAINCLKUEN = 0;
  LPC_SYSCON->MAINCLKUEN = 1;

  /* Increment our system time by the value in our microsecond timer */
  increment_us(LPC_CT32B0->TC);

  LPC_SYSCON->MAINCLKUEN = 1;

  /* Setup and enable the microsecond timer for the new frequency */
  setup_tmr32b0_12_mhz();
  enable_tmr32b0();
}


/* -------- Deep Sleep -------- */

void configure_deep_sleep(void) {
  /* Set the watchdog oscillator to run in deep-sleep */
  LPC_SYSCON->PDSLEEPCFG = PDSLEEPCFG_NotUsed | PDSLEEPCFG_BOD_Off | PDSLEEPCFG_WDT_On;

  /* Configure the power settings on wakeup */
  /* TODO: For some reason the IRC needs to be on when we wake up from sleep. Work out why */
  LPC_SYSCON->PDAWAKECFG = PDCFG_NotUsed | IRCOUT_POWERUP | IRCOSC_POWERUP |
    FLASH_POWERUP | BOD_POWERDOWN | ADC_POWERDOWN | /* Flash on, BOD and ADC off */
    SYSOSC_POWERUP | WDTOSC_POWERUP | SYSPLL_POWERUP; /* System Osc, WDT Osc and PLL all on */

  /* Initialise our timer */
  init_tmr32b0();

  /* Configure startup edge detection on P0[1] */
  LPC_SYSCON->STARTAPRP0 &= ~(1 << 1); /* Falling edge wakeup */

  /* Reset and disable the start logic */
  LPC_SYSCON->STARTRSRP0CLR |= (1 << 1);
  LPC_SYSCON->STARTERP0 &= ~(1 << 1);

  /* Enable wakeup on P0[1] in the NVIC */
  NVIC_SetPriority(WAKEUP1_IRQn, 3); /* Lowest Priority interrupt */
  NVIC_ClearPendingIRQ(WAKEUP1_IRQn);
  NVIC_EnableIRQ(WAKEUP1_IRQn);

  /* Power Control - Enter Sleep or Deep-Sleep on __WFI() */
  LPC_PMU->PCON &= ~(1 << 1);

  /* System Control Register - Select SLEEPDEEP on __WFI() */
  SCB->SCR = SCR_SLEEPDEEP;
}
void do_deep_sleep(uint32_t sleep_time_halfseconds) {
  /* Make sure the watchdog oscillator is powered up*/
  LPC_SYSCON->PDRUNCFG &= ~WDTOSC_POWERDOWN;

  /* Slow the watchdog oscillator down to 7.8125kHz */
  configure_watchdog_freq(WDT_SLOW);

  /* Increment the microseconds count with the value in TMR32B0 */
  increment_us(LPC_CT32B0->TC + WOSC_SWITCH_LATNECY);

  /* Setup our timer for wakeup */
  setup_tmr32b0_wakeup_timer(sleep_time_halfseconds);

  /* Reset and enable the start logic */
  LPC_SYSCON->STARTRSRP0CLR |= (1 << 1);
  LPC_SYSCON->STARTERP0 |= (1 << 1);

  /* Prepare to switch to the watchdog oscillator as our main clock source */
  LPC_SYSCON->MAINCLKSEL = WATCHDOG_OSC_CLKSEL; /* Switch to watchdog - 7.8125kHz*/
  LPC_SYSCON->MAINCLKUEN = 0;

  /* Enable our wakeup timer */
  enable_tmr32b0();

#ifndef FAKE_SLEEP
  /* Actually switch to the watchdog oscillator */
  LPC_SYSCON->MAINCLKUEN = 1;

  /* Enter Deep Sleep Mode */
  __WFI();
#else
  /* Wait for the sleep flag to clear */
  asleep = 1; while (asleep > 0);
#endif
}

/**
 * This is where we start when we wake up.
 */
void WAKEUP_IRQHandler(void) {
  /* We need to power up the system oscillator */
  LPC_SYSCON->PDRUNCFG &= ~SYSOSC_POWERDOWN;

  /* We're currently running on the WDT clock at about 8kHZ! */
  /* Prepare to transition to the main oscillator @ 12MHz */
  LPC_SYSCON->MAINCLKSEL = MAIN_OSC_CLKSEL;
  LPC_SYSCON->MAINCLKUEN = 0;
  LPC_SYSCON->MAINCLKUEN = 1;	/* And complete the clock change */

  /* Setup and enable our microsecond timer for a 12MHz clock */
  setup_tmr32b0_12_mhz();
  enable_tmr32b0();

  /* Reset and disable the start logic (which clears the interrupt) */
  LPC_SYSCON->STARTRSRP0CLR |= (1 << 1);
  LPC_SYSCON->STARTERP0 &= ~(1 << 1);

#ifdef FAKE_SLEEP
  /* Clear the flag that says we're asleep */
  asleep = 0;
#endif
}

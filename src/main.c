/* 
 * Main application loop for VLF Signal Strength Logger
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

/**
 * Configured Interrupt Priorities:
 *
 * (highest)
 * 
 * 0: TIMER_16_0_IRQn: WDT Oscillator Calibration End. Needs to be on
 * time so that calibration is effective.
 *
 * 1: EINT1_IRQn: Radio Interrupt. Needs to be above other interrupts
 * that use the radio functions so flags can be set and so on.
 *
 * 2: TIMER_32_1_IRQn: Flash write trigger. Allows writes to continue
 * while other processing is ongoing. Prevents EINT1_IRQn from
 * vectoring during part of the handler
 * 2: I2C_IRQn: I2C Communications with WM8737. Isn't using SPI module so
 * can be interrupted by EINT1_IRQn.
 * 2: ADC_IRQn: Picks up the result of the ADC conversion. Not time
 * sensitive.
 *
 * 3: WAKEUP1_IRQn: Timed wake-up from deep sleep
 *
 * (lowest)
 *
 * main: Sends processor to deep-sleep
 *
 */
/**
 * Timers:
 *
 * LPC_CT16B0: Watchdog oscillator calibration
 *
 * LPC_CT16B1: Microsecond delay for radio
 *
 * LPC_CT32B0: Sleep Timer
 *
 * LPC_CT32B1: Waiting Time Byte Program when writing to external
 * memory
 *
 * main: Sends processor to deep-sleep
 *
 */

#include "LPC11xx.h"

#include <string.h>
#include "audio/wm8737.h"
#include "audio/sampling.h"
#include "mem/flash.h"
#include "mem/wipe_mem.h"
#include "mem/write.h"
#include "radio/radio.h"
#include "spi.h"
#include "debug.h"
#include "pwrmon.h"
#include "timing.h"
#include "console.h"
#include "fft.h"
#include "envelope.h"
#include "radio_callback.h"
#include "comms.h"
#include "sleeping.h"
#include "led.h"
#include "settings.h"

/**
 * Function declarations for later.
 */
void infinite_deep_sleep(void);
void do_battery(void);
void do_comms(void);
void do_calibration(void);

/**
 * The entry point to the application.
 */
int main(void) {
  /* Hardware Setup - Don't leave MCLK floating */
  LPC_GPIO0->DIR |= (1 << 1);

  /* Start the SPI Bus first, that's really important */
  general_spi_init();

  /* Power monitoring - Turn off the battery measurement circuit */
  pwrmon_init();

  /* LED */
  led_init();
  led_on();

  /* Initialise the flash memory first so this gets off the SPI bus */
  flash_spi_init();
  flash_init();
  flash_setup();
  spi_shutdown();

  /* Optionally wipe the memory. This may take a few seconds... */
  wipe_mem();

  /* Initialise the memory writing code */
  init_write();

  /* Try to initialise the audio interface */
  if (wm8737_init() < 0) { /* If it fails */
    while (1); /* Wait here forever! */
  }

  /**
   * This delay of approximately 5 seconds is so we can
   * re-program the chip before it goes to sleep
   */
  uint32_t i = 1000*1000*5;
  while (i-- > 0);

  /* Initialise the radio stack */
  radio_init(radio_rx_callback);
  /* Initialise the time */
  time_init();

  /* Sleep forever, let the wakeup loop in sleeping.c handle everything */
  infinite_deep_sleep();

  return 0;
}
/**
 * Our working loop while when running.
 */
void infinite_deep_sleep(void) {
  uint8_t has_logged = 0;
  uint32_t left_em_acc = 0, right_em_acc = 0;
  //uint16_t left_envelope = 0, right_envelope = 0;
  uint32_t acc_counter = 0;

  /* Configure all the calibration stuff first */
  configure_calibration();
  /* Start the first calibration running */
  start_calibration();

  /* Configure all the registers for deep sleep */
  configure_deep_sleep();
  /* Wait for the first calibration to finish */
  wait_for_calibration();

  while (1) {
    /* Sleep for 500 milliseconds */
    do_deep_sleep(1);
    increment_us(500*1000);

    if (is_time_valid()) {
      /* Fire up the ADC */
      prepare_sampling();

      /* If we've taken at least one reading before */
      if (has_logged > 1) {
	/**
	 * Update the envelope values. NOTE: This must be done before
	 * the fft as the fft is in-place.
	 */
//	left_envelope = get_envelope_32(left_envelope, samples_left+4);
//	right_envelope = get_envelope_32(right_envelope, samples_right+4);

	/**
	 * Add our samples to the accumulators. We skip the first 4 points of each sample.
	 * TODO 48MHz clock?
	 */
	left_em_acc += fft_32(samples_left+2, get_left_tuned_bin()) >> 7;
	right_em_acc += fft_32(samples_right+2, get_right_tuned_bin()) >> 7;

	if (++acc_counter >= 128) { /* If we're ready to write to memory */
	  /* Write em to memory */
	  /* Middle of average is 32 seconds ago */
	  write_sample_to_mem(get_em_record_flags(), left_em_acc, right_em_acc, 32);
	  /* Clear accumulators */
	  acc_counter = left_em_acc = right_em_acc = 0;
	  /* Wait for the write to finish */
	  wait_for_write_complete();

	  /* Write envelope to memory */
//	  write_sample_to_mem(get_envelope_record_flags(),
//			      left_envelope, right_envelope, 32);
	  /* Clear envelope */
//	  left_envelope = right_envelope = 0;
	  /* Wait for the write to finish */
//	  wait_for_write_complete();
	}
      } else {
	has_logged++;
	led_off();
      }

      /* Take a reading */
      do_sampling();
      /* Shutdown the ADC */
      shutdown_sampling();

      /* Take battery readings */
      do_battery();
    } else { /* Invalid time */
      led_toggle();
    }

    /* Other tasks */
    do_comms();
    do_calibration();
  }
}
/**
 * Periodically records the battery voltage.
 */
uint16_t battery_counter = 0xFFFE;
uint16_t battery_acc = 0, battery_acc_counter = 0;

uint8_t battery_reading_flag = 0;

void battery_callback(uint16_t adc_value) {
  /* Add this value to the accumulator */
  battery_acc += adc_value;
  battery_acc_counter++;
  battery_reading_flag = 0;
}
void do_battery(void) {
  /* Save the summed reading */
  if (battery_acc_counter >= 10) { /* Every 600 seconds (10 minutes) */
    /* Write to memory */
    write_sample_to_mem(get_battery_record_flags(), (uint32_t)battery_acc, 0, 300); /* Middle of average is 5 mins ago */
    /* Clear the accumulator */
    battery_acc = 0; battery_acc_counter = 0;
  }
  /* Take an individual reading */
  if (++battery_counter >= 120) { /* Every 60 seconds */
    battery_counter = 0;
    /* Get the battery voltage */
    battery_reading_flag = 1;
    pwrmon_start(battery_callback);
    while(battery_reading_flag == 1);
  }
}
/**
 * Periodically communicates with the gateway.
 */
uint16_t comms_counter = 0xFFFE;
void do_comms(void) {
  if (++comms_counter >= 90) { /* Every 45 seconds */
    comms_counter = 0;
    /* Change the clock to 24MHz */
    transition_to_24_mhz();
    /* Do our communications operations */
    comms();
    /* Change the clock back to 12MHz */
    transition_to_12_mhz();
  }
}
/**
 * Periodically calibrates the watchdog oscillator.
 */
uint16_t calibration_counter = 0xFFFF;
void do_calibration(void) {
  if (++calibration_counter >= 40) { /* Every 20 seconds */
    calibration_counter = 0;
    /* Start the calibration */
    start_calibration();
    /* Wait for our calibration run to finish. */
    wait_for_calibration();
  }
}

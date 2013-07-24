/* 
 * Defines a radio interface structure. This provides a standard way
 * for the code that controls a radio to interact with the hardware.
 * Copyright (C) 2013 Richard Meadows
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

#ifndef RADIF_H
#define RADIF_H

#include <stdint.h>

/**
 * Usefulness for unused variables.
 */
#define UNUSED(var) do { (void)var; } while (0)

/**
 * Function type definitions for the hardware functions we need.
 */
typedef void (*pin_set_func) (void);
typedef uint8_t (*spi_xfer_func) (uint8_t);
typedef void (*delay_us_func) (uint32_t);
/**
 * Function type definition for the received callback.
 */
typedef void (*rx_callback_func) (uint8_t* data, uint8_t length,
				  uint8_t energy_detect, uint16_t source);
/**
 * Function type definitions for the protected regions.
 */
typedef void (*protect_func) (void);

/**
 * Represents an interface to a radio.
 */
struct radif {
  /* ---- The callback function for when data is received ---- */
  rx_callback_func rx_callback;

  /* ---- Pointers to the hardware functions we need for the radio
     interface ---- */
  pin_set_func spi_start;
  pin_set_func spi_stop;
  spi_xfer_func spi_xfer;
  pin_set_func slptr_set;
  pin_set_func slptr_clear;
  pin_set_func reset_set;
  pin_set_func reset_clear;
  delay_us_func delay_us;

  /* ---- Pointers to the protection functions that allow us exclusive
     use of the hardware ---- */
  protect_func enter_protected;
  protect_func exit_protected;

  /* ---- Statistics ---- */
  uint16_t rx_success_count;
  uint16_t rx_overflow;

  uint16_t tx_success_count;
  uint16_t tx_channel_fail;
  uint16_t tx_noack;
  uint16_t tx_invalid;

  uint16_t last_trac_status;
};

/**
 * Radio Modulation Modes
 */
enum {
  RADIF_1000KBITS_S_SCRAMBLER		= 0x20,
  RADIF_1000KCHIPS_SIN			= 0x10,
  RADIF_1000KCHIPS_RC			= 0,
  RADIF_OQPSK_1000KCHIPS_1000KBITS_S	= 0x0E,
  RADIF_OQPSK_1000KCHIPS_500KBITS_S	= 0x0D,
  RADIF_OQPSK_1000KCHIPS_250KBITS_S	= 0x0C,
  RADIF_OQPSK_400KCHIPS_400KBITS_S	= 0x0A,
  RADIF_OQPSK_400KCHIPS_200KBITS_S	= 0x09,
  RADIF_OQPSK_400KCHIPS_100KBITS_S	= 0x08,
  RADIF_BPSK_600KCHIPS_40KBITS_S	= 0x4,
  RADIF_BPSK_300KCHIPS_20KBITS_S	= 0,
  RADIF_OQPSK				= 0x08,
  RADIF_BPSK				= 0,
};
/**
 * Radio Clock Output Modes.
 */
enum {
  CLKM_1MHz		= 0x01,
  CLKM_2MHz		= 0x02,
  CLKM_4MHz		= 0x03,
  CLKM_8MHz		= 0x04,
  CLKM_16MHz		= 0x05,
  CLKM_250KHz		= 0x06,
  CLKM_SYMBOL_RATE	= 0x07,
  CLKM_DRIVE_2mA	= 0x00,
  CLKM_DRIVE_4mA	= 0x10,
  CLKM_DRIVE_6mA	= 0x20,
  CLKM_DRIVE_8mA	= 0x30,
};
/**
 * Radio statuses
 */
enum {
  RADIO_SUCCESS = 0x40,		/* The requested service was performed successfully. */
  RADIO_UNSUPPORTED_DEVICE,	/* The connected device is not an Atmel AT86RF212. */
  RADIO_INVALID_ARGUMENT,	/* One or more of the supplied function args are invalid. */
  RADIO_TIMED_OUT,		/* The requested service timed out. */
  RADIO_WRONG_STATE,		/* The end-user tried to do an invalid state transition. */
  RADIO_BUSY_STATE,		/* The radio transceiver is busy receiving or transmitting. */
  RADIO_STATE_TRANSITION_FAILED,/* The requested state transition could not be completed. */
  RADIO_CCA_IDLE,		/* Channel is clear, available to transmit a new frame. */
  RADIO_CCA_BUSY,		/* Channel busy. */
  RADIO_TRX_BUSY,		/* Transceiver is busy receiving or transmitting data. */
  RADIO_BAT_LOW,		/* Measured battery voltage is lower than voltage threshold. */
  RADIO_BAT_OK,			/* Measured battery voltage is above the voltage threshold. */
  RADIO_CRC_FAILED,		/* The CRC failed for the actual frame. */
  RADIO_CHANNEL_ACCESS_FAILURE,	/* The channel access failed during the auto mode. */
  RADIO_NO_ACK,			/* No acknowledge frame was received. */
};
/**
 * TRAC statuses
 */
enum {
  TRAC_SUCCESS               = 0,
  TRAC_SUCCESS_DATA_PENDING  = 1,
  TRAC_WAIT_FOR_ACK          = 2,
  TRAC_CHANNEL_ACCESS_FAIL   = 3,
  TRAC_NO_ACK                = 5,
  TRAC_INVALID               = 7
};

#endif /* RADIF_H */

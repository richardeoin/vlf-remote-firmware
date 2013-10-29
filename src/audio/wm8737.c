/* 
 * Manages the WM8737 Analogue-to-Digital Converter (ADC).
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
#include "audio/wm8737.h"
#include "audio/i2c.h"
#include "settings.h"
#include "debug.h"

/**
 * VMID Enable: P0[7]
 */
#define VMID_INIT()	do { LPC_GPIO0->DIR |= (1<<7); LPC_GPIO0->MASKED_ACCESS[1<<7] = 0; } while (0);
#define VMID_ON()	LPC_GPIO0->MASKED_ACCESS[1<<7] = (1<<7);
#define VMID_OFF()	LPC_GPIO0->MASKED_ACCESS[1<<7] = 0;

/**
 * See the part datasheet at:
 * http://www.wolfsonmicro.com/documents/uploads/data_sheets/en/WM8737.pdf, Revision 4.3
 */

int32_t wm8737_init(void) {
  InitI2C();

  if (PingI2C() > 0) {
    WriteI2C(WM_RESET); /* Completely reset the ADC */
    wm8737_spi_off(); /* Stop the ADC clogging up the SPI bus */
  } else {
    debug_puts("Can't initialise the WM8737 data bus!");
    return -1;
  }

  /* ---- ADC Setup ---- */

  /**
   * Bypass the 3D enhancement filter so we can hit 96kHz sample rate.
   * See Page 21 of the datasheet for reference.
   */
  WriteI2C(WM_ALC1);
  WriteI2C(WM_ALC3);
  WriteI2C(WM_3D_ENHANCE);
  /* More magic to support 96kHz */
  WriteI2C(WM_ALC2 | 0x1C0); /* 1_110x_xxxx */
  /* Totally unsupported register here. I wonder what other magical things are up there.. */
  WriteI2C((0x1C << 9) | 0x4);

  /* fs = 96kHz from MCLK = 12MHz */
  WriteI2C(WM_CLOCKING | CLOCKING_USB | CLOCKING_SR_USB_96000HZ);

  /* Set our inputs to be LINPUT1 and RINPUT1, MICBOOST enabled */
  WriteI2C(WM_LEFT_PATH | PATH_INPUT1 | PATH_MICBOOST_ENABLE | get_left_micboost());
  WriteI2C(WM_RIGHT_PATH | PATH_INPUT1 | PATH_MICBOOST_ENABLE | get_right_micboost());

  /**
   * Preamplifer Bias. Lower values reduce current consumption,
   * PREAMP_BIAS25 uses about 1-2mA less than PREAMP_BIAS100.
   *
   * However, noise increases significantly, so it's best to use
   * PREAMP_BIAS100.
   */
  WriteI2C(WM_PREAMP_CTRL | PREAMP_BIAS100);

  /* Set the PGA Gain */
  WriteI2C(WM_LEFT_PGA | get_left_pga_gain() | PGA_UPDATE);
  WriteI2C(WM_RIGHT_PGA | get_left_pga_gain() | PGA_UPDATE);

  /* High impedance VMID: Slow Charging time, low power usage */
  WriteI2C(WM_BIAS_CTRL | VMID_300000_OMHS | BIAS_LEFT_ENABLE | BIAS_RIGHT_ENABLE);

  WaitForI2C();

  VMID_INIT();
  VMID_ON();

  /* Put the interface on standby */
  wm8737_power_standby();

  return 1;
}

/* -------- Clock -------- */

void wm8737_clock_on(void) {
  /* Setup the clocking for the ADC @ 12MHz */
  LPC_SYSCON->CLKOUTCLKSEL = 0x3; /* Clock direct from main clock */
  LPC_SYSCON->CLKOUTUEN = 0;
  LPC_SYSCON->CLKOUTUEN = 1; /* Update the clock source */
  LPC_SYSCON->CLKOUTCLKDIV = 1; /* Output clock divided by 1 = 12MHz */
  /* Configure the CLKOUT pin */
  LPC_IOCON->PIO0_1 &= ~0x7;
  LPC_IOCON->PIO0_1 |= 0x1; /* Select Function CLKOUT */
  // TODO - See if changing pin to open-drain mode saves power
}
void wm8737_clock_off(void) {
  /* Return the CLKOUT pin to GPIO */
  LPC_IOCON->PIO0_1 &= ~0x7;
}

/* -------- Power -------- */
/**
 * VMID and VREF need to be on for 500-1000ms before sampling.
 * PGA and ADC need to be on 10-20ms before sampling.
 * The audio interface can be started just before.
 * TODO: VMID impedance.
 */
void wm8737_power_standby(void) {
  WriteI2C(WM_POWER_CTRL);
  WaitForI2C();
}
void wm8737_power_on(void) {
  WriteI2C(WM_POWER_CTRL | POWER_AUDIO_INTERFACE | POWER_VMID | POWER_VREF |
	   POWER_PGA_LEFT | POWER_PGA_RIGHT | POWER_ADC_LEFT | POWER_ADC_RIGHT);
  WaitForI2C();
}

/* -------- SPI -------- */
void wm8737_spi_on(void) {
  /* ADCDAT pin enabled, DSP Mode, 16 bit sampling, DSP Mode B, Slave Mode */
  WriteI2C(WM_AUDIO_FMAT | 0 | FMAT_LRP_MODEB | FMAT_DSP | FMAT_16BIT |
	   FMAT_SLAVE);
  WaitForI2C();
}
void wm8737_spi_off(void) {
  /* ADCDATA pin high impedance, DSP Mode, 16 bit sampling, DSP Mode B, Slave Mode */
  WriteI2C(WM_AUDIO_FMAT | FMAT_ADCDAT_PIN_HIGH_I | FMAT_LRP_MODEB |
	   FMAT_DSP | FMAT_16BIT | FMAT_SLAVE);
  WaitForI2C();
}

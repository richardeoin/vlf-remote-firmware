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

#ifndef WM8737_H
#define WM8737_H

#include "LPC11xx.h"

/**
 * Register Addresses (shift them up 9 too)
 */
enum {
  WM_LEFT_PGA		= (0x00 << 9),
  WM_RIGHT_PGA		= (0x01 << 9),
  WM_LEFT_PATH		= (0x02 << 9),
  WM_RIGHT_PATH		= (0x03 << 9),
  WM_3D_ENHANCE		= (0x04 << 9),
  WM_ADC_CONTROL	= (0x05 << 9),
  WM_POWER_CTRL		= (0x06 << 9),
  WM_AUDIO_FMAT		= (0x07 << 9),
  WM_CLOCKING		= (0x08 << 9),
  WM_PREAMP_CTRL	= (0x09 << 9),
  WM_BIAS_CTRL		= (0x0A << 9),
  WM_NOISE_GATE		= (0x0B << 9),
  WM_ALC1		= (0x0C << 9),
  WM_ALC2		= (0x0D << 9),
  WM_ALC3		= (0x0E << 9),
  WM_RESET		= (0x0F << 9),
};
/**
 * WM_PGA
 */
enum {
  PGA_UPDATE		= (1 << 8),
};
/**
 * WM_PATH
 */
enum {
  PATH_INPUT1		= (0 << 7),
  PATH_INPUT2		= (1 << 7),
  PATH_INPUT3	       	= (2 << 7),
  PATH_DCINPUT1		= (3 << 7),
  PATH_13DB_MICBOOST	= (0 << 5),
  PATH_18DB_MICBOOST	= (1 << 5),
  PATH_28DB_MICBOOST	= (2 << 5),
  PATH_33DB_MICBOOST	= (3 << 5),
  PATH_MICBOOST_ENABLE	= (1 << 4),
};
/**
 * WM_ADC_CONTROL
 */
enum {
  ADC_ANALOGUE_MONO_MIX	= (1 << 7),
  ADC_DIGITAL_MONO_MIX	= (2 << 7),
  ADC_LEFT_INVERT	= (1 << 5),
  ADC_RIGHT_INVERT	= (2 << 5),
  ADC_AUTOUPDATE_OFFSET	= (1 << 4),
  ADC_LOW_POWER		= (1 << 2),
  ADC_DUAL_MONO_OUTPUT	= (1 << 1),
  ADC_DISABLE_HIGH_PASS	= (1 << 0),
};
/**
 * WM_POWER_CTRL
 */
enum {
  POWER_VMID		= (1 << 8),
  POWER_VREF		= (1 << 7),
  POWER_AUDIO_INTERFACE	= (1 << 6),
  POWER_PGA_LEFT	= (1 << 5),
  POWER_PGA_RIGHT	= (1 << 4),
  POWER_ADC_LEFT	= (1 << 3),
  POWER_ADC_RIGHT	= (1 << 2),
  POWER_MICBIAS_75AVDD	= 1,	/* Micbias is 75% of AVDD */
  POWER_MICBIAS_90AVDD	= 2,
  POWER_MICBIAS_120AVDD	= 3,	/* Micbias is 120% of AVDD. Honest, check datasheet */
};
/**
 * WM_AUDIO_FMAT
 */
enum {
  FMAT_RIGHT_JUSTIFIED	= 0,
  FMAT_LEFT_JUSTIFIED	= 1,
  FMAT_I2S		= 2,
  FMAT_DSP		= 3,
  FMAT_16BIT		= (0 << 2),
  FMAT_20BIT		= (1 << 2),
  FMAT_24BIT		= (2 << 2),
  FMAT_32BIT		= (3 << 2),
  FMAT_LRP_MODEB	= (1 << 4),
  FMAT_MASTER		= (1 << 6),
  FMAT_SLAVE		= 0,
  FMAT_ADCDAT_PIN_HIGH_I= (1 << 7),
};
/**
 * WM_CLOCKING
 */
enum {
  CLOCKING_AUTO			= 0x80,
  CLOCKING_DIV2			= 0x40,
  CLOCKING_USB			= 0x01,

  CLOCKING_SR_USB_16000HZ	= 0x14,
  CLOCKING_SR_USB_20059HZ	= 0x36,
  CLOCKING_SR_USB_24000HZ	= 0x38,
  CLOCKING_SR_USB_32000HZ	= 0x18,
  CLOCKING_SR_USB_44188HZ	= 0x22,
  CLOCKING_SR_USB_48000HZ	= 0x00,
  CLOCKING_SR_USB_88235HZ	= 0x3E,
  CLOCKING_SR_USB_96000HZ	= 0x1C,
};
/**
 * WM_PREAMP_CTRL
 */
enum {
  PREAMP_RIGHT_BYPASS		= (1 << 3),
  PREAMP_LEFT_BYPASS		= (1 << 2),
  PREAMP_BIAS100		= 3,	/* 100% preamplifier bias */
  PREAMP_BIAS75			= 2,
  PREAMP_BIAS50			= 1,
  PREAMP_BIAS25			= 0,
};
/**
 * WM_BIAS_CTRL
 */
enum {
  VMID_75000_OHMS		= (0 << 2),
  VMID_300000_OMHS		= (1 << 2),
  VMID_2500_OMHS		= (2 << 2),
  BIAS_LEFT_ENABLE		= (1 << 1),
  BIAS_RIGHT_ENABLE		= (1 << 0),
};

int32_t wm8737_init(void);
void wm8737_clock_on(void);
void wm8737_clock_off(void);
void wm8737_power_standby(void);
void wm8737_power_on(void);
void wm8737_spi_on(void);
void wm8737_spi_off(void);

#endif /* WM8737_H */

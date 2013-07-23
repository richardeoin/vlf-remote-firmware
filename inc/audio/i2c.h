/* 
 * A partial implementation of the I2C protocol for communications
 * with the WM8737 ADC.
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

#ifndef I2C_H
#define I2C_H

volatile int8_t I2CMode; // 0 = Write, 1 = Ping
volatile int8_t I2CDone; // -1 = Failed, 0 = working, 1 = done

volatile uint16_t I2CValue;
volatile uint8_t I2CIndex;

#define I2CMAX_TIMEOUT	0x0FFFFF

#define WM8737_ADDR	0x34

#define I2C_I2EN	(1<<6)  /* I2C Control Set & Clear Registers */
#define I2C_STA        	(1<<5)	/* aka STAC */
#define I2C_STO		(1<<4)
#define I2C_SI		(1<<3)	/* aka SIC */
#define I2C_AA         	(1<<2)	/* aka AAC */

/**
 * Set the duty cycle for a 400kHz bus speed (with 12MHz PCLK)
 */
#define I2SCLH		15	/* I2C SCL Duty Cycle High Register */
#define I2SCLL		15	/* I2C SCL Duty Cycle Low Register */

void InitI2C(void);

void WriteI2C(uint16_t value);
uint8_t PingI2C(void);
void WaitForI2C(void);

#endif /* I2C_H */

/* 
 * A partial implementation of the I2C protocol for communications
 * with the WM8737 ADC.
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

#include "LPC11xx.h"
#include "audio/i2c.h"

void InitI2C(void) {
  LPC_SYSCON->PRESETCTRL |= (0x1<<1);

  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);
  LPC_IOCON->PIO0_4 &= ~0x3F;		/* I2C I/O config */
  LPC_IOCON->PIO0_4 |= 0x01;		/* I2C SCL */
  LPC_IOCON->PIO0_5 &= ~0x3F;
  LPC_IOCON->PIO0_5 |= 0x01;		/* I2C SDA */

  //  LPC_IOCON->PIO0_4 |= (0x1<<10);	/* open drain pins */
  //  LPC_IOCON->PIO0_5 |= (0x1<<10);	/* open drain pins */

  /*--- Clear flags ---*/
  LPC_I2C->CONCLR = I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;

  /*--- Setup the timings ---*/
  LPC_I2C->SCLL = I2SCLL;
  LPC_I2C->SCLH = I2SCLH;

  /* Clear the variables */
  I2CDone = 1;

  /* Enable the I2C Interrupt */
  NVIC_SetPriority(I2C_IRQn, 2);
  NVIC_EnableIRQ(I2C_IRQn);

  LPC_I2C->CONSET = I2C_I2EN;
}
void WriteI2C(uint16_t value) {
  /* Wait for any currently pending transactions to complete */
  WaitForI2C();

  I2CIndex = 0; I2CMode = 0; I2CDone = 0; I2CValue = value;
  LPC_I2C->CONSET = I2C_STA;	/* Set Start flag */
}
uint8_t PingI2C(void) {
  /* Wait for any currently pending transactions to complete */
  WaitForI2C();

  I2CMode = 1; I2CDone = 0;
  LPC_I2C->CONSET = I2C_STA;	/* Set Start flag */

  /* Wait for the transaction to complete */
  WaitForI2C();

  return I2CDone;
}
void WaitForI2C(void) {
  int i = 0;

  while (I2CDone == 0 && i < I2CMAX_TIMEOUT) { i++; }

  if (I2CDone == 0) { // Still haven't finished...
    /* Attempt to dispatch a Stop bit */
    LPC_I2C->CONSET = I2C_STO;
  }
}

/**
 * This code can only do writes, it's all the WM8737 supports!
 */
void I2C_IRQHandler(void) {
  uint8_t StatValue;

  /* This handler deals with master write only. */
  StatValue = LPC_I2C->STAT;

  switch (StatValue) {
    case 0x08: /* A Start condition was issued. */
      /* Send Slave Address*/
      LPC_I2C->DAT = WM8737_ADDR;
      LPC_I2C->CONCLR = (I2C_SI | I2C_STA);
      return;

    case 0x18: /* ACK following Slave Address (Write) */
      if (I2CMode == 1) { /* Ping Mode */
	LPC_I2C->CONSET = I2C_STO; /* Set Stop flag */
	LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
	I2CDone = 1; return; /* We're done */
      } else { /* Read or Write Mode */
	LPC_I2C->DAT = (I2CValue >> 8) & 0xFF; /* Write MSB */
	LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
	return;
      }

    case 0x20: /* NACK following Slave Address (Write) */
      /* Can't find slave... Clear up and go home */
      LPC_I2C->CONSET = I2C_STO; /* Set Stop flag */
      LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
      I2CDone = -1; return; /* We're done */
    case 0x28: /* ACK following Data Byte (Write Mode) */
      /* Must be write mode */
      if (I2CIndex == 0) {
	LPC_I2C->DAT = I2CValue & 0xFF; /* Write LSB */
	LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
	I2CIndex++;
	return;
      } else { /* Done the 2nd byte now, time to stop */
	LPC_I2C->CONSET = I2C_STO; /* Set Stop flag */
	LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
	I2CDone = 1; return;
      }
      break;

    case 0x30: /* NACK following Data Byte (Write Mode) */
      /* The slave doesn't want the write to proceed */
      LPC_I2C->CONSET = I2C_STO; /* Set Stop flag */
      LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
      I2CDone = -1; return; /* Error :( */

    case 0x38: /* Arbitration lost we don't deal with multiple master situation */
    default:
      LPC_I2C->CONCLR = I2C_SI; /* Clear the SI bit */
      return;
  }
  return;
}

/* 
 * Functions for driving an AT86RF212. A radif structure is used to
 * interact with hardware.
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

#include "radif.h"
#include "at86rf212_constants.h"
#include "ieee_frame.h"

/**
 * Define if the we should try to wait for a response to every
 * transmission.
 */
#define QUERY_MODE	1
/**
 * If QUERY_MODE is defined, how many milliseconds we should wait for
 * a response to our transmissions before giving up.
 */
#define QUERY_TIMEOUT	50

/**
 * This is what's sent when the radio doesn't care what we send.
 */
#define BLANK_SPI_CHARACTER 0x00

/**
 * The current sequence number
 */
uint8_t sequence;
/**
 * PAN ID and Short Address of this node
 */
uint16_t pan_id, short_address;

/* -------- Register Read, Write & Read-Modify-Write -------- */

uint8_t at86rf212_reg_read(uint8_t addr, struct radif* radif) {
  radif->enter_protected();
  radif->spi_start();

  /* Send Register address and read register content.*/
  radif->spi_xfer(addr | RADIO_SPI_CMD_RR);
  uint8_t val = radif->spi_xfer(BLANK_SPI_CHARACTER);

  radif->spi_stop();
  radif->exit_protected();

  return val;
}
uint16_t at86rf212_read16(struct radif* radif) {

  uint16_t val = radif->spi_xfer(BLANK_SPI_CHARACTER);
  val |= (radif->spi_xfer(BLANK_SPI_CHARACTER) << 8);

  return val;
}
uint16_t at86rf212_reg_read16(uint8_t addr, struct radif* radif) {

  uint16_t val = at86rf212_reg_read(addr, radif);
  val |= at86rf212_reg_read(addr+1, radif) << 8;

  return val;
}
void at86rf212_reg_write(uint8_t addr, uint8_t val, struct radif* radif) {
  radif->enter_protected();
  radif->spi_start();

  /* Send Register address and write register content.*/
  radif->spi_xfer(addr | RADIO_SPI_CMD_RW);
  radif->spi_xfer(val);

  radif->spi_stop();
  radif->exit_protected();
}
void at86rf212_write16(uint16_t val, struct radif* radif) {
  radif->spi_xfer(val & 0xFF);
  radif->spi_xfer((val >> 8) & 0xFF);
}
void at86rf212_reg_write16(uint8_t addr, uint16_t val, struct radif* radif) {

  at86rf212_reg_write(addr, val, radif);
  at86rf212_reg_write(addr+1, val >> 8, radif);
}
void at86rf212_reg_write64(uint8_t addr, uint8_t *val, struct radif* radif) {
  int i;

  for (i=0; i<8; i++) {
    at86rf212_reg_write(addr + i, *(val + i), radif);
  }
}
void at86rf212_reg_read_mod_write(uint8_t addr, uint8_t val, uint8_t mask,
				  struct radif* radif) {
  uint8_t tmp;

  tmp = at86rf212_reg_read(addr, radif);
  val &= mask;                /* Mask off stray bits from value */
  tmp &= ~mask;               /* Mask off bits in register value */
  tmp |= val;                 /* Copy value into register value */
  at86rf212_reg_write(addr, tmp, radif);   /* Write back to register */
}

/* -------- SRAM Read & Write -------- */

void at86rf212_sram_read(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif) {
  int i;

  radif->enter_protected();
  radif->spi_start();

  /* Send SRAM read command.*/
  radif->spi_xfer(RADIO_SPI_CMD_SR);

  /* Send address where to start reading.*/
  radif->spi_xfer(addr);

  for (i = 0; i < len; i++) {
    *data++ = radif->spi_xfer(BLANK_SPI_CHARACTER);
  }

  radif->spi_stop();
  radif->exit_protected();
}
void at86rf212_sram_write(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif) {
  int i;

  radif->enter_protected();
  radif->spi_start();

  /* Send SRAM write command.*/
  radif->spi_xfer(RADIO_SPI_CMD_SW);

  /* Send address where to start writing to.*/
  radif->spi_xfer(addr);

  for (i = 0; i < len; i++) {
    radif->spi_xfer(*data++);
  }

  radif->spi_stop();
  radif->exit_protected();
}

/* -------- Radio State -------- */

uint8_t at86rf212_get_state(struct radif* radif) {
  return at86rf212_reg_read(TRX_STATUS, radif) & 0x1f;
}
uint8_t at86rf212_set_state(uint8_t state, struct radif* radif) {
  uint8_t curr_state = at86rf212_get_state(radif);

  /* If we're already in the correct state it's not a problem */
  if (curr_state == state) { return RADIO_SUCCESS; }

  /* If we're in a transition state, wait for the state to become stable */
  if ((curr_state == BUSY_TX_ARET) || (curr_state == BUSY_RX_AACK) ||
      (curr_state == BUSY_RX) || (curr_state == BUSY_TX)) {

    while (at86rf212_get_state(radif) == curr_state);
  }

  /* At this point it is clear that the requested new_state is one of */
  /* TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON or TX_ARET_ON. */
  /* we need to handle some special cases before we transition to the new state */
  switch (state) {
    case TRX_OFF:
      /* Go to TRX_OFF from any state. */
      radif->slptr_clear();
      at86rf212_reg_read_mod_write(TRX_STATE, CMD_FORCE_TRX_OFF, 0x1f, radif);
      radif->delay_us(TIME_ALL_STATES_TRX_OFF);
      break;

    case TX_ARET_ON:
      if (curr_state == RX_AACK_ON) {
	/* First do intermediate state transition to PLL_ON, then to TX_ARET_ON. */
	at86rf212_reg_read_mod_write(TRX_STATE, CMD_PLL_ON, 0x1f, radif);
	radif->delay_us(TIME_RX_ON_TO_PLL_ON);
      }
      break;

    case RX_AACK_ON:
      if (curr_state == TX_ARET_ON) {
	/* First do intermediate state transition to RX_ON, then to RX_AACK_ON. */
	at86rf212_reg_read_mod_write(TRX_STATE, CMD_PLL_ON, 0x1f, radif);
	radif->delay_us(TIME_RX_ON_TO_PLL_ON);
      }
      break;
  }

  /* Now we're okay to transition to any new state. */
  at86rf212_reg_read_mod_write(TRX_STATE, state, 0x1f, radif);

  /* When the PLL is active most states can be reached in 1us. However, from */
  /* TRX_OFF the PLL needs time to activate. */
  uint32_t delay = (curr_state == TRX_OFF) ? TIME_TRX_OFF_TO_PLL_ON : TIME_RX_ON_TO_PLL_ON;
  radif->delay_us(delay);

  if (at86rf212_get_state(radif) == state) {
    return RADIO_SUCCESS;
  }

  return RADIO_TIMED_OUT;
}

uint8_t at86rf212_is_state_busy(struct radif* radif) {
  uint8_t state = at86rf212_get_state(radif);
  if (state == BUSY_RX || state == BUSY_RX_AACK || state == BUSY_RX_AACK_NOCLK ||
      state == BUSY_TX || state == BUSY_TX_ARET) {
    return 1;
  } else {
    return 0;
  }
}
uint8_t at86rf212_get_trac(struct radif* radif) {
  radif->last_trac_status = (at86rf212_reg_read(TRX_STATE, radif) >> RADIO_TRAC_STATUS_POS);
  return radif->last_trac_status;
}

/* -------- Set Radio Properties -------- */

void at86rf212_set_modulation(uint8_t modulation, struct radif* radif) {
  /* The radio must be in TRX_OFF to change the modulation */
  at86rf212_set_state(TRX_OFF, radif);

  at86rf212_reg_read_mod_write(TRX_CTRL_2, modulation, 0x3F, radif);

  if (modulation & RADIF_OQPSK) { /* According to table 7-16 in AT86RF212 datasheet */
    at86rf212_reg_read_mod_write(RF_CTRL_0, RADIO_OQPSK_TX_OFFSET, 0x3, radif);
  } else {
    at86rf212_reg_read_mod_write(RF_CTRL_0, RADIO_BPSK_TX_OFFSET, 0x3, radif);
  }
}
uint8_t at86rf212_set_freq(uint16_t freq, struct radif* radif) {
  uint8_t band, number;

  /**
   * Translate the frequency (given in MHz or 100s of kHz) into a band and number
   * See Table 7-35 in the AT86RF212 datasheet
   */
  if (7690 <= freq && freq <= 7945) { /* 769.0 MHz - 794.5 MHz: Chinese Band */
    band = 1; number = freq-7690;
  } else if (8570 <= freq && freq <= 8825) { /* 857.0 MHz - 882.5 MHz: European Band */
    band = 2; number = freq-8570;
  } else if (9030 <= freq && freq <= 9285) { /* 903.0 MHz - 928.5 MHz: North American Band */
    band = 3; number = freq-9030;
  } else if (769 <= freq && freq <= 863) { /* 769 MHz - 863 MHz: General 1 */
    band = 4; number = freq-769;
  } else if (833 <= freq && freq <= 935) { /* 833 MHz - 935 MHz: General 2 */
    band = 5; number = freq-833;
  } else { /* Unknown frequency */
    return RADIO_INVALID_ARGUMENT;
  }

  /* Write these values to the control register */
  at86rf212_reg_read_mod_write(CC_CTRL_1, band, 0x7, radif);
  at86rf212_reg_write(CC_CTRL_0, number, radif);

  /* Add a delay to allow the PLL to lock if in active mode. */
  uint8_t state = at86rf212_get_state(radif);
  if ((state == RX_ON) || (state == PLL_ON)) {
    radif->delay_us(TIME_PLL_LOCK_TIME);
  }

  /* TODO: Wait for the PLL to lock instead */

  return RADIO_SUCCESS;
}
void at86rf212_set_power(uint8_t power, struct radif* radif) {
  at86rf212_reg_write(PHY_TX_PWR, power, radif);
}
void at86rf212_set_clkm(uint8_t clkm, struct radif* radif) {
  at86rf212_reg_read_mod_write(TRX_CTRL_0, clkm, 0x3F, radif);
}
void at86rf212_set_address(uint16_t _pan_id, uint16_t _short_address, struct radif* radif) {
  /* Set PAN ID */
  pan_id = _pan_id;
  at86rf212_reg_write16(PAN_ID_0, pan_id, radif);

  /* Set the Short Address */
  short_address = _short_address;
  at86rf212_reg_write16(SHORT_ADDR_0, short_address, radif);
}

/* -------- Wake & Sleep -------- */

void at86rf212_sleep(struct radif* radif) {
  /* First we need to go to TRX OFF state */
  at86rf212_set_state(TRX_OFF, radif);

  /* Set the SLPTR pin */
  radif->slptr_set();
}
void at86rf212_wake(struct radif* radif) {
  /* Clear the SLPTR pin */
  radif->slptr_clear();

  /* We need to allow some time for the PLL to lock */
  radif->delay_us(TIME_SLEEP_TO_TRX_OFF);

  /* Turn the transceiver back on */
  at86rf212_set_state(RX_AACK_ON, radif);
}

/* -------- Misc -------- */

uint8_t at86rf212_get_random(struct radif* radif) {
  /* Set the radio in the standard operating mode to do this */
  at86rf212_set_state(RX_ON, radif);

  uint8_t i, rand = 0;
  for (i = 0; i < 8; i+=2) {
    rand |= ((at86rf212_reg_read(PHY_RSSI, radif) << 1) & 0xC0) >> i;
  }

  return rand;
}
uint8_t at86rf212_measure_energy(struct radif* radif) { /* TODO: Make this more efficient */
  /* Set the radio in the standard operating mode to do this */
  at86rf212_set_state(RX_ON, radif);

  /* Write to PHY_ED_LEVEL to trigger off the measurement */
  at86rf212_reg_write(PHY_ED_LEVEL, BLANK_SPI_CHARACTER, radif);

  /* Enable the CCA_ED_DONE interrupt */
  at86rf212_reg_read_mod_write(IRQ_MASK, RADIO_IRQ_CCA_ED_DONE, RADIO_IRQ_CCA_ED_DONE, radif);

  /* Wait for interrupt CCA_ED_DONE */
  while ((at86rf212_reg_read(IRQ_STATUS, radif) & RADIO_IRQ_CCA_ED_DONE) == 0);

  /* Disable the CCA_ED_DONE interrupt */
  at86rf212_reg_read_mod_write(IRQ_MASK, 0, RADIO_IRQ_CCA_ED_DONE, radif);

  /* Return the result */
  return at86rf212_reg_read(PHY_ED_LEVEL, radif);
}

/* -------- Reset -------- */

uint8_t at86rf212_reset(struct radif* radif) {
  /* This is the reset procedure as per Table A-5 (p166) of the AT86RF212 datasheet */

  /* Set input pins to their default operating values */
  radif->reset_clear();
  radif->slptr_clear();
  radif->spi_stop();

  /* Wait while transceiver wakes up */
  radif->delay_us(TIME_P_ON_WAIT);

  /* Reset the device */
  radif->reset_set();
  radif->delay_us(TIME_RST_PULSE_WIDTH);
  radif->reset_clear();

  uint8_t i = 0;
  /* Check that we have the part number that we're expecting */
  while ((at86rf212_reg_read(VERSION_NUM, radif) != AT86RF212_VER_NUM) ||
	 (at86rf212_reg_read(PART_NUM, radif) != AT86RF212_PART_NUM)) {
    if (i++ > 100) {
      /* This is never going to work, we've got the wrong part number */
//      return RADIO_UNSUPPORTED_DEVICE;
    }
  }

  at86rf212_reg_read_mod_write(TRX_CTRL_0, CLKM_1MHz | CLKM_DRIVE_4mA, 0x3F, radif);

  /* Force transceiver into TRX_OFF state */
  at86rf212_reg_read_mod_write(TRX_STATE, CMD_FORCE_TRX_OFF, 0x1F, radif);
  radif->delay_us(TIME_ALL_STATES_TRX_OFF);

  i = 0;
  /* Make sure the transceiver is in the off state before proceeding */
  while ((at86rf212_reg_read(TRX_STATUS, radif) & 0x1f) != TRX_OFF) {
    if (i++ > 100) {	/* Nope, it's never going to change state */
      return RADIO_WRONG_STATE;
    }
  }

  at86rf212_reg_read(IRQ_STATUS, radif);	/* Clear any outstanding interrupts */
  at86rf212_reg_write(IRQ_MASK, 0, radif);	/* Disable interrupts */

  return RADIO_SUCCESS;
}
/**
 * Configure the radio and make it operational.
 */
void at86rf212_startup(struct radif* radif) {
  /* Set the number of retries if no ACK is received */
  at86rf212_reg_write(XAH_CTRL_0,
		  (RADIO_MAX_FRAME_RETRIES << 4) | (RADIO_MAX_CSMA_RETRIES << 1), radif);

  /* Set frame version that we'll accept */
  at86rf212_reg_read_mod_write(CSMA_SEED_1, RADIO_FRM_VER << RADIO_FVN_POS,
			       3 << RADIO_FVN_POS, radif);

  /* Enable interrupts */
  at86rf212_reg_write(IRQ_MASK, RADIO_IRQ_RX_START | RADIO_IRQ_TRX_END, radif);

  /* Enable Automatic CRC */
  at86rf212_reg_read_mod_write(TRX_CTRL_1, RADIO_AUTO_CRC_GEN, RADIO_AUTO_CRC_GEN, radif);

  /* Enable promiscuous mode */
//  at86rf212_reg_read_mod_write(XAH_CTRL_1, RADIO_PROMISCUOUS, RADIO_PROMISCUOUS, radif);

  /* Take a random sequence number to start with */
  sequence = at86rf212_get_random(radif);

  /* Start the radio in receiving mode */
  while (at86rf212_set_state(RX_AACK_ON, radif) != RADIO_SUCCESS);
}

/* -------- Transmit & Receive -------- */

/**
 * Called when the radio has finished receiving a frame.
 */
void at86rf212_rx(struct radif* radif) { 
  uint8_t data[0x80], length, energy_detect, crc_status;
  uint16_t source_address, destination_address;
 
  /* Get the ED measurement */
  energy_detect = at86rf212_reg_read(PHY_ED_LEVEL, radif);

  /* Find out if the CRC on the last received packet was valid */
  crc_status = (at86rf212_reg_read(PHY_RSSI, radif) & (1<<7)) ? 1 : 0;
  
  radif->enter_protected();
  radif->spi_start();

  /* Send frame read command */
  radif->spi_xfer(RADIO_SPI_CMD_FR);

  /* Read the length of the whole frame */
  uint8_t mpdu_len = radif->spi_xfer(BLANK_SPI_CHARACTER);

  /* Frame Control Field (FCF) */
  uint16_t fcf = at86rf212_read16(radif);
  /* Decode the FCF */
  uint8_t pan_id_compression = (fcf & FCF_PAN_ID_COMPRESSION);
  uint8_t dest_addr_mode = (fcf >> 10) & 0x3;
  uint8_t frame_version = (fcf >> 12) & 0x3;
  uint8_t src_addr_mode = (fcf >> 14) & 0x3;

  /* Sequence Number */
  uint8_t rx_sequence = radif->spi_xfer(BLANK_SPI_CHARACTER);

  /* Keep track for how many bytes we've read from this point on */
  uint8_t hdr_len = 3;

  if (dest_addr_mode == FRAME_PAN_ID_16BIT_ADDR) {
    at86rf212_read16(radif); /* PAN ID */
    destination_address = at86rf212_read16(radif); /* Destination Address */
    hdr_len += 4;
  } else if (dest_addr_mode == FRAME_PAN_ID_64BIT_ADDR) {
    at86rf212_read16(radif); /* PAN ID */
    hdr_len += 2;
    /* TODO */
  }

  if (src_addr_mode == FRAME_PAN_ID_16BIT_ADDR) {
    if (pan_id_compression) {
      /* TODO */
    } else {
      at86rf212_read16(radif); /* PAN ID */
      hdr_len += 2;
    }
    source_address = at86rf212_read16(radif); /* Source Address */
    hdr_len += 2;
  } else if (src_addr_mode == FRAME_PAN_ID_64BIT_ADDR) {
    if (pan_id_compression) {
      /* TODO */
    } else {
      at86rf212_read16(radif); /* PAN ID */
      hdr_len += 2;
    }
    /* TODO */
  }

  /* TODO: Possible Security Header */

  /* Things we didn't use */
  UNUSED(rx_sequence);
  UNUSED(frame_version);
  UNUSED(destination_address);
  UNUSED(crc_status);

  /* Work out how long the actual data is */
  length = mpdu_len - (hdr_len + 2);

  /* Read in the MAC Service Data Unit */
  uint8_t i;
  for (i = 0; i < length; i++) {
    data[i] = radif->spi_xfer(BLANK_SPI_CHARACTER);
  }
  data[i] = 0; /* Null terminator */

  /* We don't bother reading in the Frame Check Sequence */

  radif->spi_stop();
  radif->exit_protected();

  /* Increment the statistics */
  radif->rx_success_count++;

  /* Make a callback with the received packet */
  radif->rx_callback(data, length, energy_detect, source_address);
}
/**
 * Sends a frame
 */
void at86rf212_tx(uint8_t* data, uint8_t length, uint16_t destination,
		  uint8_t ack, struct radif* radif) {

  /* Stop whatever else we were up to */
  at86rf212_set_state(TRX_OFF, radif);
  /* Get ready to transmit */
  at86rf212_set_state(TX_ARET_ON, radif);

  radif->enter_protected();
  radif->spi_start();

  /* Send frame write command */
  radif->spi_xfer(RADIO_SPI_CMD_FW);

  /* Write out the length (MAC Service Data Unit + Header + FCS) */
  uint8_t len = length + 9 + 2;
  radif->spi_xfer(len);

  /* If the length is too big, halt! */
  if (len > 127) {
    while(1); /* ERROR - Frame too long! */
  }

  /* Write out the header */
  uint16_t fcf = 
    (FRAME_PAN_ID_16BIT_ADDR << 14) | /* Source addressing mode */
    (FRAME_VERSION_IEEE_2006 << 12) | /* Frame version */
    (FRAME_PAN_ID_16BIT_ADDR << 10) | /* Destination addressing mode */
    FCF_PAN_ID_COMPRESSION | FRAME_TYPE_DATA; /* Frame type = data */

  /* Set the acknowledge flag if we've been asked */
  if (ack) {
    fcf |= FCF_ACKNOWLEDGE_REQUEST;
  }

  /* Frame Control Field (FCF) */
  at86rf212_write16(fcf, radif);

  /* Sequence Number */
  radif->spi_xfer(sequence++);

  /* Destination PAN ID */
  at86rf212_write16(pan_id, radif);

  /* Destination Address */
  at86rf212_write16(destination, radif);

  /* Source Address */
  at86rf212_write16(short_address, radif);

  /* TODO: Possible Security Header */

  /* Write out the MAC Service Data Unit */
  uint8_t i;
  for (i = 0; i < length; i++) {
    radif->spi_xfer(data[i]);
  }

  /**
   * Write two bytes in place of the Frame Check Sequence which will
   * be added by the at86rf212
   */
  radif->spi_xfer(BLANK_SPI_CHARACTER);
  radif->spi_xfer(BLANK_SPI_CHARACTER);

  radif->spi_stop();
  radif->exit_protected();

  /* Actually start the transmission */
  at86rf212_reg_read_mod_write(TRX_STATE, CMD_TX_START, 0x1F, radif);
}
/**
 * Called when the radio has finished transmitting a frame.
 */
void at86rf212_tx_end(struct radif* radif) {
  uint32_t i = 0;

  /* See how the transmission went */
  uint8_t trac_status = at86rf212_get_trac(radif);

  radif->last_trac_status = trac_status;

  /* Update the statistics */
  if (trac_status == TRAC_SUCCESS || trac_status == TRAC_SUCCESS_DATA_PENDING) {
    radif->tx_success_count++;
  } else if (trac_status == TRAC_CHANNEL_ACCESS_FAIL) {
    radif->tx_channel_fail++;
  } else if (trac_status == TRAC_NO_ACK) {
    radif->tx_noack++;
  } else {
    /* This should never happen. Was at86rf212_tx_end() called too early? */
    radif->tx_invalid++;
  }

  /* Return the radio to receiving mode */
  while (at86rf212_set_state(RX_AACK_ON, radif) != RADIO_SUCCESS);

#if QUERY_MODE
  if (trac_status == TRAC_SUCCESS || trac_status == TRAC_SUCCESS_DATA_PENDING) {

    /* Wait for something to be received */
    while ((at86rf212_reg_read(IRQ_STATUS, radif) & RADIO_IRQ_TRX_END) == 0) {
      if (i++ > QUERY_TIMEOUT*10) { return; }
      radif->delay_us(100);
    }

    /* When it is, call the handler */
    at86rf212_rx(radif);
  }
#endif
}

/* -------- Interrupt -------- */

/**
 * Should be called when the interrupt pin on the AT86RF212 goes off.
 */
void at86rf212_interrupt(struct radif* radif) { 
  uint8_t intp_src = at86rf212_reg_read(IRQ_STATUS, radif);
  uint8_t state = at86rf212_get_state(radif);

  /* Deal with each of the current interrupts in turn */
  if (intp_src & RADIO_IRQ_RX_START) {
    /**
     * We could start to read in frames here, but then we'd have to
     * stagger the SPI read, and I cba
     */
  }
  if (intp_src & RADIO_IRQ_TRX_END) {
    if (state == RX_ON || state == RX_AACK_ON || state == BUSY_RX_AACK) {
      /* We've been receiving */
      at86rf212_rx(radif);
    } else {
      /* We've been transmitting */
      at86rf212_tx_end(radif);
    }
  }
  if (intp_src & RADIO_IRQ_TRX_UR) {
    /**
     * We shouldn't get any problems here as long as the SPI clock is
     * higher than the radio link bitrate
     */
  }
  if (intp_src & RADIO_IRQ_PLL_UNLOCK) {
  }
  if (intp_src & RADIO_IRQ_PLL_LOCK) {
  }
  if (intp_src & RADIO_IRQ_BAT_LOW) {
  }
}

/* 
 * Definitions for IEEE 802.15.4 frame fields
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

#ifndef IEEE_FRAME_H
#define IEEE_FRAME_H

/**
 * Frame Control Fields
 */
enum {
  FCF_PAN_ID_COMPRESSION	= 0x40,
  FCF_ACKNOWLEDGE_REQUEST	= 0x20,
  FCF_FRAME_PENDING		= 0x10,
  FCF_SECURITY_ENABLED		= 0x08
};
/**
 * Frame Type
 */
enum {
  FRAME_TYPE_MAC_COMMAND	= 3,
  FRAME_TYPE_ACKNOWLEDGE	= 2,
  FRAME_TYPE_DATA		= 1,
  FRAME_TYPE_BEACON		= 0
};
/**
 * Frame Version
 */
enum {
  FRAME_VERSION_IEEE_2006	= 1,
  FRAME_VERSION_IEEE_2003	= 0
};
/**
 * Frame Addressing
 */
enum {
  FRAME_NO_ADDRESS		= 0,
  FRAME_PAN_ID_16BIT_ADDR	= 2,
  FRAME_PAN_ID_64BIT_ADDR	= 3
};

#endif /* IEEE_FRAME_H */

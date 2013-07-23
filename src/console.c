/* 
 * A printf-style console that operates over the radio interface.
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "console.h"
#include "radio/rf212.h"
#include "radio/radio.h"

static uint8_t console_buf[CONSOLE_BUF_LEN];
static uint8_t console_buf_index = 1;

void _console_flush(void) {
  /* Write out the data that's in the buffer */
  radif_query(console_buf, console_buf_index, BASE_STATION_ADDR, 1, &rf212_radif);
  /* And go back to the start of the buffer */
  console_buf_index = 1;
}

int _console_putchar(char c) {
  console_buf[console_buf_index++] = c;

  /* If we've filled our buffer or got to a newline */
  if(console_buf_index >= CONSOLE_BUF_LEN-1 || c == '\n') {
    /* Add on a null terminator */
    console_buf[console_buf_index++] = '\0';
    /* And output everything in the buffer */
    _console_flush();
  }

  return 1;
}

void _console_printf(const char *format, ...) {
  /* Make sure the first char in the buffer is 'D' */
  console_buf[0] = 'D';

  va_list args;

  va_start(args, format);
  vsprintf((char*)(console_buf+1), format, args);
}
void _console_puts(const char *s) {
  /* Make sure the first char in the buffer is 'D' */
  console_buf[0] = 'D';

  while(*s) {
    _console_putchar(*(s++));
  }
  _console_putchar('\n');
}

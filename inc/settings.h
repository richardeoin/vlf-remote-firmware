/* 
 * Defines the system settings
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

#ifndef SETTINGS_H
#define SETTINGS_H

/**
 * ======== Tuning ========
 */
uint32_t get_em_record_flags(void);
uint32_t get_battery_record_flags(void);
uint32_t get_rssi_record_flags(void);
uint32_t get_time_jump_record_flags(void);
uint32_t get_envelope_record_flags(void);
uint8_t get_left_tuned_bin(void);
uint8_t get_right_tuned_bin(void);

/**
 * ======== Gain ========
 */
uint8_t get_left_pga_gain(void);
uint8_t get_right_pga_gain(void);
uint8_t get_left_micboost(void);
uint8_t get_right_micboost(void);

#endif /* SETTINGS_H */

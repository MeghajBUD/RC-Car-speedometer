/*
 * bb_oled.h -- bit-banged SOC1602A serial-mode driver
 *
 * Assumes MCC has generated LCD_CS, LCD_SCL, LCD_SDI as GPIO outputs
 * with SetHigh/SetLow macros in pin_manager.h.
 */

#ifndef BB_OLED_H
#define BB_OLED_H

void bb_oled_init(void);
void bb_cmd(unsigned int b);
void bb_data(unsigned int b);
void bb_write_line1(const char *s);
void bb_write_line2(const char *s);

#endif
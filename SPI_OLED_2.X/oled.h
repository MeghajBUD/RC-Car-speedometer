/*
 * File:   oled.h
 * SOC1602A OLED driver over hardware SPI1, PIC24EP128GP202
 *
 * Pins:
 *   RB0 (pin 4)  -> display pin 16 (/CS)   manual GPIO
 *   RB7 (pin 16) -> display pin 12 (SCL)   SCK1 via PPS
 *   RB8 (pin 17) -> display pin 14 (SDI)   SDO1 via PPS
 */

#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

/* Initialize SPI1, PPS, pins, and run display init sequence.
 * Call once at startup, after SYSTEM_Initialize(). */
void oled_init(void);

/* Clear display, cursor to row 0 col 0. */
void oled_clear(void);

/* Move cursor. line: 0 or 1. col: 0..15. */
void oled_set_cursor(uint8_t line, uint8_t col);

/* Print null-terminated string at current cursor position. */
void oled_print(const char *s);

/* Print one character at current cursor position. */
void oled_putc(char c);

/* printf-style print at current cursor position.
 * Max output: 16 chars (one display line). Automatically pads with
 * spaces to 16 chars so old characters are always overwritten.
 * Example:
 *   oled_set_cursor(1, 0);
 *   oled_printf("Temp: %d C", 23);
 */
void oled_printf(const char *fmt, ...);

/* Convenience: clear + home + print on row 0. */
void oled_display1(const char *s);

/* Convenience: cursor to row 1 + print (row 0 unchanged). */
void oled_display2(const char *s);

/* Convenience: printf directly to row 0 (clears row 0 first). */
void oled_printf1(const char *fmt, ...);

/* Convenience: printf directly to row 1 (row 0 unchanged). */
void oled_printf2(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* OLED_H */

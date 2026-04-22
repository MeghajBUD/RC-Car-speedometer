/*
 * File:   oled.c
 * SOC1602A OLED driver over hardware SPI1, PIC24EP128GP202
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "oled_copy.h"

/* ---- /CS control ------------------------------------------------- */
#define CS_HIGH()   (LATBbits.LATB0 = 1)
#define CS_LOW()    (LATBbits.LATB0 = 0)

/* ---- SPI1 bring-up ----------------------------------------------- */
static void spi1_init(void)
{
    SPI1STATbits.SPIEN = 0;

    __builtin_write_OSCCONL(OSCCON & 0xBF);

    __builtin_write_OSCCONL(OSCCON | 0x40);

    SPI1CON1 = 0;
    SPI1CON1bits.MSTEN  = 1;
    SPI1CON1bits.MODE16 = 1;
    SPI1CON1bits.CKP    = 0;
    SPI1CON1bits.CKE    = 1;
    SPI1CON1bits.SMP    = 1;
    SPI1CON1bits.PPRE   = 0b10;
    SPI1CON1bits.SPRE   = 0b110;

    SPI1CON2 = 0;
    SPI1STAT  = 0;
    SPI1STATbits.SPIEN = 1;
}

static void spi1_xfer16(uint16_t w)
{
    volatile uint16_t dummy;
    while (SPI1STATbits.SPIRBF) { dummy = SPI1BUF; (void)dummy; }
    SPI1BUF = w;
    while (!SPI1STATbits.SPIRBF) { }
    dummy = SPI1BUF;
    (void)dummy;
}

/* ---- Pin setup --------------------------------------------------- */
static void pins_configure(void)
{
    ANSELBbits.ANSB0 = 0;
    TRISBbits.TRISB0 = 0;
    CS_HIGH();
    TRISBbits.TRISB7 = 0;
    TRISBbits.TRISB8 = 0;
}

/* ---- Low-level frame send ---------------------------------------- */
static void oled_send(uint16_t packed10)
{
    CS_LOW();
    __delay_us(1);
    spi1_xfer16((uint16_t)(packed10 << 6));
    __delay_us(1);
    CS_HIGH();
    __delay_us(100);
}

#define OLED_CMD(c)   oled_send((uint16_t)((c) & 0xFF))
#define OLED_DAT(c)   oled_send(0x200u | ((uint16_t)((c) & 0xFF)))

/* ================================================================== */
/* Public API                                                          */
/* ================================================================== */

void oled_init(void)
{
    pins_configure();
    spi1_init();
    __delay_ms(100);

    OLED_CMD(0x38); __delay_us(600);
    OLED_CMD(0x08); __delay_us(600);
    OLED_CMD(0x01); __delay_ms(3);
    OLED_CMD(0x06); __delay_us(600);
    OLED_CMD(0x02); __delay_ms(2);
    OLED_CMD(0x0C); __delay_ms(5);
}

void oled_clear(void)
{
    OLED_CMD(0x01);
    __delay_ms(3);
}

void oled_set_cursor(uint8_t line, uint8_t col)
{
    uint8_t addr = (line ? 0x40 : 0x00) + (col & 0x0F);
    OLED_CMD(0x80 | addr);
    __delay_us(600);
}

void oled_putc(char c)
{
    OLED_DAT((uint8_t)c);
}

void oled_print(const char *s)
{
    while (*s) OLED_DAT((uint8_t)*s++);
}

/* ------------------------------------------------------------------ */
/* oled_printf: printf into a 16-char buffer, pad with spaces,        */
/* print at current cursor. Old chars always overwritten.             */
void oled_printf(const char *fmt, ...)
{
    char buf[17];
    int  len, i;
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* Clamp to 16 */
    if (len > 16) len = 16;

    /* Print formatted string */
    for (i = 0; i < len; i++)
        OLED_DAT((uint8_t)buf[i]);

    /* Pad remainder of line with spaces to erase old characters */
    for (i = len; i < 16; i++)
        OLED_DAT(' ');
}

/* ------------------------------------------------------------------ */
void oled_display1(const char *s)
{
    oled_clear();
    oled_set_cursor(0, 0);
    oled_print(s);
}

void oled_display2(const char *s)
{
    oled_set_cursor(1, 0);
    oled_print(s);
}

void oled_printf1(const char *fmt, ...)
{
    char buf[17];
    int  len, i;
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 16) len = 16;

    oled_set_cursor(0, 0);
    for (i = 0; i < len; i++) OLED_DAT((uint8_t)buf[i]);
    for (i = len; i < 16; i++) OLED_DAT(' ');
}

void oled_printf2(const char *fmt, ...)
{
    char buf[17];
    int  len, i;
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 16) len = 16;

    oled_set_cursor(1, 0);
    for (i = 0; i < len; i++) OLED_DAT((uint8_t)buf[i]);
    for (i = len; i < 16; i++) OLED_DAT(' ');
}

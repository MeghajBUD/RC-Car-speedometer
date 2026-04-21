/*
 * spi_oled.c -- SOC1602A serial-mode driver for PIC24EP128GP202
 * Minimal version -- restore what showed text, with generous delays.
 * No wake-up dance (it was sending 0x30 which might lock 1-line mode).
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/spi1.h"
#include "spi_oled.h"

void init_spi1(void)
{
    SPI1_Initialize();
    LCD_CS_SetHigh();
}

static void spi1_send_frame(unsigned int rs, unsigned int byte)
{
    unsigned int frame = 0;
    if (rs) frame |= 0x8000u;
    frame |= (byte & 0xFFu) << 6;

    LCD_CS_SetLow();
    __delay_us(2);
    (void)SPI1_Exchange16bit(frame);
    __delay_us(2);
    LCD_CS_SetHigh();
    __delay_us(2);
}

void spi_cmd(unsigned int data)
{
    spi1_send_frame(0u, data);
    __delay_us(600);                   /* datasheet: 600 us max for most commands */
}

void spi_data(unsigned int data)
{
    spi1_send_frame(1u, data);
    __delay_us(600);
}

void spi1_init_oled(void)
{
    __delay_ms(50);                    /* power stabilization */

    spi_cmd(0x38);                     /* Function Set: 8-bit, 2-line */
    spi_cmd(0x08);                     /* Display OFF */
    spi_cmd(0x01); __delay_ms(3);      /* Clear -- needs >= 2 ms */
    spi_cmd(0x06);                     /* Entry Mode: increment */
    spi_cmd(0x02); __delay_ms(2);      /* Home -- needs >= 600 us */
    spi_cmd(0x0C);                     /* Display ON */
}

void spi1_display1(const char *s)
{
    spi_cmd(0x02); __delay_ms(2);
    while (*s) spi_data((unsigned char)*s++);
}

void spi1_display2(const char *s)
{
    spi_cmd(0xC0);
    while (*s) spi_data((unsigned char)*s++);
}
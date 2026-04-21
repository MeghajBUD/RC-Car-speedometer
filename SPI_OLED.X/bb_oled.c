/*
 * bb_oled.c -- bit-banged SOC1602A serial-mode driver for PIC24EP128GP202
 *
 * Expects MCC-generated pin macros:
 *   LCD_CS_SetHigh()  / LCD_CS_SetLow()
 *   LCD_SCL_SetHigh() / LCD_SCL_SetLow()
 *   LCD_SDI_SetHigh() / LCD_SDI_SetLow()
 *
 * Protocol per SOC1602A datasheet pg 13:
 *   /CS low -> start frame
 *   10 bits MSB first: RS, RW, D7..D0
 *   Each bit: set SDI, pulse SCL high then low (display samples on rising edge)
 *   /CS high -> end frame
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/pin_manager.h"
#include "bb_oled.h"

/* Short delay between pin transitions -- ~5us at 1.84MHz */
static inline void tiny_delay(void)
{
    Nop(); Nop(); Nop(); Nop(); Nop();
    Nop(); Nop(); Nop(); Nop(); Nop();
}

/* ----------------------------------------------------------- */
/* Send one 10-bit frame: RS, RW=0, D7..D0                     */
static void bb_send(unsigned int rs, unsigned int byte)
{
    unsigned int frame;
    int i;

    /* Pack: RS in bit 9, RW=0 in bit 8, byte in bits 7..0 */
    frame = ((rs & 1u) << 9) | (byte & 0xFFu);

    /* Start transaction */
    LCD_SCL_SetLow();
    LCD_CS_SetLow();
    tiny_delay();

    /* Shift out 10 bits, MSB first */
    for (i = 9; i >= 0; i--) {
        if ((frame >> i) & 1u)
            LCD_SDI_SetHigh();
        else
            LCD_SDI_SetLow();
        tiny_delay();

        LCD_SCL_SetHigh();
        tiny_delay();

        LCD_SCL_SetLow();
        tiny_delay();
    }

    /* End transaction */
    LCD_CS_SetHigh();
    LCD_SDI_SetLow();
    tiny_delay();
}

/* ----------------------------------------------------------- */
void bb_cmd(unsigned int b)
{
    bb_send(0u, b);
    __delay_us(600);
}

void bb_data(unsigned int b)
{
    bb_send(1u, b);
    __delay_us(600);
}

/* ----------------------------------------------------------- */
void bb_oled_init(void)
{
    /* Ensure idle state before anything else */
    LCD_CS_SetHigh();
    LCD_SCL_SetLow();
    LCD_SDI_SetLow();

    __delay_ms(100);            /* power stabilization */

    bb_cmd(0x38);               /* Function Set: 8-bit, 2-line, FT=00 */
    bb_cmd(0x08);               /* Display OFF */
    bb_cmd(0x01);               /* Clear */
    __delay_ms(3);
    bb_cmd(0x06);               /* Entry Mode: increment */
    bb_cmd(0x02);               /* Home */
    __delay_ms(2);
    bb_cmd(0x0C);               /* Display ON */

    __delay_ms(10);
}

/* ----------------------------------------------------------- */
void bb_write_line1(const char *s)
{
    bb_cmd(0x02);               /* Home -> row 1, col 0 */
    __delay_ms(2);
    while (*s) bb_data((unsigned char)*s++);
}

void bb_write_line2(const char *s)
{
    bb_cmd(0xC0);               /* DDRAM 0x40 -> row 2, col 0 */
    while (*s) bb_data((unsigned char)*s++);
}
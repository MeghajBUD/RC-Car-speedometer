/*
 * main.c -- SOC1602A OLED via hardware SPI1
 *
 * Pins (same wiring as bit-bang version):
 *   RB0 (PIC pin 4)  -> display pin 16 (/CS)   GPIO, manual
 *   RB7 (PIC pin 16) -> display pin 12 (SCL)   SCK1 via PPS
 *   RB8 (PIC pin 17) -> display pin 14 (SDI)   SDO1 via PPS
 *
 * Frame packing (since PIC24 SPI is 16-bit, not 10-bit):
 *   bit 15: RS
 *   bit 14: R/W (always 0)
 *   bits 13..6: D7..D0
 *   bits 5..0: don't-care (zeros; ignored because /CS rises)
 *
 * This is equivalent to: left-shift the (RS<<9 | data) 10-bit value by 6.
 *
 * MCC SPI1 settings needed:
 *   Master, 16-bit width, CKP=0 (idle low), CKE=1 (Active to Idle),
 *   SMP=1 (sample at End), SSEN disabled.
 *   Baud: anything below ~3 MHz. At Fcy=1.84 MHz you can't exceed ~920 kHz
 *   anyway, so default prescalers are fine.
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include <stdint.h>
#include "mcc_generated_files/system.h"

#define CS_HIGH()   (LATBbits.LATB0 = 1)
#define CS_LOW()    (LATBbits.LATB0 = 0)

/* ------------------------------------------------------------------ */
/* SPI1 setup. Done in code rather than relying on MCC so the SPI
 * config is visible and explicit.                                    */
static void spi1_init(void)
{
    /* Turn SPI off during reconfiguration */
    SPI1STATbits.SPIEN = 0;

    /* Unlock PPS, map SPI1 output signals to RB7/RB8 */
    __builtin_write_OSCCONL(OSCCON & 0xBF);
   
    __builtin_write_OSCCONL(OSCCON | 0x40);

    /* SPI1CON1:
     *   MODE16 = 1    -> 16-bit
     *   MSTEN  = 1    -> master
     *   CKP    = 0    -> idle low
     *   CKE    = 1    -> data changes on falling edge (stable on rising)
     *   SMP    = 1    -> sample at end (master mode)
     *   SSEN   = 0    -> we drive /CS manually
     *   DISSDO = 0, DISSCK = 0
     *   PPRE   = 0b10 (4:1 primary prescaler)
     *   SPRE   = 0b110 (2:1 secondary prescaler)  -> Fcy / 8
     */
    SPI1CON1 = 0;
    SPI1CON1bits.MSTEN = 1;
    SPI1CON1bits.MODE16 = 1;
    SPI1CON1bits.CKP = 0;
    SPI1CON1bits.CKE = 1;
    SPI1CON1bits.SMP = 1;
    SPI1CON1bits.PPRE = 0b10;       /* 4:1 */
    SPI1CON1bits.SPRE = 0b110;      /* 2:1 */

    SPI1CON2 = 0;                   /* no framed mode, no enhanced buffer */
    SPI1STAT = 0;
    SPI1STATbits.SPIEN = 1;         /* enable */
}

/* Send one 16-bit word, wait for completion, drain RX */
static void spi1_xfer16(uint16_t w)
{
    volatile uint16_t dummy;

    /* Drain any stale RX */
    while (SPI1STATbits.SPIRBF) { dummy = SPI1BUF; (void)dummy; }

    SPI1BUF = w;
    while (!SPI1STATbits.SPIRBF) { /* wait for TX to complete */ }
    dummy = SPI1BUF;
    (void)dummy;
}

/* ------------------------------------------------------------------ */
static void oled_pins_configure(void)
{
    /* RB0 -- keep as GPIO output for manual /CS */
    ANSELBbits.ANSB0 = 0;
    TRISBbits.TRISB0 = 0;
    CS_HIGH();

    /* RB7, RB8 are driven by SPI1 via PPS; TRIS should still be output */
    TRISBbits.TRISB7 = 0;
    TRISBbits.TRISB8 = 0;
}

/* ------------------------------------------------------------------ */
/* Send one frame via SPI:
 *   packed = (RS<<9) | (RW<<8) | data   (10 bits)
 *   shifted to occupy bits 15..6 of the 16-bit SPI frame
 */
static void oled_send(uint16_t packed10)
{
    uint16_t frame = (uint16_t)(packed10 << 6);

    CS_LOW();
    __delay_us(1);
    spi1_xfer16(frame);
    __delay_us(1);
    CS_HIGH();
    __delay_us(100);
}

#define oled_cmd(c)   oled_send((uint16_t)((c) & 0xFF))               /* RS=0 */
#define oled_dat(c)   oled_send(0x200 | ((uint16_t)((c) & 0xFF)))     /* RS=1 */

/* ------------------------------------------------------------------ */
static void oled_init(void)
{
    oled_pins_configure();
    spi1_init();
    __delay_ms(100);

    oled_cmd(0x38); __delay_us(600);   /* Function Set: 8-bit, 2-line */
    oled_cmd(0x08); __delay_us(600);   /* Display OFF */
    oled_cmd(0x01); __delay_ms(3);     /* Clear */
    oled_cmd(0x06); __delay_us(600);   /* Entry Mode: increment */
    oled_cmd(0x02); __delay_ms(2);     /* Home */
    oled_cmd(0x0C); __delay_ms(5);     /* Display ON */
}

static void oled_set_cursor(uint8_t line, uint8_t col)
{
    uint8_t addr = (line ? 0x40 : 0x00) + (col & 0x0F);
    oled_cmd(0x80 | addr);
    __delay_us(600);
}

static void oled_print(const char *s)
{
    while (*s) oled_dat(*s++);
}

/* ------------------------------------------------------------------ */
int main(void)
{
    SYSTEM_Initialize();
    oled_init();

    oled_set_cursor(0, 0);
    oled_print("Hello Meghaj!");
    oled_set_cursor(1, 0);
    oled_print("SPI hardware OK");
    oled_set_cursor(1, 0);
    oled_print("Sorru for the trobles");
    while (1) { Nop(); }
    return 0;
}
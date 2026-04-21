/*
 * spi_oled.h -- SOC1602A serial-mode driver for PIC24EP128GP202
 * Mirrors the STM32 lab structure: spi_cmd, spi_data, spi1_init_oled,
 * spi1_display1, spi1_display2.
 *
 * Differences from STM32 lab:
 *   - PIC24EP SPI is 16-bit, not 10-bit. We left-shift the 8-bit payload
 *     by 6 so RS/RW end up in bits 15/14, and the trailing 6 bits are
 *     ignored by the display controller.
 *   - /CS is toggled manually as a GPIO (LCD_CS), since we need it HIGH
 *     between command and data writes (datasheet pg. 13).
 */

#ifndef SPI_OLED_H
#define SPI_OLED_H

void init_spi1(void);             /* configures SPI1 (if not done by MCC) */
void spi_cmd(unsigned int data);  /* RS=0, RW=0 : send command byte */
void spi_data(unsigned int data); /* RS=1, RW=0 : send data byte    */
void spi1_init_oled(void);        /* full init sequence */
void spi1_display1(const char *s);/* write string starting at row 1, col 0 */
void spi1_display2(const char *s);/* write string starting at row 2, col 0 */

#endif

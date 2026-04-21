/**
 * oled.h
 * SSD1306 128x64 OLED Driver (4-wire SPI, Software Bit-Bang)
 * Target: PIC24EP128GP202 (MPLAB X / XC16)
 *
 * Pin Mapping (from MCC pin_manager.h):
 *   CS  (Chip Select)  -> RB0  (SS1_SetHigh / SS1_SetLow)
 *   SCK (Clock)        -> RB7  (SCK1_SetHigh / SCK1_SetLow)
 *   SDA/MOSI (Data)    -> RB8  (SDO1_SetHigh / SDO1_SetLow)
 *   DC  (Data/Command) -> RB10 (IO_RB10_SetHigh / IO_RB10_SetLow)
 *   RST (Reset)        -> RA0  (IO_RA0_SetHigh / IO_RA0_SetLow)
 */

#ifndef OLED_H
#define OLED_H

#include <xc.h>
#include <stdint.h>

/* ---------- Display geometry ---------- */
#define OLED_WIDTH      128
#define OLED_HEIGHT      64
#define OLED_PAGES        8   // 64 px / 8 px per page

/* ---------- SSD1306 Command bytes ---------- */
#define OLED_CMD_DISPLAY_OFF        0xAE
#define OLED_CMD_DISPLAY_ON         0xAF
#define OLED_CMD_SET_CONTRAST       0x81
#define OLED_CMD_ENTIRE_DISPLAY_ON  0xA4
#define OLED_CMD_NORMAL_DISPLAY     0xA6
#define OLED_CMD_SET_MEM_ADDR_MODE  0x20
#define OLED_CMD_SET_PAGE_ADDR      0xB0   // OR with page number (0-7)
#define OLED_CMD_SET_COL_LOW        0x00   // OR with lower nibble
#define OLED_CMD_SET_COL_HIGH       0x10   // OR with upper nibble
#define OLED_CMD_SET_DISP_OFFSET    0xD3
#define OLED_CMD_SET_DISP_START_LINE 0x40
#define OLED_CMD_SET_SEG_REMAP      0xA1
#define OLED_CMD_SET_MUX_RATIO      0xA8
#define OLED_CMD_COM_SCAN_DEC       0xC8
#define OLED_CMD_SET_COM_PINS       0xDA
#define OLED_CMD_SET_DISP_CLK_DIV   0xD5
#define OLED_CMD_SET_PRECHARGE      0xD9
#define OLED_CMD_SET_VCOM_DETECT    0xDB
#define OLED_CMD_CHARGE_PUMP        0x8D

/* ---------- Public API ---------- */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawChar(uint8_t page, uint8_t col, char c);
void OLED_DrawString(uint8_t page, uint8_t col, const char *str);

#endif /* OLED_H */

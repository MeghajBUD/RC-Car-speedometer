/*
 * main.c -- bit-bang SOC1602A test
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include "mcc_generated_files/system.h"
#include "bb_oled.h"

int main(void)
{
    SYSTEM_Initialize();

    bb_oled_init();

    bb_write_line1("Hello Meghaj!");
    bb_write_line2("PIC24 bit-bang");

    while (1) { Nop(); }
    return 0;
}
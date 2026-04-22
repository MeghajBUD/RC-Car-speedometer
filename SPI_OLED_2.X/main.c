/*
 * main.c -- example: update OLED with a changing value
 */

#ifndef FCY
#define FCY 1840000UL
#endif

#include <xc.h>
#include <libpic30.h>
#include "mcc_generated_files/system.h"
#include "oled.h"

int main(void)
{
    SYSTEM_Initialize();
    oled_init();

    /* Static label on row 0 -- set once */
    oled_display1("Sensor Reading:");

    int value = 0;

    while (1) {
        /* --- replace this with your actual sensor read --- */
        value++;
        if (value > 9999) value = 0;
        /* ------------------------------------------------- */

        /* Update row 1 with current value -- no flicker, no oled_clear() */
        oled_printf2("Val: %d", value);

        __delay_ms(200);
    }

    return 0;
}
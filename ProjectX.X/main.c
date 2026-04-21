/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.171.5
        Device            :  PIC24EP128GP202
    The generated drivers are tested against the following:
        Compiler          :  XC16 v2.10
        MPLAB 	          :  MPLAB X v6.05
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/spi1.h"
#include "oled.h"
#include <stdint.h>
#define HALL_PIN    PORTBbits.RB15

// FOSC = 3,685,000 Hz, FOSC/2 = 1,842,500 Hz
// With 1:8 prescaler: effective tick rate = 1,842,500 / 8 = 230,312.5 Hz
#define TICK_RATE_HZ   230312.5f   // ticks per second at 1:8 prescaler

///* Simple blocking delay — counts down instruction cycles */
static void delay_ms(uint16_t ms)
{
    uint16_t i, j;
    /* At FCY ~1.84 MHz, ~1840 iterations ≈ 1 ms */
    for (i = 0; i < ms; i++)
        for (j = 0; j < 460; j++)
            Nop();
}

/*
                         Main application
 */
void spi_cmd(uint8_t data) {
    while (SPI1STATbits.SPITBF);  // Wait until TX buffer has room
    SPI1BUF = data;
}

void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}
void spi1_init_oled() {
    delay_ms(1);
    spi_cmd(0x38);  //function set
    spi_cmd(0x08);  //trun off display
    spi_cmd(0x01);  //clear display
    delay_ms(2);
    spi_cmd(0x06);  //set to entry mode
    spi_cmd(0x02);  //moving cursor to home position
    spi_cmd(0x0c);  //turn on display
}

void spi1_display1(const char *string) {
    spi_cmd(0x02);  //moving to home position
    while(*string != '\0'){
        spi_data((*string));
        string++;
    }
}
void spi1_display2(const char *string) {
    spi_cmd(0x0c);  //moving to the second row
    while(*string != '\0'){
        spi_data((*string));
        string++;
    }
}

int main(void)

{
    SYSTEM_Initialize();
    spi1_init_oled();
    spi1_display1("Hello again,");
    spi1_display2("Meghaj Kabra");

    while(1)
    {
        if (HALL_PIN == 0)
        {
            LATBbits.LATB6 = 1;
        }
        else
        {
            LATBbits.LATB6 = 0;
        }
    }

}
    
  





/**
 End of File
*/


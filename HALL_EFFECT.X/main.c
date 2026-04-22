#ifndef FCY
#define FCY 1840000UL
#endif
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/tmr1.h"
#include <xc.h>
#include <libpic30.h>
#include "mcc_generated_files/system.h"
#include "oled_copy.h"
#define HALL_PIN        PORTBbits.RB15
#define TICK_RATE_HZ    230312.5f

volatile uint16_t overflow_count = 0;



void get_velocity(uint16_t cycle_time)
{
    float period_seconds = 0.0f;
    float rpm = 0.0f;
    float velocity = 0.0f;
    uint32_t total_ticks = ((uint32_t)overflow_count << 16) + cycle_time;
    period_seconds = (float)total_ticks / TICK_RATE_HZ;
    rpm = (period_seconds > 0.0f) ? 60.0f / period_seconds : 0.0f;
    velocity = (2 * 3.14 * 0.01778 * rpm)/ 60.0;
    //printf("Period: %.4f s | RPM: %.2f\r\n | Velocity m/s : %.2f\r\n", period_seconds, rpm, velocity);
    oled_printf1("SP: %.1f\r\n", velocity);
    oled_printf2("RPM: %.1f\r\n", rpm);
    //add oled display code
}

void TMR1_OverflowCallback(void) {
    overflow_count++;
}

int main(void) {
    
    SYSTEM_Initialize();
    TMR1_Initialize();
    oled_init();    //oled initalize
    TMR1_SetInterruptHandler(TMR1_OverflowCallback);
    TMR1_Start();
        
    bool sensor_was_low = false;
    uint16_t current_cycle_time; 

    while (1) {
        if (HALL_PIN == 0) {
            // Magnet present ? stop and hold counter
            LATBbits.LATB6 = 1;
            TMR1_Stop();
            if (!sensor_was_low){
                
                current_cycle_time =TMR1_Counter16BitGet();
                get_velocity(current_cycle_time);
            }
            TMR1_Counter16BitSet(0);
            overflow_count = 0;
            TMR1_Start();
            sensor_was_low = true;
            

        } else {
            LATBbits.LATB6 = 0;
            sensor_was_low = false;
        }

        
    }

    return 0;
}
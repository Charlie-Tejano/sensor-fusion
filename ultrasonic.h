// ultrasonic.h

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "stm32f4xx.h"
#include <stdint.h>

// Constants
#define TIMCLK_HZ 16000000u
#define TICK_HZ 1000000u
#define PRESCALER ((TIMCLK_HZ / TICK_HZ) - 1u)
#define ULTRASONIC_ARR 59999u
#define TRIG_PULSEWIDTH 15u

// Autonomous Emergency Braking
typedef enum {
    aeb_off = 0,
    aeb_on,
    aeb_fault
} aeb_state;

// Function to set the state of the automatic emergency braking system
void aeb_set_state(aeb_state state, float dist_cm);

// Function prototypes
void  ultrasonicInit(void);
uint8_t ultrasonic_GetDistance_cm(float *output_cm);
void ultrasonic_resync(void);
void ultrasonic_TIM3(void);

#endif // ULTRASONIC_H
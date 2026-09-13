// ultrasonic.c

#include "ultrasonic.h"
#include "MOTOR.h"
#include "stm32f4xx.h"
#include <stdint.h>
    
// Pins
#define TRIG_PIN 4u
#define ECHO_PIN 5u

#define ULTRASONIC_LAPSE 4u
#define ULTRASONIC_MIN_US 116u
#define ULTRASONIC_MAX_US 23200u

// State variables
static volatile uint8_t s_edge_state = 0u;
static volatile uint32_t s_rise_tick = 0u;
static volatile uint32_t s_echo_tick = 0u;
static volatile uint8_t s_lapse = 0xFFu;
volatile float g_distance_cm = 0.0f;

// Ultrasonic initialization
void ultrasonicInit(void)
{
// Enable GPIOB clock
RCC->AHB1ENR |= (1u << 1);
	
    // PB4; Trigger output
    GPIOB->MODER &= ~(3u << (4u * 2u));
    GPIOB->MODER |= (2u << (4u * 2u));
	
    GPIOB->OTYPER &= ~(1u << 4u);
	
    GPIOB->PUPDR &= ~(3u << (4u * 2u));
	
    GPIOB->AFR[0] &= ~(0xFu << (4u * 4u));
    GPIOB->AFR[0] |=  (0x2u << (4u * 4u));

    // PB5; Echo input
    GPIOB->MODER &= ~(3u << (5u * 2u));
    GPIOB->MODER |= (2u << (5u * 2u));
	
    GPIOB->PUPDR &= ~(3u << (5u * 2u));
	GPIOB->PUPDR |= (2u << (5u * 2u)); // Pull-down (10)
	
    GPIOB->AFR[0] &= ~(0xFu << (5u * 4u));
    GPIOB->AFR[0] |= (2u << (5u * 4u));
		
		ultrasonic_TIM3();
}

// Initialize TIM3 for ultrasonic sensor
void ultrasonic_TIM3(void)
{
    RCC->APB1ENR |= (1u << 1);
	
    TIM3->CR1 = 0x0000u; // Clear first
	
    TIM3->PSC = PRESCALER;
    TIM3->ARR = ULTRASONIC_ARR;
		TIM3->CCR1 = TRIG_PULSEWIDTH;

    TIM3->CCMR1 &= ~(0x00FFu);
    TIM3->CCMR1 |= (6u << 4) | (1u << 3);
	
    TIM3->CCER &= ~(1u << 1);
    TIM3->CCER |= (1u << 0);
	
    TIM3->CCMR1 &= ~(0xFF00u);
    TIM3->CCMR1 |= (1u << 8u) | (0xFu << 12u);

    TIM3->CCER &= ~((1u << 5u) | (1u << 7u)); // CC2P=0, CC2NP=0
    TIM3->CCER |= (1u << 4u); // CC2E

    TIM3->CR1 |= (1u << 7u); // ARPE
	TIM3->CR1 |= (1u << 2);

    TIM3->SR  = 0x0000u;
    TIM3->EGR = (1u << 0u); // UG
    TIM3->SR  = 0x0000u;

    TIM3->DIER |= (1u << 0) | (1u << 2u); // CC2IE

    NVIC_SetPriority(TIM3_IRQn, 1u);
    NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1 |= (1u << 0u); // Enable TIM3
}

uint8_t ultrasonic_GetDistance_cm(float *output_cm)
{
    uint32_t us; // microseconds
    uint8_t lapse;

    NVIC_DisableIRQ(TIM3_IRQn); // Disable interrupt to read shared variable
    us = s_echo_tick;
    lapse = s_lapse;
    NVIC_EnableIRQ(TIM3_IRQn);

    if (lapse > ULTRASONIC_LAPSE) { // If the echo pulse is too old, return 0
        return 0u;
    }

    *output_cm = (float)us * 0.017241379f;
	    return 1u;
	}

// Resynchronize capture & compare to ensure proper edge detection
void ultrasonic_resync(void)
{
    TIM3->CCER &= ~((1u << 5u) | (1u << 7u));
    (void)TIM3->CCR2;
    TIM3->SR = ~((1u << 2u) | (1u << 10u));
    s_edge_state = 0u;
}

void TIM3_IRQHandler(void)
{
    uint32_t sr = TIM3->SR; // snapshot of status register

    if ((sr & (1u << 0u)) != 0u) { // Clear snapshot of UIF bit
        TIM3->SR = ~(1u << 0u);

        if (s_lapse < 0xFFu) { 
            s_lapse++; 
        }

        if (s_edge_state != 0u) { // Echo rise was detected but no fall yet
            ultrasonic_resync();
            return;
        }
    }

    if ((sr & (1u << 10u)) != 0u) {
        ultrasonic_resync();
        return;
    }

    // Echo capture event
    if ((sr & (1u << 2u)) != 0u) {
        uint32_t tick = TIM3->CCR2;

        if (s_edge_state == 0u) {
            s_rise_tick = tick;
            TIM3->SR = ~(1u << 2u);
            TIM3->CCER = (TIM3->CCER & ~(1u << 7u)) | (1u << 5u);
            s_edge_state = 1u;
        } else {
            uint32_t width = (tick >= s_rise_tick)
                           ? (tick - s_rise_tick)
                           : ((ULTRASONIC_ARR + 1u) - s_rise_tick + tick);

            if ((width >= ULTRASONIC_MIN_US) && (width <= ULTRASONIC_MAX_US)) {
                s_echo_tick = width;
                s_lapse = 0u;
            }
            ultrasonic_resync();
        }
    }
}

// AEB states
void aeb_set_state(aeb_state state, float dist_cm) {
	(void)dist_cm;
	
    if (state == aeb_on) {
            MOTOR_Stop();
        } else if (state == aeb_off) {
						MOTOR_Forward();
				} else if (state == aeb_fault) {
						MOTOR_Stop();
				}
			}
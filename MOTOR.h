// MOTOR.h

#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f4xx.h"
#include <stdint.h>

// Motor state definitions
typedef enum {
    MOTOR_STOPPED = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} MotorState_t;

// Function prototypes
void MOTOR_Init(void);
void MOTOR_SetDutyCycle(uint8_t duty_percent);
uint8_t MOTOR_GetDutyCycle(void);
void MOTOR_Forward(void);
void MOTOR_Backward(void);
void MOTOR_Stop(void);
void MOTOR_EStop(void);
void MOTOR_SetState(MotorState_t state);
MotorState_t MOTOR_GetState(void);

// Optional helpers
void MOTOR_IncreaseDuty_5(void);
void MOTOR_DecreaseDuty_5(void);

#endif // MOTOR_H
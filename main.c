// main.c
/***
 * 
 */
#include "LCD.h"
#include "stm32f4xx.h"
#include "MOTOR.h"
#include "ultrasonic.h"
#include "lis3dsh.h"
#include "RPG.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// Autonomous Emergency Brake
#define AEB_STOP 10.0f
#define AEB_RESUME 15.0f
#define NO_ECHO 401.0f

// Tilt Detection
#define TILT_FLAT 0.05f

// ESC, Hill-descent
#define hill_descent 5u

// AEB stop flag
static uint8_t s_aeb_stopped = 0u;

// Classify tilt result
typedef enum {
	terrain_flat = 0,
	terrain_incline,
	terrain_decline
} terrain_t;

// Detect flat surface, incline, and decline based on pitch angle
static terrain_t classify_pitch(float x_dir) 
	{
	if (x_dir > TILT_FLAT) {
	return terrain_incline;
	}
	
	if (x_dir < -TILT_FLAT) {
	return terrain_decline;
	}
	return terrain_flat;
}

// Text for Display Only
static const char *terrain_str(terrain_t t) 
	{
	switch (t) {
		case terrain_incline : return "Incline";
		case terrain_decline : return "Decline";
		default : return "Flat";
	}
	
}
// Distance, duty mapping
static uint8_t DIST_to_duty(float dist_cm)
{
    // Measure distance and map to duty cycle
    if (dist_cm < 30.0f) {
        return 40u;
    }
    if (dist_cm < 60.0f) {
        return 60u;
    } if (dist_cm < 401.0f) {
				return 80u;
		}
    return 100u;
}

// Display distance and duty cycle on LCD
static void LCD_showUltrasonic(float dist_cm, uint8_t duty, const char *terrain)
{
    char line1[17];
    char line2[17];
	snprintf(line1, sizeof(line1), "D:%5.1fcm S:%3u%%", dist_cm, (unsigned)duty);
	snprintf(line2, sizeof(line2), "TERRAIN: %-7s", terrain);
	
    LCD_placeCursor(1);
    LCD_printString(line1);
    LCD_placeCursor(2);
    LCD_printString(line2);
}

// Main function
int main(void)
{
    LCD_port_init();
    LCD_init();
    lis3dsh_init();
    MOTOR_Init();
    ultrasonicInit();
    rotary_Init();
    MOTOR_Stop();
    MOTOR_SetDutyCycle(40u);
    MOTOR_Forward();

    while (1) {
        // Use e-stop flag to indicate a fault condition
        if (g_estop_flag) {
            aeb_set_state(aeb_fault, 0.0f);
            LCD_placeCursor(1);
            LCD_printString("AEB: Fault      ");
            LCD_placeCursor(2);
            LCD_printString("ESTOP...        ");
            delay_ms(25);
            continue;
        }

        // Read distance from ultrasonic sensor
        float dist_cm = 0.0f;
        if (ultrasonic_GetDistance_cm(&dist_cm) == 0u) {
            dist_cm = NO_ECHO;
        }

        // Nothing detected within max range
        if (dist_cm >= NO_ECHO) {
            aeb_set_state(aeb_fault, dist_cm);
            LCD_placeCursor(1);
            LCD_printString("AEB: Fault      ");
            LCD_placeCursor(2);
            LCD_printString("Out of bounds   "); 
            delay_ms(15);
            continue;
        }

        // Hysteresis, separate stop and resume thresholds
        if (!s_aeb_stopped && dist_cm < AEB_STOP) {
            s_aeb_stopped = 1u;
        } else if (s_aeb_stopped && dist_cm > AEB_RESUME) {
            s_aeb_stopped = 0u;
        }

        if (s_aeb_stopped) {
            aeb_set_state(aeb_on, dist_cm);
            LCD_placeCursor(1);
            LCD_printString("AEB: Initiated  ");
            LCD_placeCursor(2);
            LCD_printString("DIST: <10cm     ");
            delay_ms(15);
            continue;
        }
        aeb_set_state(aeb_off, dist_cm);

				// Read tilt
				float x_dir = 0.0f, y_dir = 0.0f, z_dir = 0.0f;
				lis3dsh_read_dir(&x_dir, &y_dir, &z_dir);
				terrain_t terrain = classify_pitch(x_dir);
	
        // Map distance to duty cycle and update motor speed
        uint8_t duty = DIST_to_duty(dist_cm);
				if (terrain == terrain_decline) {
					duty = (duty > 5u) ? (duty - 5u) : 0u;
				}
        MOTOR_SetDutyCycle(duty);
        LCD_showUltrasonic(dist_cm, duty, terrain_str(terrain));
    }
}
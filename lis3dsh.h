// lis3dsh.h

#ifndef LIS3DSH_H
#define LIS3DSH_H

#include <stdint.h>
#include "stm32f4xx.h"

// g-direction helper
typedef enum {
    DIR_NONE = 0,
    DIR_POSITIVE_X,
    DIR_NEGATIVE_X,
    DIR_POSITIVE_Y,
    DIR_NEGATIVE_Y,
    DIR_POSITIVE_Z,
    DIR_NEGATIVE_Z
} LIS3DSH_Direction;

// Returns the direction of the g-force detected from the accelerometer
LIS3DSH_Direction lis3dsh_dir(void);

// Prototypes
void lis3dsh_init(void);
void lis3dsh_read_dir(float *x_dir, float *y_dir, float *z_dir);

// Return 0x3F if the LIS3DSH is connected and responding, otherwise return 0x00
uint8_t lis3dsh_whoami(void);

#endif // LIS3DSH_H
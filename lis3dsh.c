// lis3dsh.c

#include "lis3dsh.h"
#include "stm32f4xx.h"
#include <stdint.h>

// LIS3DSH Register Addresses
#define CTRL_REG4 0x20u
#define CTRL_REG5 0x24u
#define OUT_X_L   0x28u
#define OUT_X_H   0x29u
#define OUT_Y_L   0x2Au
#define OUT_Y_H   0x2Bu
#define OUT_Z_L   0x2Cu
#define OUT_Z_H   0x2Du

// SPI1 Command, read and write
#define SPI_READ  0x80u

// Block Data Update bit in Control Register 4
#define BDU_BIT (1u << 3)

/***LIS3DSH Sensitivity for acceleration in g/LSB \n
 **conversion factor for 6-bit data, 0.06 mg/LSB = 0.00006 g/LSB
*/
#define SPI_SENSITIVITY (0.06f / 1000.0f)

// PE3 - Chip-Select for the LIS3DSH
static void cs_low(void)
{
    GPIOE->BSRR = (1u << (3u + 16u)); // Set PE3 low
}

// Set PE3
static void cs_high(void)
{
    GPIOE->BSRR = (1u << 3u); // Set PE3 high
}

// 8-bit SPI transfer
static uint8_t spi1_transfer(uint8_t data)
{
    while (!(SPI1->SR & (1u << 1))) {} // TXE transmit, wait until empty
    SPI1->DR = data;

    while (!(SPI1->SR & (1u << 0))) {} // Wait RXNE receive not empty
    return (uint8_t)SPI1->DR; // Distribute received data
}

// Write a byte to the LIS3DSH register
static void lis3dsh_write(uint8_t reg, uint8_t data)
{
    cs_low();
    spi1_transfer(reg & 0x7Fu); // Clear the R/W bit for write operation
    spi1_transfer(data);
    cs_high();
}

// Read a byte from the LIS3DSH register
static uint8_t lis3dsh_read(uint8_t reg)
{
    uint8_t data;
    cs_low();
    spi1_transfer(reg | SPI_READ); // Set the R/W bit for read operation
    data = spi1_transfer(0xFFu); // Send dummy byte to receive data
    cs_high();
    return data;
}

uint8_t lis3dsh_whoami(void)
{
    return lis3dsh_read(0x0Fu); // WHO_AM_I register address
}

// Initialize the LIS3DSH accelerometer
void lis3dsh_init(void)
{
RCC->AHB1ENR |= (1u << 0) | (1u << 4);
RCC->APB2ENR |= (1u << 12);

GPIOA->MODER &= ~((3u << (5*2)) 
                | (3u << (6*2)) 
                | (3u << (7*2)));

GPIOA->MODER |= ((2u << (5*2)) 
                | (2u << (6*2)) 
                | (2u << (7*2))); // Alternate function mode (10)

GPIOA->OTYPER &= ~((1u << 5) 
                | (1u << 6) 
                | (1u << 7)); // Set to push-pull (00)

GPIOA->PUPDR &= ~((3u << (5*2)) 
                | (3u << (6*2)) 
                | (3u << (7*2))); // No pull-up, no pull-down

GPIOA->OSPEEDR |= ((3u << (5*2)) 
                | (3u << (6*2)) 
                | (3u << (7*2))); // High speed (11)

GPIOA->AFR[0] &= ~((0xFu << (5u*4u)) 
                | (0xFu << (6u*4u)) 
                | (0xFu << (7u*4u))); // Clear AFRL for PA5, PA6, PA7

GPIOA->AFR[0] |= ((5u << (5u*4u)) 
                | (5u << (6u*4u)) 
                | (5u << (7u*4u))); // AF5 for SPI1

// Config. PE3 for Chip Select (CS) pin
GPIOE->MODER &= ~(3u << (3u*2u));
GPIOE->MODER |= (1u << (3u*2u)); // Output mode (01)

GPIOE->OTYPER &= ~(1u << 3u); // Not open-drain

GPIOE->PUPDR &= ~(3u << (3u*2u)); // No pull-up, no pull-down
cs_high(); // Set CS high

// Configure SPI1
SPI1->CR1 = 0x0000u;
SPI1->CR1 |= (1u << 2) | (1u << 1) | (1u << 0); // Master mode, clock-idle high, 8-bit data
SPI1->CR1 |= (1u << 9) | (1u << 8);

/***
 ** APB2 clock freq. = 84 MHz, SPI1 baud rate\n
 * SPI1 baud rate = 84 MHz / 64 MHz = ~1.3125 MHz
 */
SPI1->CR1 |= (5u << 3);
SPI1->CR1 |= (1u << 6); // Enable SPI1

// Configure LIS3DSH
lis3dsh_write(CTRL_REG4, 0x67u | BDU_BIT); // Enable Block Data Update, 100 Hz data rate
lis3dsh_write(CTRL_REG5, 0x00u);
}

// Read and register values of each axis
void lis3dsh_read_dir(float *x_dir, float *y_dir, float *z_dir)
{
    int16_t raw_x = (int16_t)(lis3dsh_read(OUT_X_H) << 8 
                            | lis3dsh_read(OUT_X_L)); // Read X-axis data

    int16_t raw_y = (int16_t)(lis3dsh_read(OUT_Y_H) << 8 
                            | lis3dsh_read(OUT_Y_L)); // Read Y-axis data

    int16_t raw_z = (int16_t)(lis3dsh_read(OUT_Z_H) << 8 
                            | lis3dsh_read(OUT_Z_L)); // Read Z-axis data

    *x_dir = (float)raw_x*SPI_SENSITIVITY;
    *y_dir = (float)raw_y*SPI_SENSITIVITY;
    *z_dir = (float)raw_z*SPI_SENSITIVITY;
}
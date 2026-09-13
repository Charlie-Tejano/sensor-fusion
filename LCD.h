//LCD.h

#ifndef LCD_H
#define LCD_H

#include "stm32f4xx.h"

/*******************************
 * LCD pins connections to PORTD
 *******************************
 */

#define RS 7 
#define RW 6 
#define EN 5 
#define DB7 3 
#define DB6 2 
#define DB5 1 
#define DB4 0 

/*******************************
 * LCD Command Definitions
 *******************************
 */
#define LCD_CLEAR_DISPLAY 0x01
#define LCD_RETURN_HOME 0x02
#define LCD_ENTRY_MODE 0x06 // Increment, no shift
#define LCD_DISPLAY_ON 0x0F // Display ON, Cursor ON, Blink ON
#define LCD_DISPLAY_OFF 0x08
#define LCD_FUNCTION_SET 0x28 // 4-bit, 2 lines, 5x8 font
#define LCD_LINE1_ADDR 0x80 // DDRAM address for line 1, position 1
#define LCD_LINE2_ADDR 0xC0 


//Function prototypes
void LCD_port_init(void);
void LCD_init(void);
void LCD_placeCursor(uint32_t lineno);
void LCD_sendData(unsigned char data);
void LCD_sendInstr(unsigned char Instruction);
void LCD_clearDisplay(void);
void delay_ms(uint32_t ms);

//Additional functions for various data types
void LCD_printChar(char c);
void LCD_printString(char text[]);
void LCD_printInt(int number);
void LCD_printFloat(float number, int decimal_places);

//Helper Functions
void clear_PIN(int PINNO);
void set_PIN(int PINNO);
void check_BF(void);


#endif // LCD_H
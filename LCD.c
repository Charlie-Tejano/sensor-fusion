// LCD.c

#include "LCD.h"
#include "stm32f4xx.h"
#include <stdio.h>

void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us*16; i++); //16MHz clock
}

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms*16000; i++);
}

void LCD_port_init(){
//STEP 1: Enable GPIOD in RCC AHB1ENR register
RCC->AHB1ENR |= (1u<<3);

//STEP 2: Set MODER of GPIOD Pins 7, 6, 5, 3, 2, 1 & 0 as outputs
GPIOD->MODER &= ~(0xFCFF);
GPIOD->MODER |= (0x5455); // Set to 01

//STEP 3: Set OTYPER of GPIOD Pins 7, 6, 5, 3, 2, 1 & 0 as push-pull
GPIOD->OTYPER &= ~(0xEF);

//Done with LCD port Initialization
}

/*******************************
 * LCD_init()
 * Inputs: NONE
 * Outputs: NONE
 * LCD Initialization
 * Read the manual carefully
 * We are doing INITIALIZATION BY INSTRUCTION
 * Don't rush it.
 *******************************
 */

void LCD_init(){

// STEP 1: Wait for 100ms for power-on-reset to take effect
delay_us(50); //Should wait less than that 

// OK - nothing needs to be done here. 
delay_us(50000);

// Keep the board powered. By the time your code is downloaded
// to flash and you are ready to start execution using the 
// debugger - 100ms will have passed

// STEP 2: Set RS pin LOW to send instructions
clear_PIN(RS); 
clear_PIN(RW);  

//Send 0x03 three times to ensure 8-bit mode sync before switching to 4-bit
for (int i = 0; i < 3; i++) {
    set_PIN(EN);
    GPIOD->ODR = (GPIOD->ODR & ~0x0F)|0x03;
    clear_PIN(EN);
    delay_ms(5);
}

// Send instructions using following format:
// Check Busy Falg; Set EN=HIGH; Send 4-bit instruction; Set EN=low;

// STEP 3a-3d: Set 4-bit mode (takes a total of 4 steps)
set_PIN(EN); //Step 3a
GPIOD->ODR = (GPIOD->ODR & ~0x0F) | 0x02;
clear_PIN(EN);
delay_ms(2); //wait for more than 4.1ms

//Step 3c
clear_PIN(EN); // enable low
clear_PIN(RW); //read and write low

set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F) | 0x02; //upper nibble
clear_PIN(EN);

set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F) | 0x08; //lower nibble
clear_PIN(EN);
delay_ms(2);

// STEP 4: Set 2 line display -- treats 16 char as 2 lines
LCD_sendInstr(0x28);

// 001DL NF** (DL 0: 4bits; N= 1: 2 lines; F=0 : 5x8 display

// STEP 5: Set DISPLAY to OFF
LCD_sendInstr(0x08);

// STEP 6: CLEAR DISPLAY
LCD_clearDisplay();

// STEP 7: SET ENTRY MODE - Auto increment; no scrolling
LCD_sendInstr(0x06);

// STEP 8: Set Display to ON with Cursor and Blink.
LCD_sendInstr(0x0F);
}

/*******************************
 * LCD_placeCursor()
 * Inputs: unsigned integer linenumber
 * Outputs: NONE
 * sets Cursor position to
 * Line 1, character 1 (hex address 0x80)
 * or Line 2, character 1 (hex addres 0xC0)
 *
 *******************************
 */

 //DDRAM line 1 starts at 0x00, Line 2 starts at 0x40
void LCD_placeCursor(uint32_t lineno){
    if (lineno == 1) {
        LCD_sendInstr(0x80 | 0x00);
    } else if (lineno == 2) {
        LCD_sendInstr(0x80 | 0x40);
    }
}

/*******************************
 * LCD_sendData()
 * Inputs: unsigned character data (8-bit)
 * Outputs: NONE
 * writes the character to LCD.
 * Since we are using 4-bit mode
 * this function will take the character (8-bit)
 * transmit upper 4 bits and then lower 4 bits.
 * make sure the RS, RW and EN signals are set to correct value
 * for each 4-bit. 
 * also make sure to check the BF
 *******************************
 */

void LCD_sendData(unsigned char data)
{
check_BF(); 

set_PIN(RS);   
clear_PIN(RW); //Write mode

//Send upper 4-bit
set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F) | ((data>>4) & 0x0F);
clear_PIN(EN);

//Send lower 4-bit
set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F) | (data & 0x0F); 
clear_PIN(EN);
}

/*******************************
 * LCD_sendInstr()
 * Inputs: unsigned character INSTRUCTION (8-bit)
 * Outputs: NONE
 * Sends commands to LCD
 * We are using 4-bit mode but 
 * this function accepts (8-bit) character
 * as input. You can make the call on how to handle that.
 * make sure the RS, RW and EN signals are set to correct value
 * for each 4-bit part of instructions. 
 * also make sure to check the BF
 *******************************
 */

void LCD_sendInstr(unsigned char Instruction)
{
check_BF(); 

clear_PIN(RS); //Instruction register
clear_PIN(RW); //write

//Send upper 4-bits first
set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F)|(Instruction>>4); //Upper nibble
clear_PIN(EN); 

//Send lower 4-bits
set_PIN(EN);
GPIOD->ODR = (GPIOD->ODR & ~0x0F)|(Instruction & 0x0F); 
clear_PIN(EN);
}


/*******************************
 * LCD_clearDisplay()
 * Inputs: NONE
 * Outputs: NONE
 * Function to erase everything and 
 * clear LCD display
 *******************************
 */
void LCD_clearDisplay() {
//clear LCD Display -- 0x01
LCD_sendInstr(0x01); 

}


/*******************************
 * clear_PIN()
 * Inputs: an integer PIN NUMBER (e.g. RW, EN)
 * Outputs: NONE
 * CLEARS PIN in GPIOD to 0
 * Read the Reference manual carefully
 * you can use the BSRR register without masks
 * OR you can use the ODR register WITH &~ (AND-NOT) mask 
 * to clear ONE specified pin.
 *******************************
 */
void clear_PIN(int PINNO){
    //BSRR register to clear pin
    GPIOD->BSRR = (1u<<(PINNO+16));
    
}

/*******************************
 * set_PIN()
 * Inputs: an integer PIN NUMBER (e.g. RW, EN)
 * Outputs: NONE
 * SETS PIN in GPIOD to 1
 * Read the Reference manual carefully
 * you can use the BSRR register without masks
 * OR you can use the ODR register WITH | (OR) mask 
 * to SET ONE specified pin.
 *******************************
 */
void set_PIN(int PINNO){
GPIOD->BSRR = (1u<<PINNO);
    
}

/*******************************
 * check_BF()
 * Inputs: NONE
 * Outputs: NONE
 * Checks BF flag on DB7 pin of LCD
 * and prevents code from moving ahead
 * if the BF flag is 1 (indicating LCD busy)
 *******************************
 */

void check_BF(){
    // STEP 1: Clear RS (set RS=0) as reading flag is an instruction
    clear_PIN(RS);

    // STEP 2: set Data Pin 7 connected to GPIOD Pin 3 as input 
    // (no pull-up or pull down setup needed here)
    GPIOD->MODER &= ~(3u<<(DB7*2));

    // STEP 3: Set RW = 1 to read the BF flag.
    set_PIN(RW);

	int is_busy = 1;
	while (is_busy) {

        //Read upper nibble
		set_PIN(EN);
		is_busy = (GPIOD->IDR & (1u<<DB7));

		// STEP 4: Set EN = 1
		clear_PIN(EN);

        //Read lower nibble
        set_PIN(EN);
        __NOP(); // small delay
        clear_PIN(EN);
	}
    clear_PIN(RW);
	GPIOD->MODER |= (1u<<(DB7*2));
  }
//Print characters to LCD
void LCD_printChar(char c) {
    LCD_sendData(c);
}

//Print strings to LCD
void LCD_printString(char text[]) {
    int i = 0;//start at first character

    while (text[i] != '\0') {
        if (i == 16) {
            LCD_placeCursor(2);
        }
        if (i == 32) {
            break;
        }
        LCD_printChar(text[i]);
        i++;
    }
}
//Print integer to LCD
void LCD_printInt(int number) {
    char buffer[16];
    sprintf(buffer, "%d", number);
    LCD_printString(buffer);
}

void LCD_printFloat(float number, int decimal_places) {
    char buffer[16];
    sprintf(buffer, "%.*f", decimal_places, number);
    LCD_printString(buffer);
}






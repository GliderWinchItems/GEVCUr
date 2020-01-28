/******************************************************************************
* File Name          : 4x20lcd.h
* Date First Issued  : 2/13/2014
* Board              : Discovery F4 (F405 or F407)
* Description        : Helper functions for the 4x20 LCD screen
*						https://www.sparkfun.com/products/9568
*******************************************************************************/
#ifndef __4X20LCD
#define __4X20LCD

#include <stdio.h>

/*	LCD Line Size  */
#define LCDLINESIZE 20
#define LCDROWSIZE 4
#define LCD_BACKLIGHT_LEVEL 70

// initializes the lcd screen
uint8_t*  lcd_init(uint8_t*  p);

// clears the lcd screen
uint8_t*  lcd_clear(uint8_t*  p);

// turn the thing on/off
uint8_t*  lcd_on(uint8_t*  p);
uint8_t*  lcd_off(uint8_t*  p);

uint8_t*  lcd_backlight(uint8_t*  p, int level);

uint8_t*  lcd_moveCursor(uint8_t* p, int row, int col);

#endif 

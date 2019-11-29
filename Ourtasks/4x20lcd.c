/******************************************************************************
* File Name          : 4x20lcd.c
* Date First Issued  : 2/13/2014
* Board              : Discovery F4 (F405 or F407)
* Description        : Helper functions for the 4x20 LCD screen
*						https://www.sparkfun.com/products/9568
*******************************************************************************/

#include "4x20lcd.h"
#include <string.h>


uint8_t*  lcd_clear(uint8_t*  p) {
	*p++ = (254);
	*p++ = (0x01); // clear screen
	return p;
}

uint8_t*  lcd_on(uint8_t*  p) {
	*p++ = (254);
	*p++ = (0x0C); // remove cursor
	return p;
}

uint8_t*  lcd_off(uint8_t*  p) {
	*p++ = (254);
	*p++ = (0x08); // turn off screen
	return p;
}

uint8_t*  lcd_backlight(uint8_t*  p, int level) {
	level = level < 0 ? 0 : level;
	level = level > 100 ? 100 : level;
	level = 128 + (level * 29) /100;
	*p++ = (124);
	*p++ = (level); // set backlight level 128 - 157
	return p;
}

uint8_t*  lcd_moveCursor(uint8_t* p, int row, int col) {
	*p++ = (254); // move cursor command

	// determine position
	if (row == 0) {
		*p++ = (128 + col);
	} else if (row == 1) {
		*p++ = (192 + col);
	} else if (row == 2) {
		*p++ = (148 + col);
	} else if (row == 3) {
		*p++ = (212 + col);
	}
	return p;
}

uint8_t*  lcd_init(uint8_t*  p) {
	p = lcd_off(p);
	p = lcd_on(p);
	p = lcd_clear(p);
	p = lcd_backlight(p, LCD_BACKLIGHT_LEVEL);
	p = lcd_moveCursor(p, 0, 0);
	return p;
}


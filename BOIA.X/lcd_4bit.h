#ifndef LCD_4BIT_H
#define	LCD_4BIT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "hardware.h"
//#include "pic16.h"

//Configuração pinos display
#define TR_DADO_4  TRISBbits.TRISB0
#define TR_DADO_5  TRISBbits.TRISB1
#define TR_DADO_6  TRISBbits.TRISB2
#define TR_DADO_7  TRISBbits.TRISB3
#define TR_ENABLE  TRISBbits.TRISB4
#define TR_RS      TRISBbits.TRISB5
#define LCD_DADO_4 PORTBbits.RB0
#define LCD_DADO_5 PORTBbits.RB1
#define LCD_DADO_6 PORTBbits.RB2
#define LCD_DADO_7 PORTBbits.RB3
#define LCD_ENABLE PORTBbits.RB4
#define LCD_RS     PORTBbits.RB5

#define lcd_type 2        // 0=5×7, 1=5×10, 2=2 lines
#define lcd_line_two 0x40 // LCD RAM address for the 2nd line

const unsigned char LCD_INIT_STRING[4]=
{
20 | (lcd_type << 2),   // Func set: 4-bit, 2 lines, 5×8 dots
0xc,                    // liga display
1,                      // limpa display
6                       // incrementa cursor
};

void init_lcd_4bit(void);
void lcd_envia_nibble(unsigned char n);
void lcd_envia_byte(unsigned char address, unsigned char n);
void lcd_gotoxy(unsigned char x, unsigned char y);
void lcd_putc(unsigned char c);
void lcd_escreve_p(const unsigned char *n, ...);
void lcd_escreve_string(const unsigned char *y);
void LCDClear();
void LCDWriteCmd(unsigned char Byte);

#endif	/* LCD_4BIT_H */


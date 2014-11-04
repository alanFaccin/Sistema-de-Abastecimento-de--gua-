#include "lcd_4bit.h"

#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
//inicializa LCD
void init_lcd_4bit(void)
{
    unsigned char i;

    TR_ENABLE=0;
    TR_RS=0;
    LCD_ENABLE=0;
    LCD_RS=0;
    TR_DADO_4=0;
    TR_DADO_5=0;
    TR_DADO_6=0;
    TR_DADO_7=0;
    LCD_DADO_4=0;
    LCD_DADO_5=0;
    LCD_DADO_6=0;
    LCD_DADO_7=0;

    __delay_ms(15);

    for(i=0; i<3; i++)
    {
        lcd_envia_nibble(0x03);
        __delay_ms(5);
    }
    lcd_envia_nibble(0x02);

    for(i=0; i < sizeof(LCD_INIT_STRING); i++)
    {
        lcd_envia_byte(0, LCD_INIT_STRING[i]);
    }
}

//envia nibble para LCD
void lcd_envia_nibble(unsigned char n)
{
    LCD_DADO_4 = !!(n & 1);
    LCD_DADO_5 = !!(n & 2);
    LCD_DADO_6 = !!(n & 4);
    LCD_DADO_7 = !!(n & 8);

    _delay(1);
    LCD_ENABLE=1;
    __delay_us(2);
    LCD_ENABLE=0;
}

//envia byte para o LCD
void lcd_envia_byte(unsigned char address, unsigned char n)
{
    LCD_RS=0;
    __delay_us(60);

    if(address) LCD_RS=1;
    else LCD_RS=0;

    _delay(1);

    LCD_ENABLE=0;

    lcd_envia_nibble(n >> 4);
    lcd_envia_nibble(n & 0xf);
}

//posiciona cursor
void lcd_gotoxy(unsigned char x, unsigned char y)
{
    unsigned char address;

    if(y != 1) address = lcd_line_two;
    else address=0;
    
    address += x-1;
    lcd_envia_byte(0, 0x80 | address);
}

//envia caracter para display
void lcd_putc(unsigned char c)
{
    switch(c)
    {
        case '\f':
        lcd_envia_byte(0,1);
        __delay_ms(2);
        break;

        case '\n':
        lcd_gotoxy(1,2);
        break;

        case '\b':
        lcd_envia_byte(0,0x10);
        break;

        default:
        lcd_envia_byte(1,c);
        break;
    }
}

//escreve parametros
void lcd_escreve_p(const unsigned char *n, ...)
{
    va_list pa;

    unsigned char i=0,j=0,caracter[6];
    const unsigned char *ptr;
    unsigned int k=0;

    ptr=n;

    while(*n!='\0')
    {
        if(*n=='%')j++;
        n++;
    }

    va_start(pa,j);

    while(*ptr!='\0')
    {
        if(*ptr=='%')
        {
            ptr++;
            if(*ptr=='i')
            {
                k=va_arg(pa,int);
                itoa(caracter,k, 10);
                if(caracter[0]!='\0')lcd_putc(caracter[0]);
                if(caracter[1]!='\0')lcd_putc(caracter[1]);
                if(caracter[2]!='\0')lcd_putc(caracter[2]);
                if(caracter[3]!='\0')lcd_putc(caracter[3]);
                if(caracter[4]!='\0')lcd_putc(caracter[4]);
                for(i=0; i<6; i++)caracter[i]='\0';
            }
            if(*ptr=='h')
            {
                lcd_putc('0');
                lcd_putc('x');
                k=va_arg(pa,int);
                itoa(caracter,k, 16);
                if(caracter[0]!='\0')lcd_putc(caracter[0]);
                if(caracter[1]!='\0')lcd_putc(caracter[1]);
                if(caracter[2]!='\0')lcd_putc(caracter[2]);
                if(caracter[3]!='\0')lcd_putc(caracter[3]);
                if(caracter[4]!='\0')lcd_putc(caracter[4]);
                for(i=0; i<6; i++)caracter[i]='\0';
            }
            if(*ptr=='c')
            {
                k=va_arg(pa,char);
                lcd_putc(k);
            }
        }
        else
        {
            lcd_putc(*ptr);
        }
        ptr++;
    }
}

//escreve strings
void lcd_escreve_string(const unsigned char *y)
{
    while(*y!='\0')
    {
        lcd_putc(*y);
        y++;
    }
}
//teste


void LCDClear()
{
	lcd_envia_byte(0,0x01);
	__delay_ms(10);

	lcd_envia_byte(0,0x0C);
	__delay_us(200);

	lcd_envia_byte(0,0x06);
	__delay_us(200);
}

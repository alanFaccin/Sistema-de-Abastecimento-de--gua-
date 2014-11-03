// File:   pic16.h
// Author: User

#ifndef PIC16_H
#define	PIC16_H

//definiçoes para uso de delays em PICS 10F, 12F, 16F
#pragma intrinsic(_delay)
extern void _delay(unsigned long);
#pragma intrinsic(_delaywdt)
extern void _delaywdt(unsigned long);
#pragma intrinsic(_delay3)
extern void _delay3(unsigned char);
// NOTE: To use the macros below, YOU must have previously defined _XTAL_FREQ
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#define __delaywdt_us(x) _delaywdt((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delaywdt_ms(x) _delaywdt((unsigned long)((x)*(_XTAL_FREQ/4000.0)))

#endif	/* PIC16_H */


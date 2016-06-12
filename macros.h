/*************************************************************************
**  HARLEY DAVIDSON Sportster 883/1200 - J1850 VPW Interface
**  Released under GNU GENERAL PUBLIC LICENSE
**  Main MCU: MICROCHIP PIC18F2553
**  Homepage: www.momex.cat
**  Contact: morales.xavier@momex.cat
**
**
**  Revision History
**  11/06/2016   XM  v1.00   Initial Release on Github
**
**
**   NOTE: This file is based on a project from Team Harley ECU 2010
**         (https://seniordesign.engr.uidaho.edu/2009-2010/harley_ecu/) 
**         for arduino platform. Code has been modified in order to be 
**         compatible with Microchip PIC MCUs.
**************************************************************************/

#ifndef MACROS_H
#define MACROS_H

//Global Definitions***********************************************************
#define MCU_XTAL 20000000L	/Converting MCU_XTAL to Long variable
//End Global Definitions*******************************************************

//enumeration of the preescalers (8 bit counter 0-255), internal clock = F_CPU/4
//REGISTER T0CON (TIMER 0)
//bit7 (TMR0ON)--> 1=Enables Timer0 / 0=Disables Timer0
//bit6 (T08BIT)--> 1=T0 as 8bit counter / 0=T0 as 16bit counter
//bit5 (T0CS)--> 1=Transtion on T0CK1 pin / 0=internal instruction cycle
//bit4  (T0SE)--> if bit5=0 not used
//bit3 (PSA) --> 1=T0 preescaler not assigned / 0=T0 preescaler assigned (bit2-1-0)
//bit2-1-0 (T0PS2-T0PS0)
enum {
STOP		= 0b00000000,
CK4		= 0b11000001,
CK8		= 0b11000010,
CK32		= 0b11000100,
CK64		= 0b11000101,
CK128		= 0b11000110,
CK256		= 0b11000111,
CK16B128	= 0b10000110
};

/* Define Timer0*/
#define timer0_start(x)	T0CON=x;TMR0L=0;  // Timer0 enabled with a preescaler x
#define timer0_16start(x)	T0CON=x;WriteTimer0(0);  // Timer0 16bit enabled with a preescaler x
#define timer0_get()	TMR0L
#define timer0_stop()	T0CON=STOP;
#define INT_CLK	20000000L/4L


// convert microseconds to counter values for each preescaler
//  x usec * (1 sec / 1000000 usec) * (CLOCK clks / 1 sec) * (1 cnt / 8 clks)
//((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 8L) + 500000L) / 1000000L))
#define us2cntT0CON8(us) ((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 8L) + 500000L) / 1000000L))
//  x usec * (1 sec / 1000000 usec) * (CLOCK clks / 1 sec) * (1 cnt / 64 clks)
#define us2cntT0CON64(us) ((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 64L) + 500000L) / 1000000L))
//  x usec * (1 sec / 1000000 usec) * (CLOCK clks / 1 sec) * (1 cnt / 128 clks)
#define us2cntT0CON128(us) ((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 128L) + 500000L) / 1000000L))
//  x usec * (1 sec / 1000000 usec) * (CLOCK clks / 1 sec) * (1 cnt / 256 clks)
#define us2cntT0CON256(us) ((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 256L) + 500000L) / 1000000L))
//FOR TIMER3
//  x usec * (1 sec / 1000000 usec) * (CLOCK clks / 1 sec) * (1 cnt / 8 clks)
//((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 8L) + 500000L) / 1000000L))
#define us2cntT3CON8(us) ((unsigned char) (((us) * ((unsigned long)(INT_CLK) / 8L) + 500000L) / 1000000L))

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

//TIMER3 - enumeration of the preescalers (16 bit counter 0-65535), internal clock = F_CPU/4
//REGISTER T0CON (TIMER 0)
//bit7 (RD16)--> 1=Enables timer3 in one 16bit operation / 0= Enables timer3 in two 8bit operation
//bit6 and 3 (T3CCP2:T3CCP1)--> fixed to 00
//bit5-4 (T3CKPS1:T3CKPS0) -->Preescaler 11(1:8),10(1:4),01(1:2),00(1:1)
//bit2 (T3SYNC) fixed to 0
//bit1 (TMR3CS): 1: External Clocl / 0: Internal clock Fosc/4
//bit0 (TMR3ON): 1:Enables T3 / 0: Stops Timer3
enum {
T3STOP	= 0b00000000,
T3CK8	= 0b10110001,
T3CK1	= 0b10000001
};

/* Define Timer3*/
#define timer3_start(x)	T3CON=x;WriteTimer3(0);  // Timer3 16bit enabled with a preescaler x
#define timer3_get()	ReadTimer3()
#define timer3_stop()	T3CON=T3STOP;

//TIMER1 - enumeration of the preescalers (16 bit counter 0-65535)(8 bit counter 0-255), internal clock = F_CPU/4
//bit7 (RD16)--> 1=Enables timer3 in one 16bit operation / 0= Enables timer3 in two 8bit operation
//bit 6(T1RUN): Timer1 system clock status bit --> fixed to 0
//bit5-4 (T1CKPS1:T1CKPS0) -->Preescaler 11(1:8),10(1:4),01(1:2),00(1:1)
//bit3 (T1OSCEN -Timer1 oscillator enable bit) fixed to 0
//bit2 (T1SYNC) fixed to 0
//bit1 (TMR1CS): 1: External Clock / 0: Internal clock Fosc/4 --> fixed to 0
//bit0 (TMR1ON): 1:Enables T1 / 0: Stops Timer1

enum {
T1STOP	= 0b00000000,
T1CK8	= 0b10110001
};

/* Define Timer1*/
#define timer1_start(x)	T1CON=x;WriteTimer1(0);  // Timer3 16bit enabled with a preescaler x
#define timer1_get()	ReadTimer1()
#define timer1_stop()	T1CON=T1STOP;

#endif //MACROS_H






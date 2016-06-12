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
**   NOTE: This file is based on a project from l.e. hughes for arduino platform
**         (http://www2.cs.uidaho.edu/) 
**         Project: stairway_v_1_code.pde 
**         Code has been modified in order to be compatible with Microchip PIC MCUs.
**************************************************************************/

#ifndef __MM5450_H__    //if MM5450.h has not been defined--> define it || if yes --> do nothing
#define __MM5450__	// it is not necessary to give a value for __MM5450_H__
//the #endif will be written at the end of the file

#include "macros.h"

//************************TRISx_Clock****************************************
//PIC: Select TRISx for Clock
#define MMClock_dir_outp()	TRISAbits.TRISA3=0
//****************************************************************

//**************************LATx_Clock**************************************
//PIC: Put a bit of an Output at High.
#define MMClock_active()	LATAbits.LATA3=1
 //*****************************************************************

//**************************LATx_Clock**************************************
//PIC: Put a bit of an Output at Low
#define MMClock_passive()	LATAbits.LATA3=0
//*****************************************************************

//************************TRISx_Data****************************************
//PIC: Select TRISx for Data
#define MMData_dir_outp()	TRISAbits.TRISA2=0
//****************************************************************

//**************************LATx_Data**************************************
//PIC: Put a bit of an Output at High.
#define MMData_active()	LATAbits.LATA2=1
 //*****************************************************************

//**************************LATx_Data**************************************
//PIC: Put a bit of an Output at Low
#define MMData_passive()	LATAbits.LATA2=0
//*****************************************************************

#define BITSB 8                        // number of bits per byte, used for code clarity
#define DATABITS 36                    // what we must send to the chip in order to control the lights (1+35)
#define STARTBIT 1                     // value of the starting bit;  

// The following line just computes the number of bytes we will need in the ledArray to hold all of
// bits of data for the signal; it could be declared statically.
#define arrayLen  (((DATABITS-1)/BITSB) + 1)
 
#define TIME20us	us2cntT3CON8(10)
#define TIME50us	us2cntT3CON8(25)

typedef enum {                         // this exists primarily for code clarity
  OFF, ON
} ledState;

//Function Prototypes
extern void MM5450_init(void);
extern void delay20us(void);
extern void delay50us(void);
extern void sendDatabits(uint8_t *ledArray_buf);
extern void toggleLight(uint8_t pin, uint8_t *ledArray_buf);
extern void setLight(uint8_t pin, uint8_t val,uint8_t *ledArray_buf);
extern void allOn(uint8_t *ledArray_buf);
extern void allOff(uint8_t *ledArray_buf);


#endif // __MM5450_H__

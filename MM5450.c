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

#include <p18f2553.h>
#include "MM5450.h"
#include "macros.h"
#include <timers.h>

/* 
**--------------------------------------------------------------------------- 
** Abstract: This function initializes the MM5450 module using the definitions provided in MM5450.h. 
**           Aquesta funció inicialitza el MM5450 utilitzant les definicions de la llibreria MM5450.h
** Parameters: none
** Returns: none
**--------------------------------------------------------------------------- 
*/ 

void MM5450_init(void)
{
	// Set CL as output passive state.
	MMClock_dir_outp();
	MMClock_passive();

	// Set DT as output and passive state
	MMData_dir_outp();
	MMData_passive();

}

/* 
**--------------------------------------------------------------------------- 
** Abstract: This 2 functions wait for 20 and 50 us 
**           Aquestes 2 funcions esperen durant 20 i 50 us
** Parameters: none
** Returns: none
**--------------------------------------------------------------------------- 
*/ 

void delay20us(void)
{
	timer3_start(T3CK1);
	//while (timer3_get()<TIME20us){
	while (ReadTimer3()<1){	// 1=1,5us 3=5us | old value 13=20us
	}
	timer3_stop();

}

void delay50us(void)
{
	timer3_start(T3CK1);
	//while (timer3_get()<TIME50us){
	while (ReadTimer3()<3){   // 3=5us 8=12,5us  | old value 31=50us
	}
	timer3_stop();

}

/* 
**--------------------------------------------------------------------------- 
** Abstract: Subroutine that sends all of the DATABITS to the chip. It begins by first sending the startbit, then it
**           sends all the bits in each byte of the ledArray.
**           Subrutina que envia tots els DATABITS al chip MM5450. Primer envia l'Start of Frame i després la resta de bits del ledArray
** Parameters: pointer ledArray
** Returns: none
**--------------------------------------------------------------------------- 
*/ 

void sendDatabits(uint8_t *ledArray_buf) {

	uint8_t temp_byte;	// temporary byte store
	uint8_t nbits;		// bit position counter within a byte
	uint8_t MMbits;
	uint8_t nbytes;

	MMbits=35;
	nbytes=5;

	//Start of frame
  	MMClock_passive();
  	delay20us();
	MMData_active();
	delay20us();
  	MMClock_active();
	delay50us();
  	delay50us();
  	MMClock_passive();
  	delay20us();
	

	do
	{// end nbytes do loop
		temp_byte=*ledArray_buf;	//store byte termporary
		nbits=8;
		delay50us();
		//delay50us();
		delay50us();
		while(nbits-- && MMbits--)	//send 8 bits
		{
			if (temp_byte & 0x80){
				MMData_active();
				delay20us();
				MMClock_active();
	  			delay50us();
	  			MMClock_passive();
	  			delay20us();
			}else{
				MMData_passive();
				delay20us();
	  			MMClock_active();
	  			delay50us();
	  			MMClock_passive();
	  			delay20us();
			}
			temp_byte <<= 1;	// next bit (desplaça els bits cap a la dreta) 0101 -->0010
		}// end nbits while loop
		++ledArray_buf;	// next byte from buffer
	
	} while(--nbytes);
	MMData_passive();
		

}

/* 
**--------------------------------------------------------------------------- 
** Remark: function not use in tachometer project, but is left here as it can be helpful for other projects
** Abstract: Subroutine that takes a light (output pin) as a sole argument and sets the bit value for that pin
**           to the opposite of its current setting (toggle).
** Parameters: pin number, pointer to ledArray
** Returns: none
**--------------------------------------------------------------------------- 
*/ 

void toggleLight(uint8_t pin, uint8_t *ledArray_buf) {
	uint8_t arrayElem;
	uint8_t byteElem;
	
	arrayElem = (pin-1)/BITSB;                     // which element of the ledArray is pin in
  	byteElem  = (pin - (arrayElem * BITSB)) - 1;        // and which bit in that byte is the pin
  	ledArray_buf[arrayElem] ^= (1 << byteElem);                  // toggle byteElem 
}

/* 
**--------------------------------------------------------------------------- 
** Abstract: Subroutine that takes a light (output pin) and an additional argument and sets the bit value
**           for that pin appropriately. Pin input is from 1 to 34
**           Subrutina que donats un pin i un valor, canvia el valor del pin especificat
** Parameters: pin number, value, pointer to ledArray
** Returns: none
**--------------------------------------------------------------------------- 
*/ 

void setLight( uint8_t pin, uint8_t val,uint8_t *ledArray_buf) {
	uint8_t arrayElem;
	uint8_t byteElem;
  	
	arrayElem = ((pin-1)/BITSB);                     //byte(0 a 4) which element of the ledArray is pin in
  	byteElem  = (pin - (arrayElem * BITSB)) - 1;     //bit(0-7) and which bit in that byte is the pin
	//byteElem=7-byteElem;				 //LSB to MSB
  	ledArray_buf[arrayElem] |= (val << byteElem);    //|= --> a = a | b zero vals require a two-step process, 
  	if(val == 0 && (ledArray_buf[arrayElem]&(1 << byteElem))) {  // only enter if val=0 and bitvalue is 1
    	ledArray_buf[arrayElem] ^= (1 << byteElem);     //^= --> a = a ^ b (bitwise xor assigment) toggle them
		//xor--> 00 11 are 0, 01 10 are 1
		//<< --> implica bitwise
  } 
}

/* 
**--------------------------------------------------------------------------- 
** Abstract: Subroutines that turn all lights ON or OFF by setting the values of ledArray to ON or OFF.
**           Subrutines que posen els valors de ledArray a ON o a OFF
** Parameters: pointer to ledArray
** Returns: none
**--------------------------------------------------------------------------- 
*/ 
void allOn(uint8_t *ledArray_buf) {
	int i;
  	for(i = 1; i < DATABITS; i++) {
    	setLight(i, ON, ledArray_buf);
  	}
}

void allOff(uint8_t *ledArray_buf) {
	int i;

  	for(i = 0; i < arrayLen; i++) {	//posa cada byte a 0
    	ledArray_buf[i] = 0;
  	}
}

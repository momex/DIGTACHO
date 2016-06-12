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
**   NOTE: This file is based on a project from Michael (www.Mictronics.de) 
**         for arduino platform. Code has been modified in order to be 
**         compatible with Microchip PIC MCUs.
**         Based on Mictronics file j1850.c v1.07
**************************************************************************/

#include <p18f2553.h>
#include "j1850.h"
#include "macros.h"

char display7seg2[10] = {0b01000000,0b11111000,0b00100010,0b00110000,0b10011000,0b00010100,0b10000100,0b01111000,0b00000000,0b00011000};

/* 
**------------------------------------------------------------------------------------------------------------------- 
** Abstract: This function initializes the J1850 module using the definitions provided in j1850.h
**           Aquesta funció inicialitza el modul J1850 utilitzant les definicions que hi ha a la llibreria j1850.h 
** Parameters: none
** Returns: none
**-------------------------------------------------------------------------------------------------------------------
*/ 

void j1850_init(void)
{
	// Set TX as output and in passive state.
	bit_dir_outp();

	// Set RX as input and disable pullup
	bit_dir_inp();

}

/* 
**-------------------------------------------------------------------------------------------------------------------- 
** Abstract: This is an internal function used by the J1850 module to wait for the bus to become idle.
**           Funció interna utilitzada pel modul J1850 per esperar a que el bus esdevingui lliure.
** Parameters: none
** Returns: none
**-------------------------------------------------------------------------------------------------------------------- 
*/ 

static void j1850_wait_idle(void)
{
	timer0_start(CK8);

	while(timer0_get() < RX_IFS_MIN)	// wait for minimum IFS symbol
	{
		if(is_vpw_active())
		{
			timer0_stop();		// Stop timer
			timer0_start(CK8);	// Restart timer1 when bus not idle
		}
	}
}

/* 
**--------------------------------------------------------------------------- 
** Abstract: Receive J1850 frame (max 12 bytes)
**           Rebre trama J1850 (màxim 12 bytes)
** Parameters: Pointer to frame buffer / punter al missatge al buffer
** Returns: Number of received bytes OR in case of error, error code with bit 7 set as error indication
**          Número de bytes rebuts o en cas d'error, el codi d'error amb el bit 7 usat com a inidcador d'error
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_recv_msg(uint8_t *msg_buf )
{
	uint8_t nbits;			// bit position counter within a byte
	uint8_t nbytes;			// number of received bytes
	uint8_t bit_state;		// used to compare bit state, active or passive
	uint16_t tcnt1_buf;		//XM: used to compare counter
	
	/*wait for responds (if 100us pass without detection of a SOF--> answer with an error)*/			
	timer0_16start(CK16B128);

	while(!is_vpw_active())	// run as long bus is passive (IDLE)
	{
		if(timer0_get() >= WAIT_300us)	// check for 300us
		{
			timer0_stop();
			
			return J1850_RETURN_CODE_NO_DATA | 0x80;	// error, no responds within 100us
		}
	}
	// wait for SOF
	timer0_start(CK8);	// restart timer1
	while(is_vpw_active())	// run as long bus is active (SOF is an active symbol)
	{
		if(timer0_get() >=  RX_SOF_MAX) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error on SOF timeout
	}

	
	timer0_stop();
	if(timer0_get() < RX_SOF_MIN) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error, symbol was not SOF

	bit_state = is_vpw_active();	// store actual bus state
	timer0_start(CK8);
	for(nbytes = 0; nbytes < 12; ++nbytes)
	{
		nbits = 8;
		do
		{
			*msg_buf <<= 1;			//this is for refer bit by bit for every for cycle

			while(is_vpw_active() == bit_state) // compare last with actual bus state, wait for change
			{
	
				if(timer0_get() >= RX_EOD_MIN	)	// check for EOD symbol
				{
					timer0_stop();
					return nbytes;	// return number of received bytes
				}
			}
			bit_state = is_vpw_active();	// store actual bus state
			tcnt1_buf = timer0_get();
			timer0_start(CK8);

			if( tcnt1_buf < RX_SHORT_MIN) return J1850_RETURN_CODE_BUS_ERROR | 0x80;	// error, pulse was to short

			// check for short active pulse = "1" bit
			if( (tcnt1_buf < RX_SHORT_MAX) && !is_vpw_active() )
				*msg_buf |= 1;

			// check for long passive pulse = "1" bit
			if( (tcnt1_buf > RX_LONG_MIN) && (tcnt1_buf < RX_LONG_MAX) && is_vpw_active() )
				*msg_buf |= 1;

		} while(--nbits);// end 8 bit while loop. XM: Part of the Do-While. While nbits is not 0 has to Do all that above and decrement on bit each time.
		
		++msg_buf;	// store next byte. XM: Goes to the next memory direction, this means the next byte on the struct[]
	
	}	// end 12 byte for loop

	// return after a maximum of 12 bytes
	timer0_stop();	
	return nbytes;
}


/* 
**--------------------------------------------------------------------------- 
** Abstract: Send J1850 frame (maximum 12 bytes). XM: Not used for Tachometer, used for dummy emmiter node.
**           Enviar trama J1850 (màxim de 12 bytes). XM: No s'utilitza pel tacòmetre, s'utilitza pel node emisor per motius de test.
** Parameters: Pointer to frame buffer, frame length / Punter al missatge al buffer
** Returns: 0 = error
**          1 = OK
**--------------------------------------------------------------------------- 
*/ 
uint8_t j1850_send_msg(uint8_t *msg_buf, int8_t nbytes)
{
	uint8_t temp_byte;	// temporary byte store
	uint8_t nbits;		// bit position counter within a byte	
	uint16_t delay;		// bit delay time

	if(nbytes > 12)	return J1850_RETURN_CODE_DATA_ERROR;	// error, message to long, see SAE J1850

	j1850_wait_idle();	// wait for idle bus

	timer0_start(CK8);	
	vpw_active();	// set bus active

	while(timer0_get() < TX_SOF);	// transmit SOF symbol
  
	do
	{
		temp_byte = *msg_buf;	// store byte temporary
		nbits = 8;
    	while (nbits--)		// send 8 bits
		{
			if(nbits & 1) // start allways with passive symbol
			{
				vpw_passive();	// set bus passive
				timer0_start(CK8);
				delay = (temp_byte & 0x80) ? TX_LONG : TX_SHORT;	// send correct pulse lenght
        		while (timer0_get() <= delay)	// wait
				{
					//Arduino--> if(!VPW_PORT_IN & _BV(VPW_PIN_IN))	// check for bus error
					if(is_vpw_active())	// check for bus error
					{
						timer0_stop();
						return J1850_RETURN_CODE_BUS_ERROR;	// error, bus collision!
					}
				}
			}
			else	// send active symbol
			{
				vpw_active();	// set bus active
				timer0_start(CK8);
				delay = (temp_byte & 0x80) ? TX_SHORT : TX_LONG;	// send correct pulse lenght
        		while (timer0_get() <= delay);	// wait
				// no error check needed, ACTIVE dominates
			}
      		temp_byte <<= 1;	// next bit
		}// end nbits while loop
		++msg_buf;	// next byte from buffer
	} while(--nbytes);// end nbytes do loop
vpw_passive();	// send EOF symbol
timer0_start(CK8);
while (timer0_get() <= TX_EOF); // wait for EOF complete
timer0_stop();
return J1850_RETURN_CODE_OK;	// no error
}

/* 
**--------------------------------------------------------------------------- 
** 
** Abstract: Calculate J1850 CRC. XM: Not used for Tachometer, used for dummy emmiter node.
**           Calcular el J1850 CRC. XM: No s'utilitza pel tacòmetre, s'utilitza pel node emisor per motius de test.
** Parameters: Pointer to frame buffer, frame length / Punter al missatge al buffer, número bytes del missatge
** Returns: CRC of frame
**--------------------------------------------------------------------------- 
*/ 
// calculate J1850 message CRC
uint8_t j1850_crc(uint8_t *msg_buf, int8_t nbytes)
{
	uint8_t crc_reg=0xff,poly,byte_count,bit_count;
	uint8_t *byte_point;
	uint8_t bit_point;

	for (byte_count=0, byte_point=msg_buf; byte_count<nbytes; ++byte_count, ++byte_point)
	{
		for (bit_count=0, bit_point=0x80 ; bit_count<8; ++bit_count, bit_point>>=1)
		{
			if (bit_point & *byte_point)	// case for new bit = 1
			{
				if (crc_reg & 0x80)
					poly=1;	// define the polynomial
				else
					poly=0x1c;
				crc_reg= ( (crc_reg << 1) | 1) ^ poly;
			}
			else		// case for new bit = 0
			{
				poly=0;
				if (crc_reg & 0x80)
					poly=0x1d;
				crc_reg= (crc_reg << 1) ^ poly;
			}
		}
	}
	return ~crc_reg;	// Return CRC
}

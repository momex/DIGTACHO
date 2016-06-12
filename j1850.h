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
**         Based on Mictronics file j1850.h v1.04
**************************************************************************/

#ifndef __J1850_H__     //if J1850.h has not been defined--> define it || if yes --> do nothing
#define __J1850_H__	// it is not necessary to give a value for __J1850_H__
//the #endif will be written at the end of the file

#include "macros.h"

//**************************LATx**************************************
//PIC: Put a bit of an Output at High.
#define vpw_active()	LATCbits.LATC2=1
//*****************************************************************

//**************************LATx**************************************
//PIC: Put a bit of an Output at Low
#define vpw_passive()	LATCbits.LATC2=0
//*****************************************************************

//************************TRISx****************************************
//PIC: Select TRISx for Output
#define bit_dir_outp()	TRISCbits.TRISC2=0
//****************************************************************

//************************TRISx****************************************
//PIC: Select TRISx for Input
#define bit_dir_inp()	TRISBbits.TRISB0=1
//****************************************************************

//************************PORTx****************************************
//PIC: Read the state of an Input (High or Low).
#define is_vpw_active()	!PORTBbits.RB0		
// It has ! symbol because hardware uses transistor NPN which is inversing the J1850 signal (delete ! if you change the hardware)
// Té el simbol ! (invertir valor) degut a que el hardware utilitza un tarnsistor NPN que inverteix el senyal (treure el ! si es canvia el hardware)
//************************************************************


// 100us, used to count 100ms
#define WAIT_100us	8000		
#define WAIT_300us	24000

// define J1850 VPW timing requirements in accordance with SAE J1850 standard
// all pulse width times in us
// transmitting pulse width
#define TX_SHORT	us2cntT0CON8(64)		// Short pulse nominal time
#define TX_LONG		us2cntT0CON8(128)		// Long pulse nominal time
#define TX_SOF		us2cntT0CON8(200)		// Start Of Frame nominal time
#define TX_EOD		us2cntT0CON8(200)		// End Of Data nominal time
#define TX_EOF		us2cntT0CON8(280)		// End Of Frame nominal time
#define TX_BRK		us2cntT0CON8(300)		// Break nominal time
#define TX_IFS		us2cntT0CON8(300)		// Inter Frame Separation nominal time

// see SAE J1850 chapter 6.6.2.5 for preferred use of In Frame Respond/Normalization pulse
#define TX_IFR_SHORT_CRC	us2cntT0CON8(64)	// short In Frame Respond, IFR contain CRC
#define TX_IFR_LONG_NOCRC us2cntT0CON8(128)		// long In Frame Respond, IFR contain no CRC

// receiving pulse width
#define RX_SHORT_MIN	us2cntT0CON8(34)		// minimum short pulse time
#define RX_SHORT_MAX	us2cntT0CON8(96)		// maximum short pulse time
#define RX_LONG_MIN	us2cntT0CON8(96)		// minimum long pulse time
#define RX_LONG_MAX	us2cntT0CON8(163)		// maximum long pulse time
#define RX_SOF_MIN	us2cntT0CON8(123)		// l'he posat a 153 (abans 163) minimum start of frame time
#define RX_SOF_MAX	us2cntT0CON8(279)		// l'he posat a 249 (abans 239) maximum start of frame time
#define RX_EOD_MIN	us2cntT0CON8(163)		// minimum end of data time
#define RX_EOD_MAX	us2cntT0CON8(239)		// maximum end of data time
#define RX_EOF_MIN	us2cntT0CON8(239)		// minimum end of frame time, ends at minimum IFS
#define RX_BRK_MIN	us2cntT0CON8(239)		// minimum break time
#define RX_IFS_MIN	us2cntT0CON8(280)		// minimum inter frame separation time, ends at next SOF

// see chapter 6.6.2.5 for preferred use of In Frame Respond/Normalization pulse
#define RX_IFR_SHORT_MIN	us2cntT0CON8(34)	// minimum short in frame respond pulse time
#define RX_IFR_SHORT_MAX	us2cntT0CON8(96)	// maximum short in frame respond pulse time
#define RX_IFR_LONG_MIN		us2cntT0CON8(96)	// minimum long in frame respond pulse time
#define RX_IFR_LONG_MAX		us2cntT0CON8(163)	// maximum long in frame respond pulse time

// define error return codes
#define J1850_RETURN_CODE_UNKNOWN    0	//000
#define J1850_RETURN_CODE_OK         1	//001
#define J1850_RETURN_CODE_BUS_BUSY   2	//010
#define J1850_RETURN_CODE_BUS_ERROR  3	//011
#define J1850_RETURN_CODE_DATA_ERROR 4	//100
#define J1850_RETURN_CODE_NO_DATA    5	//101
#define J1850_RETURN_CODE_DATA       6	//110


//Function Prototypes
extern void j1850_init(void);
extern uint8_t j1850_recv_msg(uint8_t *msg_buf );
extern uint8_t j1850_send_msg(uint8_t *msg_buf, int8_t nbytes);
extern uint8_t j1850_crc(uint8_t *msg_buf, int8_t nbytes);

#endif // __J1850_H__

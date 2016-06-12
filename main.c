/*************************************************************************
**  HARLEY DAVIDSON Sportster 883/1200 - J1850 VPW Interface
**  License: Creative Commons CC BY-SA 3.0
**  Main MCU: MICROCHIP PIC18F2553
**  Homepage: www.momex.cat
**  Contact: morales.xavier@momex.cat
**
**
**  Revision History
**  11/06/2016   XM  v1.00   Initial Release on Github
**
**
**  Abstract: This code will allow to show by the 7 segment display and
**            different leds the information broadcasted on the J1850 bus
**            Aquest codi permetrà mostrar pels 7 segments i LEDs la
**            informació transmesa pel bus J1850
**************************************************************************/

/***************************************************************************
**   Example of a HD frame:
**
**   j1850_msg_buf[0]=0x28;	//0b0010 1000 -- PRIORITY
**   j1850_msg_buf[1]=0x1B;	//0b0001 1011 -- RPM
**   j1850_msg_buf[2]=0x10;	//0b0001 0000 -- ECM
**   j1850_msg_buf[3]=0x02;	//0b0000 0010 -- RPM HIGH RESOL
**   j1850_msg_buf[4]=0x0A;  	//xx rpm value (700)  (0Ah*256+0Fh)=700
**   j1850_msg_buf[5]=0xF0;  	//xx rpm value (700)
**   serial_msg_len=6;
**
**   check:
**   catalan: http://momex.cat/HD-tacho-part3
**   english: http://momex.cat/en/HD-tacho-part3
***************************************************************************/

/*INCLUDES*/
#include <p18f2553.h>
#include <timers.h>		//TO:j1850 functions	T1:used on main 	T2:PWM for brightness control on Micrel IC    T3:MICREL
#include <string.h>
#include <usart.h>
#include <stdio.h>
#include <pwm.h> 		//Used to vary the brightness of the MICREL MM5450

/*INCLUDE CUSTOM HEADERS*/
#include "macros.h"
#include "j1850.h"
#include "MM5450.h"

/*DEFINE CONSTANTS*/
#define BUTTON    	PORTAbits.RA0  		// Back switch. (Read values use LATAbits.LATA0) value=0 (GND=depressed) and value=1 (5V=released)
#define TRIS_BUTTON   	TRISAbits.TRISA0    	// TRIS (0 OUTPUT , 1 INPUT)

#define LED_MODE2   	LATAbits.LATA1  	// TEMP LED. MODE2. RECEIV LED (RED).  (0=OFF, 1=ON)
#define TRIS_MODE2   	TRISAbits.TRISA1    	// TRIS (0 OUTPUT , 1 INPUT)

#define DIG3    	LATAbits.LATA4  	// Used to show Digit3 or RPM bar. Value=0 (GND=digit working) i value=1 (5V=DIG3 not working)
#define TRIS_DIG3  	TRISAbits.TRISA4    	// TRIS (0 OUTPUT , 1 INPUT)

#define RPM_BAR    	LATAbits.LATA5  	// Mostrar RPM o No. Value=0 (GND=RPMbar working) i value=1 (5V=RPMbar not working)
#define TRIS_RPM_BAR   	TRISAbits.TRISA5    	// TRIS (0 OUTPUT , 1 INPUT)

#define LED_MODE0    	LATCbits.LATC0  	// RPM LED. INDICATOR OF MODE0 ACTIVE(1) OR NOT(0)
#define TRIS_MODE0   	TRISCbits.TRISC0    	// TRIS (0 OUTPUT , 1 INPUT)

#define LED_MODE1    	LATCbits.LATC1  	// INDICATOR OF MODE1 ACTIVE(1) OR NOT(0) (0=OFF, 1=ON)
#define TRIS_MODE1   	TRISCbits.TRISC1    	// TRIS (0 OUTPUT , 1 INPUT)

#define LED_MODE2    	LATAbits.LATA1  	// INDICATOR OF MODE2 ACTIVE(1) OR NOT(0) (0=OFF, 1=ON)
#define TRIS_MODE2   	TRISAbits.TRISA1    	// TRIS (0 OUTPUT , 1 INPUT)

#define PWM_BRIGHTNESS	TRISCbits.TRISC2	// TRIS (0 OUTPUT , 1 INPUT)

void USART_hex2ascii(uint8_t val);
void send_byte(unsigned char ch);

//declare variable as global for ISR process
uint8_t j1850_msg_buf[12];  	// J1850 message buffer
uint8_t	recv_nbytes;		// info from reception of message
char display7seg[18] = {0b01000001,0b11111001,0b00100011,0b00110001,0b10011001,0b00010101,0b00000101,0b01111001,0b00000001,0b00011001,0b11111111,0b01111111,0b11111011,0b11111101,0b11110111,0b11101111,0b11011111,0b10111111};   
// 0           1          2           3         4         5          6        7           8          9          10:OFF      11:a      12:b       13:c       14:d        15:e    16:f 17:- g

//Interruption header for external interrupt
void InterruptHandlerHigh(void);

//main program
void main(void){

	/*Declare variables*/
	int i;				//aux variable in some "for" (-127 to 127)
	int counter_switch;		//counter for rear switch - change mode or change light intensity
	int mode;			//mode indicator. 0:rpm 7seg, 1:fuel consumpt + rpm bar, 2: Temp + rpm bar
	int blinking_counter;		//variable counter for blinking 
	uint8_t ledArray[5];		//array for Display
	unsigned int rpm[3];		//array for rpm. rpm[0]=for the display    rpm[1]=current rpm    rpm[2]=last rpm
	unsigned int speed[2];		//array for speed. speed[0]=for the display    speed[1]=current speed
	char engon;			//flag to know if engine is ON
	int comptengon;			//counter engine ON
	unsigned int temp[2];		//array for temp. temp[0]=for the display    temp[1]=current temp
	uint8_t gear[2];		//array for gear.
	uint8_t nextgear;		//next gear
	int comptcurrentgear;		//counter for current gear
	unsigned char auxiliar;		//variable for "val" calculation in Sendvalues function of Micrel
	unsigned char digits[4];	//calculation of the 4 digits for the 4x7segments
	char buffer[20];
	int brightness;			//value for light intensity for MM5450
	float instantconsum;		//value for liters of petrol every 100km

	//7 segment common annode. PORT Values for 7 6 5 ...2 1 0 bits for values from 0 to 9, OFF, 7x RPM bar status(idle, 1000,...,6000) i E (d'error)
	char display4x7seg[19] = {0b11111100,0b01100000,0b11011010,0b11110010,0b01100110,0b10110110,0b10111110,0b11100000,0b11111110,0b11110110,0b00000000,0b00000010,0b00000110,0b00001110,0b00011110,0b00111110,0b01111110,0b11111110,0b00111110};

	/*Modify PIC registers*/
	ADCON0 = 0b00000000;		//bit0=0 to turn off A/D conversion
	ADCON1 = 0b00001111;		//bit0-3 =1 to show that there is no analog input
	
	//Set TRISx and LATx 
	TRISC=0x00;			//all pins as output
	LATC=0x00;			//all output values values as GND
	TRISB=0b00000001;		//Port B, outputs for 7segments (0 output / 1 input)
			
	PWM_BRIGHTNESS=0;		//Brightness control pin port as Output.
	
	//Functions Inicialitzation
	MM5450_init();			//init MM5450 chip
	j1850_init();			// init J1850 bus
	
	TRIS_BUTTON=1;   		//Switch port as input
	
	//LEDs configuration
	TRIS_MODE2=0;			//TEMP LED as Output
	LED_MODE2=0;			//OFF as initial status
	
	TRIS_DIG3=0;			//Controller of DIG3 for ON or OFF as Output
	DIG3=0;				//OV --> DIG3 will work
	
	TRIS_RPM_BAR=0;			//Controller of RPM bar for ON or OFF as Output
	RPM_BAR=1;			//5V--> will not work
	
	TRIS_MODE0=0;			//RPM LED as output
	LED_MODE0=0;			//MODE0 active from the begining
	
	TRIS_MODE1=0;			//L/100 LED as output
	LED_MODE1=0;			//MODE1 OFF as initial status
	

	/******************************************************************
	** Configuring timer 2 which provides timing for PWM
	** TIMER_INT_OFF: disable timer interrupt
	** T2_PS_1_4: Timer2 prescaling set to 16
	** T2_POST_1_1: Timer2 postscaling set to 16
	********************************************************************/
	OpenTimer2(TIMER_INT_OFF & T2_PS_1_16 & T2_POST_1_4);
	/******************************************************************
	**  configuration of PWM Brightness regulation on MM5450
	** PWM period =[(period ) + 1] x 4 x TOSC x TMR2 prescaler. The value of period is from 0x00 to 0xff
	** PWM Period= [(30)+1]*4*(1/20e6)*16=0,099ms -->10Hz
	*******************************************************************/
	OpenPWM1(30); // configuring PWM module 1 --> aprox 1221Hz amb prescaler de 16

	//set brightness	    
	brightness=60;
	SetDCPWM1(brightness); // Range goes from (0-1023). 1023 sets PWM duty cycle 100% (full speed).
	
	/********************************************************************************************
	************************** Init of array's and LEDs checking ********************************
	*********************************************************************************************/
	//All A segments lighted
	ledArray[0]=0b00000001;
	ledArray[1]=ledArray[0];
	ledArray[2]=ledArray[0];
	ledArray[3]=ledArray[0];
	ledArray[4]=0b00000000;
	sendDatabits(ledArray);
	LATB=display7seg[11];

	//wait for 0,1s*2 seconds aprox for each segment
	i=1;
	while(i<8){		
		//wait 0,2s
		timer1_start(T1CK8);	
		while(timer1_get()<65000){
		}	
		timer1_stop();
		while(timer1_get()<65000){
		}
		timer1_stop();
		//end of wait

		ledArray[0]<<=1;
		ledArray[1]=ledArray[0];
		ledArray[2]=ledArray[0];
		ledArray[3]=ledArray[0];
		sendDatabits(ledArray);
		//As Gear 7seg display does not have DP, this if will prevent to show strange value
		if(i==7){
               LATB=display7seg[10];
		}else{
  		       LATB=display7seg[11+i];
		}
		i=i+1;
	}
	//end off segments init

	//All 7seg OFF
	ledArray[0]=0x00;
	ledArray[1]=0x00;
	ledArray[2]=0x00;
	ledArray[3]=0x00;
	ledArray[4]=0x00;
	sendDatabits(ledArray);
	LATB=display7seg[10];	//Gear Display OFF
	
	//wait for 0,2 seconds
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_stop();
	while(timer1_get()<65000){
	}
	timer1_stop();
	//end of wait

	/****************LEDs Check******************/
	RPM_BAR=0;	//0V --> RPM bar will work
	DIG3=1;		//5v --> Digit3 will not work
	//Light ON from bottom to top LED
	ledArray[0]=0b00000000;
	ledArray[1]=0b00000000;
	ledArray[2]=0b00000000;
	ledArray[3]=0b01000000;
	ledArray[4]=0b01000000;
	sendDatabits(ledArray);	
	LED_MODE2=1;

	i=1;
	//wait for 0,1s*2 seconds for each LED
	while(i<7){
		//wait 0,2s
		timer1_start(T1CK8);	
		while(timer1_get()<65000){
		}
		timer1_stop();
		while(timer1_get()<65000){
		}
		timer1_stop();
		//end of wait
		
		if(i==1){
			ledArray[3]=0b01100000;
		}else if(i==2){
			ledArray[3]=0b01110000;
			ledArray[4]=0b11000000;
			LED_MODE1=1;
		}else if(i==3){
			ledArray[3]=0b11111000;
		}else if(i==4){
			ledArray[2]=0b10000000;	
			ledArray[3]=0b11111100;
			LED_MODE0=1;
		}else if(i==5){
			ledArray[0]=0b10000000;
			ledArray[3]=0b11111110;
		}else if(i==6){
			ledArray[3]=0b11111111;
		}else{
			LATB=display7seg[17];		//show a -
		}
		sendDatabits(ledArray);	
		i=i+1;
	}
	//wait for 0,2s
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_stop();
	while(timer1_get()<65000){
	}
	timer1_stop();
	//emd of wait
	
	ledArray[0]=0x00;
	ledArray[1]=0x00;
	ledArray[2]=0x00;
	ledArray[3]=0x00;
	ledArray[4]=0x00;
	sendDatabits(ledArray);

	LATB=display7seg[10];	//Gear display OFF
	LED_MODE0=0;		//LEDs OFF
	LED_MODE1=0;
	LED_MODE2=0;

	RPM_BAR=1;		//5V --> RPM bar will not work
	DIG3=0;			//0v --> Digit3 will not work

	//arrays init
	digits[0]=8;	
	digits[1]=8;
	digits[2]=8;
	digits[3]=8;
	
	rpm[0]=0;
	rpm[1]=0;

	speed[0]=0;
	speed[1]=0;

	temp[0]=0;
	temp[1]=0;

	engon=0;		//Engine OFF
	comptengon=0;
	
	gear[0]=0x00;
	gear[1]=0x00;
	comptcurrentgear=0; 	//counter init
	nextgear=0x00;  	//init as 0x00 (Neutral)
	
	//init to 0 all message buffer
	for (i=0;i++;i<12){
		j1850_msg_buf[i]=0;
	}
	
	//until switch is not depressed, Digital Tacho will remain OFF
	i=0;
	while(i<127){							
		if(BUTTON==0){			//rear switch depressed.
				i=i+1;
		}else if(BUTTON==1){		//switch released
			//do nothing
		}
	}
	//wait 0,5seconds
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	timer1_start(T1CK8);	
	while(timer1_get()<65000){
	}
	//end of wait

	//CONFIG: EXTERNAL INTERRUPTION - RB0 (INT0)
	INTCON2bits.RBPU=0;	//pull-ups deactivated from ports RB (RB0 has alreday one on the circuit)
	INTCONbits.INT0IE = 1; 	//enable INT0 external interrupt
	INTCON2bits.INTEDG0=0;	//INT0 on falling edge.    It was inverted due to the Hardware is already inverted (j1850 vs TTL: 7V is 0V and 0V is 5V. 
				//If schematic changes and INTEDG0 goes to 1, j1850.h has to be changed as well (delete ! in (#define is_vpw_active()	!PORTBbits.RB0)
	RCONbits.IPEN = 1; 	//enable priority levels on interrupts
	INTCONbits.GIEH = 1; 	//enable all high-priority interrupts
	INTCONbits.INT0IF = 0; 	//clear INT0 flag
	
	// USART CONFIGURATION FOR PC COMM
	// Without Tx or Rx interruptions, asincronous mode (uart), 8 bits of data without parity
	// Recepction mode continuous, Transmission speed of ...
	OpenUSART(USART_TX_INT_OFF &
	 USART_RX_INT_OFF &
	 USART_ASYNCH_MODE &
	 USART_EIGHT_BIT &
	 USART_CONT_RX &
	 USART_BRGH_HIGH,
	 10); //10=115,2K  //129=9,6K
	
	putrsUSART((const far rom char *)"TachoJ1850_XMM_2010-2015");
	
	//System auxiliar variables init
	recv_nbytes=0x50; 	//0b 1010 0000
	i=0;
	blinking_counter=0;
	counter_switch=0;
	mode=0;			//initial mode=0 (RPM)
	LED_MODE0=1;
	
	//The idea is to refresh the display at a aproximate of 10 times per second
	timer1_start(T1CK8);
	
		while(1){			
		/***********************************************************************************************************
		***********************************   BRIGHTNESS AND MODE CHANGE   *****************************************
		************************************************************************************************************/
			if(BUTTON==0){			//rear switch depressed.
				if(counter_switch>25000){
					//do nothing, once switch is released brightness will change
				}else{
					counter_switch=counter_switch+1;
				}				
			}else if(BUTTON==1){		//switch released
				//do nothing
			}

			//change of mode
			if(counter_switch>2000 && counter_switch<10000 && BUTTON==1){
				counter_switch=0;
				if(mode==0){		//RPM
					mode=1;		//--> Fuel Consump
					LED_MODE0=0;
					LED_MODE1=1;
					LED_MODE2=0;
				}else if(mode==1){	//Fuel consump
					mode=2;		//--> Temp
					LED_MODE0=0;
					LED_MODE1=0;
					LED_MODE2=1;
				}else if(mode==2){	//Temp
					mode=3;		//--> Speed
					LED_MODE0=0;
					LED_MODE1=0;
					LED_MODE2=0;
				}else if(mode==3){	//speed
					mode=0;		//--> RPM
					LED_MODE0=1;
					LED_MODE1=0;
					LED_MODE2=0;
				}
			}else if(counter_switch>10000 && BUTTON==1){
				//max value=120, but for safety reasons (too much heat) is software limited to 60
				counter_switch=0;
				//brightness=brightness+10;
				if(brightness==60){		
					brightness=10;
				}else if(brightness==10){
					brightness=60;					
				} //values will be either 10 or 60
				SetDCPWM1(brightness);
			}
		/**************************************************************************************************/
	
			if (recv_nbytes & 0x50){	//Until first signal is not received it will show a "-"
				LATB=display7seg[17];		// "-"
			}else{
				if(recv_nbytes & 0x80){	//in case of error
					//rpm[1]=(recv_nbytes && 0x0F);
				}else{		//use the array[2] to save the current value and to store new value in array[1]
					if (j1850_msg_buf[0]==0x28 && j1850_msg_buf[1]==0x1B && j1850_msg_buf[2]==0x10 && j1850_msg_buf[3]==0x02){   	//rpm
						rpm[2]=rpm[1]; 	//save the previous value
						rpm[1]= (((unsigned char)j1850_msg_buf[4]*0x100+(unsigned char)j1850_msg_buf[5])/4);			//0x100=256dec
						//way to know if engine on or not (and a filter to avoid strange values on display)
						if ((engon==0 && rpm[1]>500) || (engon==1 && rpm[1]<500)){
							comptengon=comptengon+1;
							if (comptengon>=4){
								engon=1;	//Engine is ON
								comptengon=0;
							}else{
								//no fer res
							}
						}else{
							comptengon=0;
						}

					}else if(j1850_msg_buf[0]==0xA8 && j1850_msg_buf[1]==0x3B && j1850_msg_buf[2]==0x10 && j1850_msg_buf[3]==0x03){	//Gear
						//current gear, 0xXX = 0x02,0x04,0x08,0x10,0x20, for gears 1-5
						//also filter to avoid strange gear display behaviour (it will check 4 times gear is the same before changing the display)
						gear[0]=j1850_msg_buf[4];
						if (comptcurrentgear==0){		//start counter
							nextgear=gear[0];
							comptcurrentgear=comptcurrentgear+1;
						}else if(comptcurrentgear>=3){	//display will be updated here
							if(mode==3){		
								LATB=display7seg[10];						
							}else{
								if(gear[0]==0x00 ){
									LATB=display7seg[0];
								}else if(gear[0]==0x02){
									LATB=display7seg[1];
								}else if(gear[0]==0x04){
									LATB=display7seg[2];
								}else if(gear[0]==0x08){
									LATB=display7seg[3];
								}else if(gear[0]==0x10){
									LATB=display7seg[4];
								}else if(gear[0]==0x20){
									LATB=display7seg[5];
								}else{
									LATB=display7seg[17];
								}
							comptcurrentgear=0;
							}
						}else{
							if(gear[0]==nextgear){
								comptcurrentgear=comptcurrentgear+1;
							}else{
								comptcurrentgear=0;
							}
						}

					}else if(j1850_msg_buf[0]==0xA8 && j1850_msg_buf[1]==0x49 && j1850_msg_buf[2]==0x10 && j1850_msg_buf[3]==0x10){	//Engine Temp
						temp[1]= (unsigned char)j1850_msg_buf[4]-40;

					}else if(j1850_msg_buf[0]==0x48 && j1850_msg_buf[1]==0x29 && j1850_msg_buf[2]==0x10 && j1850_msg_buf[3]==0x02){	//Speed
						speed[1]= (((unsigned char)j1850_msg_buf[4]*0x100+(unsigned char)j1850_msg_buf[5])/128);		//0x100=256dec
					}
				}
			}
	
	
	//Modify ledArray to show the values we want in all 7 Segments connected to MICREL
	//Digit[3] is the one showing thousands and digit[0] is the one showing units (it will be fixed to 0 for rpm)
	//Decimal point is in Digit[1] and it can be activated
	
			if (timer1_get()>60000){		//It will be accessed between 5-10 times per second
				//save values
				timer1_stop();
				temp[0]=temp[1];
				speed[0]=speed[1];
				if (engon==1 && rpm[1]<500){
					rpm[0]=rpm[2];
				}else{
					rpm[0]=rpm[1];
				}
				
				//show (or not) the RPM bar and set the value for digit3
				if(mode==1 || mode==2){	//mode 1, 2 RPM bar i 7seg
					RPM_BAR=0;	//0V --> RPM bar will work
					DIG3=1;		//5v --> digit3 will not work
					
					if(rpm[0]<6500){
						blinking_counter=0;
					}
					
					if(rpm[0]<700){
						digits[3]=10;
					}else if(rpm[0]>=700 && rpm[0]<1500){	//yellow
						digits[3]=11;
					}else if(rpm[0]>=1500 && rpm[0]<2500){  //yellow
						digits[3]=12;
					}else if(rpm[0]>=2500 && rpm[0]<3500){  //green
						digits[3]=13;
					}else if(rpm[0]>=3500 && rpm[0]<4000){  //green
						digits[3]=14;
					}else if(rpm[0]>=4000 && rpm[0]<4500){  //yellow
						digits[3]=15;
					}else if(rpm[0]>=4500 && rpm[0]<5000){  //yellow
						digits[3]=16;
					}else if(rpm[0]>=5000 && rpm[0]<5500){  //red
						digits[3]=17;
					}else if(rpm[0]>=5500){			//blinking
						if(blinking_counter<2){
							blinking_counter=blinking_counter+1;
						}else if(blinking_counter==2){
							if(digits[3]==17){
								digits[3]=10;
							}else if(digits[3]==10){
								digits[3]=17;
							}else{
								//
								digits[3]=17;
							}
							blinking_counter=1;
						
						}else if(blinking_counter==0){	//All LEDs ON when it enters here
							digits[3]=17;
						}else{
							//off due to shouldn't enter here
							digits[3]=10;
							blinking_counter=1;
						}
						
					}else{
						//off due to shouldn't enter here
						digits[3]=10;
					}
	
					
				}else if(mode==0 || mode==3 ){	//RPM normal
					RPM_BAR=1;	//5V --> RPM bar will not work
					DIG3=0;		//0v --> Digit3 will work
					
				}
				//end setting the RPM bar

				//setting digit 2,1 amd 0 values for each mode
				if(mode==0){
					digits[3]=rpm[0]/1000;
					if (digits[3]==0){
						digits[3]=10;		//digit OFF
					}
					if (rpm[0]>650 && rpm[0]<850){	//rpms are not constant at idle, this will force rpm to be at 800 all the time wihile idling
						rpm[0]=800;
					}
					digits[2]=rpm[0]/100%10;
					if (digits[2]==0 && digits[3]==10){
						digits[2]=10;  		//digit OFF
					}		
				  	digits[1]=rpm[0]/10%10;

					if (digits[1]<5){
						digits[1]=0;
					}else{
						digits[1]=5;
					}
					if (digits[1]==0 && digits[2]==10 && digits[3]==10){
						digits[1]=10;
					}
		
				 	digits[0]=0;
				}else if(mode==1){  //Speed
					digits[2]=speed[0]/100%10;	// e.g. 1237/100 --> 12 -->12%10 = 2
					if (digits[2]==0){
						digits[2]=10;
					}
	
				  	digits[1]=speed[0]/10%10;	//  e.g. 1237/10 --> 123 --> 123%10 = 3
					if (digits[1]==0 && digits[2]==10){
						digits[1]=10;
					}
	
			 		digits[0]=speed[0]%10;							
				}else if(mode==2){		//Temperature
					digits[2]=temp[0]/100%10;	// e.g. 1237/100 --> 12 -->12%10 = 2
					if (digits[2]==0){
						digits[2]=10;
					}
	
				  	digits[1]=temp[0]/10%10;	// e.g. 1237/10 --> 123 --> 123%10 = 3
					if (digits[1]==0 && digits[2]==10){
						digits[1]=10;
					}
	
			 		digits[0]=temp[0]%10;
				}else if(mode==3){	//Fuel consumption will not be implemented, this mode will be all OFF
					//Fuel consumption calculation is this version of tachometer gives a bad estimation because it can not ask for real Throttle Position.
					// This mode will set OFF 4x7segment
					//5.2L/2=2.6L*2000rpm=5200L\m*.85=4420L\m*1.22g\L=5392.4g\m /14.7=
					//366.83g\m/760g\l=.48L\m
					//instantconsum=(0.883/2)*rpm[0]*(85/100)*1.22*(1/14.68)*(1/750)*(1/speed[0])*100*60;	//liter per 100km
					digits[0]=10;
					digits[1]=10;
					digits[2]=10;
					digits[3]=10;		//digit OFF							
				}
	
				//Modify the bits of ledArray. Once finished, only need to send it to MIcrel
				for (i=1;i<9;i++){
					//DIGIT 0
					if(display4x7seg[digits[0]] & (1<<(8-i))){	//check bit by bit if it is a 0 or 1
						auxiliar=1;
					}else{
						auxiliar=0;
					}
					setLight(i,auxiliar,ledArray);  //set bit on ledarray and inverse the order
					
					//DIGIT 1
					if(display4x7seg[digits[1]] & (1<<(8-i))){
						auxiliar=1;
					}else{
						auxiliar=0;
					}
					setLight(i+8,auxiliar,ledArray);  //set bit on ledarray and inverse the order //i+8 is the second array
		
					//DIGIT 2
					if(display4x7seg[digits[2]] & (1<<(8-i))){
						auxiliar=1;
					}else{
						auxiliar=0;
					}
					setLight(i+16,auxiliar,ledArray);  //set bit on ledarray and inverse the order //i+16 is the third array
		
					//DIGIT 3
					if(display4x7seg[digits[3]] & (1<<(8-i))){
						auxiliar=1;
					}else{
						auxiliar=0;
					}
					setLight(i+24,auxiliar,ledArray);  //set bit on ledarray and inverse the order //i+24 is the fourth array
					
					//Still free bits i=1,17,25,33 i 34.
					//Add code if you want to use them for Fuel tank level
				
					//Array sent
					sendDatabits(ledArray);
					timer1_start(T1CK8);
				}
				
			}else{
				//do nothing
			}
			
		}
}

void USART_hex2ascii(uint8_t val){
uint8_t ascii1=val;
send_byte( ((ascii1>>4)<10)?(ascii1>>4)+48 : (ascii1>>4)+55);
send_byte( ((val&0x0f)<10)?(val&0x0f)+48 : (val&0x0f)+55);
}

 void send_byte(unsigned char ch)
{
 while (TXSTAbits.TRMT==0);   // wait until TX buffer is empty
 TXREG=ch;
}

/******************************************
*****High priority interrupt vector********
******************************************/

#pragma code InterruptVectorHigh = 0x08			// address of high priority interruption
void InterruptVectorHigh(void)
{
	_asm
		goto	InterruptHandlerHigh			//Jump to interruption service
	_endasm
}
#pragma code

  
/****************************************
***********INTERRUPCION ROUTINE********
*****************************************/

#pragma interrupt InterruptHandlerHigh

void InterruptHandlerHigh(){

char i;
char buffer[20];

recv_nbytes=j1850_recv_msg(j1850_msg_buf);
if(recv_nbytes & 0x80){
}else{
	i=0;
	while(i<recv_nbytes){
		USART_hex2ascii(j1850_msg_buf[i]);
		if(i==recv_nbytes-1){
			send_byte(0x0D);	//intro	
		}else{
			send_byte(0x20);	//space
		}
		i=i+1;
	}

}
INTCONbits.INT0IF = 0; //clear INT0 flag
}

/************************************************************************************************************
*								Weihnachtsbaum ( XmasTreeXtreeme )											*
*								__________________________________  										*
*						Simulationen :	- Sterne funkeln													*
*										- Kerzen flackern													*
*										- Regentropfen														*
*										- Hoch / Runter faden												*
*										- LED´s wandern von oben nach unten									*
*										- Eine LED wandert im Karussel nach unten							*
*										- Rundum Licht ( 1 Seite von insgesamt zwei Ästen an )				*
*										- Jede zweite LED wechselt ( on / off )								*
*										- Es werden zufällig 4 LED´s auf und ab gedimmt						*
*																											*
*						Eigenschaften :	- Extrem sparsamer Standby (Sleep Mode ca. 57 µA)					*
*										- Menüauswahl über 2 Taster (10 Menüfunktionen)						*
*										- Anzeigen von Texte über eine 8x8 LED-Matrix						*
*										- Ladezeit ca. 1 Stunde 10 Minuten									*
*										- Standby nach ca. 12 Std.											*
*										- Verbleibende Zeit bevor das Gerät ausgeht ( jede Stunde 1 x kurz)	*
************************************************************************************************************/

/*
LED Matrix :

Zeile 1 : PORTC0
Zeile 2 : PORTC1
Zeile 3 : PORTC2
Zeile 4 : PORTC3
Zeile 5 : PORTC4
Zeile 6 : PORTC5
Zeile 7 : PORTC6
Zeile 8 : PORTC7

column 1 : PORTB0
column 2 : PORTB1
column 3 : PORTB2
column 4 : PORTB3
column 5 : PORTB4
column 6 : PORTB5
column 7 : PORTB6
column 8 : PORTB7
- - - - - - - - - - - - - - 

Shift Register (74HC595) :

MR	 : PORTD7
SHCP : PORTD6
SD   : PORTD5
STCP : PORTD4
_____________________________

Ambient Light Sensor :

ADC0 : PORTA0
_____________________________


Show Accu full :

ADC5 : PORTA5
_____________________________


Show Accu Charging :

ADC6 : PORTA6
_____________________________ 

Voltage monitoring :

ADC7 : PORTA7
_____________________________

INT0 :

PORTD2 ( weak up the µC! )
_____________________________

Live_LED :

PORTA1 ( live the µC? )
_____________________________

Switch Off 595 :

PORTA4 ( Disconnect the GND from the 595´ers? )
_____________________________

*/


/**********************************************************************/

		/* PLESE DO YOUR CONFIGURATION ONLY HERE */


/* show on the 8 x 8 Matrix the remaining hours bevor sleep */
#define WITH_REMAINING

/* would you like show LIVE_LED? */
#define WITH_LIVE_LED

/* would you like show Start Text ? */
#define WITH_START_TEXT

/* ATTENTION : choose one please */
//#define MENUE_CHANGE_ONE
#define MENUE_CHANGE_TWO

/* would you like to "show Sleep Info ?" */
#define WITH_SLEEP_INFO

/* would you like "UART" ? */
#define WITH_UART


/**********************************************************************/



/* Definiert die Frequenz */
#define F_CPU 16000000

/* Disconnect the GND from the 595´er ( save Current) */
#define SWITCH_OFF_595			PORTA &= ~(1<<PA4)
#define SWITCH_ON_595			PORTA |=  (1<<PA4)
#define SWITCH_TOGGLE_595		PORTA ^=  (1<<PA4)

/* Switches */
#define SWITCH_ONE_PORT			PINA
#define SWITCH_ONE_PIN			PA3

#define SWITCH_TWO_PORT			PINA
#define SWITCH_TWO_PIN			PA2

#define SWITCH_ONE_PRESSED		(!(PINA & (1<<PA3)))
#define SWITCH_TWO_PRESSED		(!(PINA & (1<<PA2)))

/* Ambient Light Sensor */
#define LIGHT_SENSOR PA0

/* Live LED */
#define LIVE_LED_ON				PORTA &= ~(1<<PA1)
#define LIVE_LED_OFF			PORTA |=  (1<<PA1)
#define LIVE_LED_TOGGLE			PORTA ^=  (1<<PA1)

/* If Accu full ? */
#define ACCU_FULL				(!(PINA & (1<<PA5)))
#define ACCU_NOT_FULL			(  PINA & (1<<PA5))

/* if ACCU charging ? */
#define ACCU_IS_CHARGING		(!(PINA & (1<<PA6)))

/* if the Voltage okay ? */
#define VOLTAGE_MONITORING		7

/* Pins from the 595´er */
#define MR_HIGH					PORTD |=  (1<<PD7)
#define MR_LOW					PORTD &= ~(1<<PD7)
#define MR_TOGGLE				PORTD ^=  (1<<PD7)

#define SHCP_HIGH				PORTD |=  (1<<PD6)
#define SHCP_LOW				PORTD &= ~(1<<PD6)
#define SHCP_TOGGLE				PORTD ^=  (1<<PD6)

#define SD_HIGH					PORTD |=  (1<<PD5)
#define SD_LOW					PORTD &= ~(1<<PD5)
#define SD_TOGGLE				PORTD ^=  (1<<PD5)

#define STCP_HIGH				PORTD |=  (1<<PD4)
#define STCP_LOW				PORTD &= ~(1<<PD4)
#define STCP_TOGGLE				PORTD ^=  (1<<PD4)

/* alias INT0 */
#define INTERRUPT_INT0_DISABLE	GICR &= ~(1<<INT0)
#define INTERRUPT_INT0_ENABLE	GICR |=  (1<<INT0)

#define INTERRUPT_TIMER0_ENABLE		TIMSK |= (1<<OCIE0)
#define INTERRUPT_TIMER0_DISABLE	TIMSK &= ~(1<<OCIE0)

/* alias OutputCompareMatch */
#define ENABLE_25ms_ISR 		TIMSK |= (1<<OCIE1A)
#define DISABLE_25ms_ISR		TIMSK &= ~((1<<OCIE1A))   


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include "Font_8x8.h"
#include "adc.h"
#include "74HC(T)595.h"
#include "uart.h"


volatile uint8_t 	speed_flag, Brightness_1 = 1, Brightness_2 = 1, Brightness_3 = 1, Brightness_4 = 1, Brightness_5, Brightness_6, Brightness_7, Brightness_8, Brightness_9, Brightness_10, Brightness_11, Brightness_12, Brightness_13, Brightness_14, Brightness_15, Brightness_16, Result_Random_Generator, Result_Random_Generator_Old, Result_Random_Generator_2, Result_Random_Generator_2_Old, Counter_For_Reading_ADC, Counter_For_Reading_Accu, Switch_One_Flag, Switch_Two_Flag;
volatile uint16_t 	VRAM[16] = {}, Auto_Change_ISR = 0, Auto_Change = 0, Auto_Change_Old, Brightness = 1, Counter, Counter_1, Counter_2, Counter_3, Counter_4, Counter_For_Brightness_1, Counter_For_Brightness_2, Counter_For_Brightness_3, Counter_For_Brightness_4, Counter_For_Brightness_5, Brightness_Flag_1, Brightness_Flag_2, Brightness_flag_3, Brightness_flag_4, Brightness_flag_5, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8, LED_9, LED_10, LED_11, LED_12, LED_13, LED_14, LED_15, Blinking_Accu_Symbol;
volatile uint16_t 	counter_for_Test_Mode, Supply_ADC_Value = 0, Change_Duration = 1, Duration = 6, LED_Status, LED_Status_Old, counter_for_change_accu_symbol;
volatile int16_t 	LED_0, LED_1;
volatile uint16_t 	speed = 500, speed_1 = 800, change_Counter_1 = 500, change_Counter_2 = 1000, Heart_Beat, Long_Key_Pressed, Counter_For_Display, variable_for_check_test_mode;
volatile float 		Sleep_Counter;

/* adress for save LED Status at the EEPROM */
uint8_t EEPROM_save_LED_Status EEMEM = 100;



void go_sleep(void);
void LED_Flare(void);
void Test_Mode(void);
void check_Accu(void);
void fill_screen(void);
void LED_Sparkle(void);
void all_LEDS_ON(void);
void all_LED_OFF(void);
void clear_screen(void); 
void check_overflow(void);
void sleep_after_12h(void);
void check_Test_Mode(void);
void LED_Down(uint16_t speed);
void LED_Round(uint16_t speed);
void LED_Blink(uint16_t speed);
void LED_Round_Test(void);
void Display_Start_String(void);
void Every_Third_LED_Down(void);
void Auto_Change_Simulation(void);
void check_accu_after_100ms(void);
void sleep_after_long_press(void);
void Average_from_Accu_Supply(void);
void Display_all_Symbols(uint8_t i);
void Display_a_Character(uint8_t i);
void LED_Circle_Down(uint16_t speed,uint8_t bright);
void PWM_for_LED(uint8_t LED, uint8_t bright);
void DisplayString(uint16_t x, const char *s);
void LED_Change(uint16_t speed, uint8_t bright);
void LED_Fading(uint16_t Fading_Time_Down, uint16_t fading_time_up);
void Raindrop_Simulation_1(uint8_t Bright_1, uint8_t Bright_2, uint8_t Bright_3, uint8_t Bright_4, uint8_t Bright_5, uint8_t Bright_6, uint8_t Bright_7, uint8_t Bright_8);
void Raindrop_Simulation_2(uint8_t Bright_1, uint8_t Bright_2, uint8_t Bright_3, uint8_t Bright_4, uint8_t Bright_5, uint8_t Bright_6, uint8_t Bright_7, uint8_t Bright_8);

uint8_t GetCharacterIndex(char a);

/* for UART Mode */

#ifdef WITH_UART
	#define UART_BAUD_RATE      9600  
#endif
int main(void)
{
	/* Config the DDRx Register */
	DDRA |= ((1<<PA1) | (1<<PA4));
	DDRC |= ((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5) | (1<<PC6) | (1<<PC7));
	DDRB |= ((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) | (1<<PB5) | (1<<PB6) | (1<<PB7));
	DDRD |= (1<<PD7);
	
	/* set internal PullUps for ACCU_FULL, ACCU_CHARGING and free Pin*/
	PORTA |= (1<<PA5) | (1<<PA6) | (1<<PA0);
	
    /* Prescaler from TIMER0 set at "0"  ! ( PWM & Multiplexing ) */
    TCCR0 |= (1<<CS01)  | (1<<CS00) | (1<<WGM00) | (1<<WGM01);
    
    /* Prescaler set at "64" ( 25 ms base ) */
    TCCR1B |= (1<<CS11) | (1<<CS10) | (1<<WGM12);
	
    /* CountRegister set at "6249" ( set Interrupt at 25 ms base) */
    OCR1A = 6249;
    	
	/* Initialize the ADC Unit */			
	init_ADC();	
		




	/* Initialize the Pins for the 595´ ers */			
	com74hc595_init();
	
	
    /* Interrupts global enable */
    sei();
	
	INTERRUPT_TIMER0_ENABLE;  	
	
#ifdef WITH_START_TEXT
	
	/* show start string */

	Display_Start_String();
	_delay_ms(100);
	
#endif	
	
	/* Interrupt "CompareMatch (OCIE1A)" enable */
	TIMSK |= (1<<OCIE1A);
		
	/* Connect 595´ er GND to Central GND */
	SWITCH_ON_595;
	
    /* read the last Effect */
	LED_Status = eeprom_read_byte (&EEPROM_save_LED_Status);
	
	/* Initialize the UART Unit */ 
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	
	
 	/* Was hat das für einen Sinn ? */
 	Supply_ADC_Value = 900;

	/* set PORTD.7 @ high */
	MR_HIGH;

	

	
 while(1)
 {			
	
		check_Test_Mode();
		
		/* If a Switch long pressed, then the µC go sleep */
		sleep_after_long_press();
		
		/* if the µC >= 12 h at work the go sleep */
		sleep_after_12h();
		
		check_accu_after_100ms();	

									
		switch (LED_Status)
		{
			case 1:
			{
				LED_Sparkle();				
			}
			break;
			
			case 2:
			{
				LED_Flare();
			}
			break;
			
			case 3:
			{
				Raindrop_Simulation_1(40,10,4,2,40,10,4,2);
				Raindrop_Simulation_2(40,10,4,2,40,10,4,2);						
			}
			break;	
			
			case 4:
			{
				LED_Fading(800,800);
			}
			break;	
			
			case 5:
			{
				LED_Circle_Down(5000,8);
			}
			break;	
			
			case 6:
			{
				LED_Down(1000);
			}
			break;	

			case 7:
			{
				LED_Round(4000);
			}
			break;											

			case 8:
			{
				Every_Third_LED_Down();
			}
			break;
		
			case 9:
			{
				LED_Change(2000,8);		
			}
			break;
			
			case 10:
			{
				LED_Round_Test();
			}
			break;		
	
			case 11:
			{
				LED_Blink(300);	
			}
			break;			
			
			case 12:
			{
				Auto_Change_Simulation();
			}
			
			
			
		}	
									
		check_overflow();

 
     }// End while
	
}// End main


/* TIMER1 for the 25 ms base ( Switch Debounce / Auto_Off ) */
ISR(TIMER1_COMPA_vect)
{
	Sleep_Counter ++;
	if (!(ACCU_IS_CHARGING))
	{
		Counter_For_Display ++;
	}
	Counter_For_Reading_ADC ++;
	Counter_For_Reading_Accu ++;
	
	/* variable for AutoChange Simulation 1 - 9 ( 2400 = 1 min )*/
	Auto_Change_ISR ++;
	
	if (LED_Status == 12)
	{
		if (Auto_Change_ISR >= 2400)
		{
			Auto_Change_ISR = 0;
			Auto_Change++;
		}	
	}


	/* change LED_Status after endless Switch one */	
	if (SWITCH_ONE_PRESSED)
	{
		Switch_One_Flag = 1;
	}
	
	if (Switch_One_Flag == 1)
	{
		if (!(SWITCH_ONE_PRESSED))
		{
			Long_Key_Pressed = 0;
			Switch_One_Flag = 0;
			counter_for_Test_Mode = 0;
			LED_Status ++;
		}
	}
	
	if (SWITCH_TWO_PRESSED)
	{
		Switch_Two_Flag = 1;
	}
	
	if (Switch_Two_Flag == 1)
	{
		if (!(SWITCH_TWO_PRESSED))
		{
			if (counter_for_Test_Mode <= 200)
			{
				Long_Key_Pressed = 0;
				counter_for_Test_Mode = 0;
				Switch_Two_Flag = 0;
				LED_Status --;			
			}

		}

	}

	
	/* clear the screen after 2 secounds when LED_Status has changed */
	
	if (!(ACCU_IS_CHARGING))
	{
		if (Counter_For_Display >= 100)
		{
			clear_screen();
		}		
	}

	
	/* sleep after long key pressed */
		
	if (SWITCH_ONE_PRESSED)
	{
		Long_Key_Pressed ++;
	}
	
	if (SWITCH_TWO_PRESSED)
	{
		Long_Key_Pressed ++;
	}



	if (SWITCH_ONE_PRESSED)
	{
		if (SWITCH_TWO_PRESSED)
		{
			Long_Key_Pressed = 0;
			counter_for_Test_Mode ++;
		}
	}	
	
	
	
	
	
	
		
	if (counter_for_Test_Mode >= 200)
	{
		LIVE_LED_ON;
		if (SWITCH_ONE_PRESSED)
		{
			if (!(SWITCH_TWO_PRESSED))
			{
				Long_Key_Pressed = 0;
				variable_for_check_test_mode ++;
			}
		}
		else
		{
			counter_for_Test_Mode = 0;
			variable_for_check_test_mode = 0;
		}
	}







}// End ISR(TIMER1_COMPA_vect)

/* TIMER0 for the multiplex function */
ISR(TIMER0_COMP_vect)
{			
	/* column must declared @ staric volatile */
    static volatile uint8_t column = 0;
    
	/* set the Pins at "low" level */
    PORTC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5) | (1<<PC6) | (1<<PC7));
	//PORTC = 0x00;
	
	/* give out the new Data on 8 x 8 Matrix */
    PORTB = (VRAM)[column];
    
	/* switch the columns ( case 0 are the right column ) */
    switch (column)
    {
	    case 0: PORTC |= (1<<PC7);
	    break;
	    case 1: PORTC |= (1<<PC6);
	    break;
	    case 2: PORTC |= (1<<PC5);
	    break;
	    case 3: PORTC |= (1<<PC4);
	    break;
	    case 4: PORTC |= (1<<PC3);
	    break;
	    case 5: PORTC |= (1<<PC2);
	    break;
	    case 6: PORTC |= (1<<PC1);
	    break;
	    case 7: PORTC |= (1<<PC0);
	    break;
	    default: break;
    }
    
	/* set the next column */
    column++; 
    
	/* check the overflow from column */
    if (column >= 8) column = 0;
	
	
	Heart_Beat ++;
	
	#ifdef WITH_LIVE_LED
	
	/* live the µC ? ( toggle the live led ) */
	if (Heart_Beat == 1000)
	{
		LIVE_LED_ON;
	}
	if (Heart_Beat == 1100)
	{
		LIVE_LED_OFF;
	}
	if (Heart_Beat == 1250)
	{
		LIVE_LED_ON;
	}
	if (Heart_Beat == 1350)
	{
		LIVE_LED_OFF;
		Heart_Beat = 0;
	}

	#endif	
	    
}// End ISR(TIMER0_COMP_vect)

/* must exist (Interrupt Vector Table!) */
ISR(INT0_vect)
{
	// empty
}

/* clear the Screen of 8 x 8 Matrix */
void clear_screen(void)
{
	for(uint8_t i = 0 ; i < 8 ; i++)
	{
		VRAM[i] = 0x00;
	}
}

/* fill the Screen of 8 x 8 Matrix */
void fill_screen(void)
{
	for (uint8_t i = 0 ; i < 8 ; i++)
	{
		VRAM[i] = 0xFF;
	}
}

/* give the Index number of charMap array  */
uint8_t GetCharacterIndex(char a)
{
	uint8_t index = 1;    // alles aus
	
	if ((a >= '0') && (a <= '9'))
	index = a+2-48;  // erste Zahl ist Index in der Tabelle des ersten Zeichens, zweite Zahl ist Index in Ascii Code des ersten Zeichens
	else if ((a >= 'A') && (a <= 'Z'))
	index = a+12-65;
	else if ((a >= 'a') && (a <= 'z'))
	index = a+38-97;
	else if ((a == '.'))
	index = a+70-46;
	else if ((a >= '/'))
	index = a+71-47;
	else if ((a >= '-'))
	index = a+72-45;
	else if ((a >= ' '))
	index = a+1-32;
	
	return index;
}

/* display a String on the 8 x 8 Matrix */
void DisplayString(uint16_t x, const char *s)
{

	uint16_t len;
	uint8_t i, d, u1, u2;
	
	len = strlen(s)*8;
	
	if (x < len)
	{
		
		clear_screen();
		
		i = x >> 3; // Index vom ersten Zeichen
		u1 = GetCharacterIndex(s[i++]); // erstes Zeichen
		u2 = GetCharacterIndex(s[i]);  // zweites Zeichen
		for (d=0 ; d<16 ; d++)
		{
			VRAM[d] = 0xFF;
			VRAM[d] &= ~(pgm_read_byte(&charMap[u1][d]) << (x & 7)); // erstes Zeichen verschoben nach links
			VRAM[d] &= ~(pgm_read_byte(&charMap[u2][d]) >> (8-(x & 7)));  // zweites Zeichen verschoben nach rechts
		}
		
		} else {
		// x zu groß, alles aus
		for (d=0 ; d<8 ; d++)
		VRAM[d] = 0xFF;
		
	}

}

/* use this function for testing Device */
void Test_Mode(void)
{
	clear_screen();
	all_LED_OFF();
	LIVE_LED_OFF;
	DISABLE_25ms_ISR;
	char s[] = " START TEST-MODE";
	for (uint16_t x=0; x<(strlen(s)*8); x++)
	{
		DisplayString(x, s);
		_delay_ms(40);
	}	
		
 	
/* Check all LEDs */
	 
	com74hc595_setall();
 	com74hc595_out();			
	while (!(SWITCH_ONE_PRESSED))
	com74hc595_unsetall();
	com74hc595_out();
	_delay_ms(1000);
	


	com74hc595_setPort(0,0b10101010);
	com74hc595_setPort(1,0b10101010);
	com74hc595_setPort(2,0b10101010);
	com74hc595_setPort(3,0b10101010);
	com74hc595_setPort(4,0b10101010);
	com74hc595_setPort(5,0b10101010);
	com74hc595_setPort(6,0b10101010);
	com74hc595_setPort(7,0b10101010);
	com74hc595_setPort(8,0b10101010);
	com74hc595_setPort(9,0b10101010);
	com74hc595_setPort(10,0b10101010);
	com74hc595_setPort(11,0b10101010);
	com74hc595_setPort(12,0b10101010);
	com74hc595_setPort(13,0b10101010);
	com74hc595_setPort(14,0b10101010);
	com74hc595_setPort(15,0b10101010);
	com74hc595_setPort(16,0b10101010);
	com74hc595_setPort(17,0b10101010);
	com74hc595_out();
	
	while (!(SWITCH_ONE_PRESSED))
	
	com74hc595_unsetall();
	com74hc595_out();
	_delay_ms(1000);

	
	
	while (!(SWITCH_ONE_PRESSED))
	{
		fill_screen();
		_delay_ms(200);
	}
	LIVE_LED_OFF;
	clear_screen();
	_delay_ms(1000);
	

       while (!(SWITCH_ONE_PRESSED))
       {
	       
	       /* from right to left */
	       for (uint16_t x = 1 ; x <= 255 ; x = x * 2)
	       {
		       VRAM[0] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from left to right */
	       for (uint16_t x = 128 ; x >= 1 ; x = x / 2)
	       {
		       VRAM[1] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from right to left */
	       for (uint16_t x = 1 ; x <= 255 ; x = x * 2)
	       {
		       VRAM[2] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from left to right */
	       for (uint16_t x = 128 ; x >= 1 ; x = x / 2)
	       {
		       VRAM[3] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from right to left */
	       for (uint16_t x = 1 ; x <= 255 ; x = x * 2)
	       {
		       VRAM[4] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from left to right */
	       for (uint16_t x = 128 ; x >= 1 ; x = x / 2)
	       {
		       VRAM[5] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from right to left */
	       for (uint16_t x = 1 ; x <= 255 ; x = x * 2)
	       {
		       VRAM[6] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
	       /* from left to right */
	       for (uint16_t x = 128 ; x >= 1 ; x = x / 2)
	       {
		       VRAM[7] = x;
		       _delay_ms(100);
		       clear_screen();
	       }
	       
       }
       ENABLE_25ms_ISR;
	   counter_for_Test_Mode = 0;
}

/* display all Symbols on the 8 x 8 Font from charMap */
void Display_all_Symbols(uint8_t i)
{

		for (i = 0 ; i < 72 ; i++)
		{
			
			for (uint8_t d= 0 ; d < 8 ; d++)
			{
				VRAM[d] = ~(pgm_read_byte(&charMap[i][d]));
			}

			_delay_ms(100);
		}// Ende for			

}

/* display one of Character from charMap */
void Display_a_Character(uint8_t i)
{
	for (uint8_t d= 0 ; d < 8 ; d++)
	{
		VRAM[d] = ~(pgm_read_byte(&charMap[i][d]));
	}
}

/* display start String */
void Display_Start_String(void)
{
	LIVE_LED_OFF;

	char s[] = " Xmas-Tree-Xtreeme";
		    
	for (uint16_t x=0; x<(strlen(s)*8); x++)
	{
		DisplayString(x, s);
		_delay_ms(100);
	}
	

}

/* show all LED´s */
void all_LEDS_ON()
{
	com74hc595_setall();
	com74hc595_out();
}

/* show no LED */
void all_LED_OFF()
{
	com74hc595_unsetall();
	com74hc595_out();
}

/* sleep Mode */
void go_sleep(void)
{
	/* all LED´s & 8 x 8 pixel off */
	all_LED_OFF();
	LIVE_LED_OFF;
	
	/* setzt die Flag zurück, damit der LED_Status nicht hoch- oder runtergezählt wird */
	Switch_One_Flag = 0;
	Switch_Two_Flag = 0;
	
#ifdef WITH_SLEEP_INFO
	
	DISABLE_25ms_ISR;
	char s[] = " Sleep";
		
	for (uint16_t x=0; x<(strlen(s)*8); x++)
	{
		DisplayString(x, s);
		_delay_ms(100);
	}

#endif	
	
	_delay_ms(100);
	clear_screen();
	
	/* disable a part of X- Mas Tree Hardware */
	SWITCH_OFF_595;
	
	/* disable the ADC Unit ( save 330nA ) */
	Disable_ADC();
		
	/* choose the sleep mode */
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	/* set the DDRx´s as Input */
 	DDRA &= ~(0xFF);
 	DDRD &= ~(0xFF);

	/* enable the INT0 Interrupt */
	INTERRUPT_INT0_ENABLE;
	
	/* short delay for initialize the DDR´s */
	for (uint8_t x = 0 ; x < 250 ; x++)
	
	/* enable the sleep mode */
	sleep_enable();
	
	/* CPU go sleep */
	sleep_cpu();
	
	/* if a Interrupt, then exit the sleep mode */
	sleep_disable();
	
	/* short delay for initialize the DDR´s */
	for (uint8_t x = 0 ; x < 250 ; x++)

	/* Config the DDRx Register */
	DDRA |= ((1<<PA1) | (1<<PA4));
	DDRC |= ((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5) | (1<<PC6) | (1<<PC7));
	DDRB |= ((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) | (1<<PB5) | (1<<PB6) | (1<<PB7));
	DDRD |= ((1<<PD7));
	
	/* MR Pin from 595´er @ high level */
	MR_HIGH;
	
	/* enable a part of Hardware */
	SWITCH_ON_595;
	
	/* Initialize the ADC Unit */
	init_ADC();
	
	/* Initialize the Pins for the 595´ ers */
	com74hc595_init();
	
	
	
	/* disable Interrupt "INT0" */
	INTERRUPT_INT0_DISABLE;

	/* set the variable @ "0" */
	Sleep_Counter = 0;	
	Long_Key_Pressed = 0;
	Counter_For_Display = 0;
		
#ifdef WITH_SLEEP_INFO
	
	char a[] = " Live";
	
	for (uint16_t x=0; x<(strlen(a)*8); x++)
	{
		DisplayString(x, a);
		_delay_ms(100);
	}
	
	ENABLE_25ms_ISR;

	
	/* Irgendeine Zahl, damit der Menuepunkt neu konfiguriert wird */
	while (LED_Status == LED_Status_Old)
	{
		LED_Status_Old = (rand() % 20)+10;
	}
	
	
#endif
	

}

/* check Accu live */
void check_Accu(void)
{
	
	/* read the Supply Voltage from the Accu */	
	Average_from_Accu_Supply();
	
	
	if (!(ACCU_IS_CHARGING))
	{		
		if (Supply_ADC_Value <= 710)
		{
			DISABLE_25ms_ISR;
 			all_LED_OFF();		
 			Counter_For_Display = 0;	
 			Blinking_Accu_Symbol = 0;	
			Display_a_Character(65);	
		
			/* generating a blink effect when the Accu has "3,35V" Power Supply */			
		
			while (Supply_ADC_Value <= 800)
			{
				Counter_For_Reading_ADC ++;
				Average_from_Accu_Supply();
				Blinking_Accu_Symbol ++;
				_delay_ms(1);
			
				if (Blinking_Accu_Symbol == 300)
				{
					clear_screen();
				}
				_delay_ms(1);
				if (Blinking_Accu_Symbol == 600)
				{
					Blinking_Accu_Symbol = 0;
					Display_a_Character(65);
				}
				/* leave while, if accu is charging */
				if (ACCU_IS_CHARGING)
				{
					Supply_ADC_Value = 900;
				}		
				// sleep, when battery has "3,32V" Power Supply
				if (Supply_ADC_Value <= 700)
				{
					DISABLE_25ms_ISR;
					all_LED_OFF();
					clear_screen();
					
					char s[] = " save energy";
					
					for (uint16_t x=0; x<(strlen(s)*8); x++)
					{
						DisplayString(x, s);
						_delay_ms(80);
					}
					ENABLE_25ms_ISR;				
					go_sleep();
					DISABLE_25ms_ISR;
				}
					
			}
		
		clear_screen();	
		Counter_For_Display = 0;
		Blinking_Accu_Symbol = 0;
		ENABLE_25ms_ISR;
		}
	}




	if (ACCU_IS_CHARGING)
	{				
		counter_for_change_accu_symbol ++;
		if (counter_for_change_accu_symbol == 10)
		{
			Display_a_Character(65);
		}
		if (counter_for_change_accu_symbol == 20)
		{
			Display_a_Character(66);
		}
		if (counter_for_change_accu_symbol == 30)
		{
			Display_a_Character(67);
		}
		if (counter_for_change_accu_symbol == 40)
		{
			Display_a_Character(68);
		}
		if (counter_for_change_accu_symbol >= 50)
		{
			Display_a_Character(69);
			counter_for_change_accu_symbol = 0;
		}
			
		Sleep_Counter = 0;
		Counter_For_Display = 0;			
	}// Ende if		
	

	
	
	if (ACCU_FULL)
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		Counter_For_Display = 0;
		Blinking_Accu_Symbol = 0;
		Display_a_Character(69);
	

		while (ACCU_FULL)
		{
			for (uint16_t x = 0; x <= 600; x++)
			{
				Counter_For_Display = 0;
				Blinking_Accu_Symbol ++;
				_delay_ms(1);
			
				if (Blinking_Accu_Symbol == 300)
				{
					clear_screen();
				}
				_delay_ms(1);
				if (Blinking_Accu_Symbol == 600)
				{
					Blinking_Accu_Symbol = 0;
					Display_a_Character(69);
				}			
			}

		}
		clear_screen();
		Counter_For_Display = 0;
		Blinking_Accu_Symbol = 0;
		ENABLE_25ms_ISR;
	}
	
}

/* fading all LED´s */
void LED_Fading(uint16_t Fading_Time_Down, uint16_t fading_time_up)
{	
	if (LED_Status != LED_Status_Old)
	{
		Brightness = 1;
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		Counter_For_Brightness_1 = 0;
		Brightness_Flag_1 = 2;
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;	
		if (LED_Status != 12)
		{
			Display_a_Character(6);
		}			
			
	}
	
	Counter ++;
	Counter_For_Brightness_1++;
		
	
	if (Brightness == 15)
	{
		Brightness_Flag_1 = 1;
	}
	if (Brightness == 1)
	{
		Brightness_Flag_1 = 2;
	}
	
	
	
	/* fading "down" */	
	if (Brightness_Flag_1 == 1)
	{
		if (Counter_For_Brightness_1 == Fading_Time_Down)
		{
			Counter_For_Brightness_1 = 0;
			Brightness --;
		}
	}
	
	
	/* fading "up" */	
	if (Brightness_Flag_1 == 2)
	{
		if (Counter_For_Brightness_1 == fading_time_up)
		{
			Counter_For_Brightness_1 = 0;
			Brightness ++;
		}
	}

	/* PWM routine */
	if (Counter >= 60)
	{
		Counter = 0;
		
		if (Brightness > 0)
		{
			com74hc595_setall();
		}
	}
	else
	{
		if (Counter >= Brightness)
		{
			com74hc595_unsetall();
		}
	}
	com74hc595_out();	
}

/* simulated raindrops Version "1" */
void Raindrop_Simulation_1(uint8_t Bright_1, uint8_t Bright_2, uint8_t Bright_3, uint8_t Bright_4, uint8_t Bright_5, uint8_t Bright_6, uint8_t Bright_7, uint8_t Bright_8)
{	
	
	if (LED_Status != LED_Status_Old)
	{
		Counter, Counter_1, Counter_2, Counter_3 = 0;
		
		all_LED_OFF();
		
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);	
		
		Brightness_1, Brightness_2, Brightness_3, Brightness_4, Brightness_5, Brightness_6, Brightness_7, Brightness_8, Brightness_9, Brightness_10, Brightness_11, Brightness_12, Brightness_13, Brightness_14, Brightness_15, Brightness_16, Counter_For_Display = 0;
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
		LED_3 = 3;
		LED_4 = 12;
		LED_5 = 13;
		LED_6 = 14;
		LED_7 = 15;
		LED_8 = 72;
		LED_9 = 73;
		LED_10 = 74;
		LED_11 = 75;
		LED_12 = 84;
		LED_13 = 85;
		LED_14 = 86;
		LED_15 = 87;	
		Result_Random_Generator     = (rand() % 6)+1;
		Result_Random_Generator_2	= (rand() % 6)+1;	
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(5);
		}				
	}	
	
	
	Brightness_1 ++;
	Brightness_2 ++;
	Brightness_3 ++;
	Brightness_4 ++;
	Brightness_5 ++;
	Brightness_6 ++;
	Brightness_7 ++;
	Brightness_8 ++;
	
	
	
	/* Einmalige Einstellung der LEDs nach dem Wechsel des Baumteils */
	if (Result_Random_Generator != Result_Random_Generator_Old)
	{
		Result_Random_Generator_Old = Result_Random_Generator;
		
		switch (Result_Random_Generator)
		{
			case 1:
			{
				LED_0 = 0;
				LED_1 = 1;
				LED_2 = 2;
				LED_3 = 3;
				LED_4 = 12;
				LED_5 = 13;
				LED_6 = 14;
				LED_7 = 15;						
			}
			break;
			
			case 2:
			{
				LED_0 = 24;
				LED_1 = 25;
				LED_2 = 26;
				LED_3 = 27;
				LED_4 = 36;
				LED_5 = 37;
				LED_6 = 38;
				LED_7 = 39;				
			}
			break;

			case 3:
			{
				LED_0 = 48;
				LED_1 = 49;
				LED_2 = 50;
				LED_3 = 51;
				LED_4 = 60;
				LED_5 = 61;
				LED_6 = 62;
				LED_7 = 63;
			}
			break;			

			case 4:
			{
				LED_0 = 72;
				LED_1 = 73;
				LED_2 = 74;
				LED_3 = 75;
				LED_4 = 84;
				LED_5 = 85;
				LED_6 = 86;
				LED_7 = 87;
			}
			break;			
		
			case 5:
			{
				LED_0 = 96;
				LED_1 = 97;
				LED_2 = 98;
				LED_3 = 99;
				LED_4 = 108;
				LED_5 = 109;
				LED_6 = 110;
				LED_7 = 111;
			}
			break;		

			case 6:
			{
				LED_0 = 120;
				LED_1 = 121;
				LED_2 = 122;
				LED_3 = 123;
				LED_4 = 132;
				LED_5 = 133;
				LED_6 = 134;
				LED_7 = 135;
			}
			break;
		}

	}
		
	
	
	
	/* Zufälliger Wechsel zwischen den verschiedenen Baumteilen */		
	switch (LED_0)
	{
		case 12:
		{
			Result_Random_Generator = (rand() % 6)+1;
		}
		break;
		
		case 36:
		{
			Result_Random_Generator = (rand() % 6)+1;			
		}
		break;	
		
		case 60:
		{
			Result_Random_Generator = (rand() % 6)+1;					
		}
		break;
		
		case 84:
		{
			Result_Random_Generator = (rand() % 6)+1;			
		}
		break;
		
		case 108:
		{
			Result_Random_Generator = (rand() % 6)+1;						
		}
		break;	
		
		case 132:
		{	
			Result_Random_Generator = (rand() % 6)+1;		
		}
		break;								
	}
		
		
	/************* PWM-Routine *************/
	switch (Result_Random_Generator)
	{
		case 1:
		{
			if (LED_0 <= 11)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
			
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 11)
			{	
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 11)
			{			
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 11)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}	

			if (LED_4 <= 23)
			{			
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 23)
			{						
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 23)
			{						
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}	
				
			if (LED_7 <= 23)
			{						
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}		
			}
		}
		break;
		
		
		case 2:
		{
			if (LED_0 <= 35)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 35)
			{
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 35)
			{
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 35)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}

			if (LED_4 <= 47)
			{
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 47)
			{
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 47)
			{
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}
			
			if (LED_7 <= 47)
			{
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}
			}
		}
		break;	
		
		
		case 3:
		{
			if (LED_0 <= 59)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 59)
			{
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 59)
			{
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 59)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}

			if (LED_4 <= 71)
			{
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 71)
			{
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 71)
			{
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}
			
			if (LED_7 <= 71)
			{
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}
			}
		}
		break;	
		
		
		case 4:
		{
			if (LED_0 <= 83)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 83)
			{
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 83)
			{
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 83)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}

			if (LED_4 <= 95)
			{
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 95)
			{
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 95)
			{
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}
			
			if (LED_7 <= 95)
			{
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}
			}
		}
		break;	
		
		
		case 5:
		{
			if (LED_0 <= 107)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 107)
			{
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 107)
			{
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 107)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}

			if (LED_4 <= 119)
			{
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 119)
			{
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 119)
			{
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}
			
			if (LED_7 <= 119)
			{
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}
			}
		}
		break;					
		
		
		case 6:
		{
			if (LED_0 <= 131)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_1 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_0);
					Brightness_1 = 0;
				}
			}
			
			if (LED_1 <= 131)
			{
				if (Brightness_2 == Bright_2)
				{
					com74hc595_setBit(LED_1);
					Brightness_2 = 0;
				}
			}
			if (LED_2 <= 131)
			{
				if (Brightness_3 == Bright_3)
				{
					com74hc595_setBit(LED_2);
					Brightness_3 = 0;
				}
			}
			
			if (LED_3 <= 131)
			{
				if (Brightness_4 == Bright_4)
				{
					com74hc595_setBit(LED_3);
					Brightness_4 = 0;
				}
			}

			if (LED_4 <= 143)
			{
				if (Brightness_5 == Bright_5)
				{
					com74hc595_setBit(LED_4);
					Brightness_5 = 0;
				}
			}

			if (LED_5 <= 143)
			{
				if (Brightness_6 == Bright_6)
				{
					com74hc595_setBit(LED_5);
					Brightness_6 = 0;
				}
			}
			
			if (LED_6 <= 143)
			{
				if (Brightness_7 == Bright_7)
				{
					com74hc595_setBit(LED_6);
					Brightness_7 = 0;
				}
			}
			
			if (LED_7 <= 143)
			{
				if (Brightness_8 == Bright_8)
				{
					com74hc595_setBit(LED_7);
					Brightness_8= 0;
				}
			}
		}
		break;		
	}

		/* If-Funktionen werden benötigt, weil die letzten LEDs auf dem letzten Baum sonst nicht sauber erlischen */
		if (LED_0 == 144)
		{
			LED_0 = 1000;
		}
		if (LED_1 == 144)
		{
			LED_1 = 1000;
		}
		if (LED_2 == 144)
		{
			LED_2 = 1000;
		}
		if (LED_3 == 144)
		{
			LED_3 = 1000;
		}		
		if (LED_4 == 144)
		{
			LED_4 = 1000;
		}
		if (LED_5 == 144)
		{
			LED_5 = 1000;
		}
		if (LED_6 == 144)
		{
			LED_6 = 1000;
		}
		if (LED_7 == 144)
		{
			LED_7 = 1000;
		}																	

							
		com74hc595_out();
		com74hc595_unsetBit(LED_0);
		com74hc595_unsetBit(LED_1);
		com74hc595_unsetBit(LED_2);	
		com74hc595_unsetBit(LED_3);
		com74hc595_unsetBit(LED_4);
		com74hc595_unsetBit(LED_5);	
		com74hc595_unsetBit(LED_6);	
		com74hc595_unsetBit(LED_7);	

		/* Counter zur Verzgerung des Schiebens der Brightnesssstufe zu den LED's */
		Counter ++;
		
		
		/* Counter_1 dient der Zufälligen Geschwindigkeit der Tropfensimulation */
		Counter_1 ++;
	
		/* change_Counter_1 dient der zufälligen Änderung der Geschwindigkeit für die Tropfensimulation nach einer zufälligen Zeit */
		if (Counter_1 == change_Counter_1)
		{
			change_Counter_1 = (rand() % 4000)+500;
			Counter_1 = 0;
			speed = (rand() % 700)+200;
		}
					
		/* Schiebevorgang der Brightnesssstufen von einer LED zur nächsten*/
		if (Counter >= speed)
		{
			Counter = 0;
			LED_0 ++;
			LED_1 ++;
			LED_2 ++;
			LED_3 ++;
			LED_4 ++;
			LED_5 ++;
			LED_6 ++;
			LED_7 ++;
		}
}

/* simulated raindrops Version "2" */
void Raindrop_Simulation_2(uint8_t Bright_1, uint8_t Bright_2, uint8_t Bright_3, uint8_t Bright_4, uint8_t Bright_5, uint8_t Bright_6, uint8_t Bright_7, uint8_t Bright_8)
{
	
	Brightness_9 ++;
	Brightness_10 ++;
	Brightness_11 ++;
	Brightness_12 ++;
	Brightness_13 ++;
	Brightness_14 ++;
	Brightness_15 ++;
	Brightness_16 ++;
	
	/* Einmalige Einstellung der LEDs nach dem Wechsel des Baumteils */
	
	if (Result_Random_Generator_2 != Result_Random_Generator_2_Old)
	{
		Result_Random_Generator_2_Old = Result_Random_Generator_2;
		
		switch (Result_Random_Generator_2)
		{
			case 1:
			{
				LED_8 = 0;
				LED_9 = 1;
				LED_10 = 2;
				LED_11 = 3;
				LED_12 = 12;
				LED_13 = 13;
				LED_14 = 14;
				LED_15 = 15;
			}
			break;
			
			case 2:
			{
				LED_8 = 24;
				LED_9 = 25;
				LED_10 = 26;
				LED_11 = 27;
				LED_12 = 36;
				LED_13 = 37;
				LED_14 = 38;
				LED_15 = 39;
			}
			break;

			case 3:
			{
				LED_8 = 48;
				LED_9 = 49;
				LED_10 = 50;
				LED_11 = 51;
				LED_12 = 60;
				LED_13 = 61;
				LED_14 = 62;
				LED_15 = 63;
			}
			break;

			case 4:
			{
				LED_8 = 72;
				LED_9 = 73;
				LED_10 = 74;
				LED_11 = 75;
				LED_12 = 84;
				LED_13 = 85;
				LED_14 = 86;
				LED_15 = 87;
			}
			break;
			
			case 5:
			{
				LED_8 = 96;
				LED_9 = 97;
				LED_10 = 98;
				LED_11 = 99;
				LED_12 = 108;
				LED_13 = 109;
				LED_14 = 110;
				LED_15 = 111;
			}
			break;

			case 6:
			{
				LED_8 = 120;
				LED_9 = 121;
				LED_10 = 122;
				LED_11 = 123;
				LED_12 = 132;
				LED_13 = 133;
				LED_14 = 134;
				LED_15 = 135;
			}
			break;
		}

	}
	
	
	
	
	/* Zufälliger Wechsel zwischen den verschiedenen Baumteilen */
	
	switch (LED_8)
	{
		case 12:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
		
		case 36:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
		
		case 60:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
		
		case 84:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
		
		case 108:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
		
		case 132:
		{
			Result_Random_Generator_2 = (rand() % 6)+1;
		}
		break;
	}
	
	
	/************* PWM-Routine *************/
	switch (Result_Random_Generator_2)
	{
		case 1:
		{
			if (LED_8 <= 11)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 11)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 11)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 11)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 23)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 23)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 23)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 23)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16= 0;
				}
			}
		}
		break;
		
		
		case 2:
		{
			if (LED_8 <= 35)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 35)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 35)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 35)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 47)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 47)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 47)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 47)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16 = 0;
				}
			}
		}
		break;
		
		
		case 3:
		{
			if (LED_8 <= 59)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 59)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 59)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 59)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 71)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 71)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 71)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 71)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16 = 0;
				}
			}
		}
		break;
		
		
		case 4:
		{
			if (LED_8 <= 83)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 83)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 83)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 83)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 95)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 95)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 95)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 95)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16 = 0;
				}
			}
		}
		break;
		
		
		case 5:
		{
			if (LED_8 <= 107)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 107)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 107)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 107)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 119)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 119)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 119)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 119)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16 = 0;
				}
			}
		}
		break;
		
		
		case 6:
		{
			if (LED_8 <= 131)							// Funktion dient dazu, dass die LED beim Erreichen der untersten LED hintereinander erlöschen
			{
				
				if (Brightness_9 == Bright_1)			// Je höher der Wert von Brightness abgefragt wird, desto dunkeler erscheint die LED
				{
					com74hc595_setBit(LED_8);
					Brightness_9 = 0;
				}
			}
			
			if (LED_9 <= 131)
			{
				if (Brightness_10 == Bright_2)
				{
					com74hc595_setBit(LED_9);
					Brightness_10 = 0;
				}
			}
			if (LED_10 <= 131)
			{
				if (Brightness_11 == Bright_3)
				{
					com74hc595_setBit(LED_10);
					Brightness_11 = 0;
				}
			}
			
			if (LED_11 <= 131)
			{
				if (Brightness_12 == Bright_4)
				{
					com74hc595_setBit(LED_11);
					Brightness_12 = 0;
				}
			}

			if (LED_12 <= 143)
			{
				if (Brightness_13 == Bright_5)
				{
					com74hc595_setBit(LED_12);
					Brightness_13 = 0;
				}
			}

			if (LED_13 <= 143)
			{
				if (Brightness_14 == Bright_6)
				{
					com74hc595_setBit(LED_13);
					Brightness_14 = 0;
				}
			}
			
			if (LED_14 <= 143)
			{
				if (Brightness_15 == Bright_7)
				{
					com74hc595_setBit(LED_14);
					Brightness_15 = 0;
				}
			}
			
			if (LED_15 <= 143)
			{
				if (Brightness_16 == Bright_8)
				{
					com74hc595_setBit(LED_15);
					Brightness_16 = 0;
				}
			}
		}
		break;
	}

	/* If-Funktionen werden benötigt, weil die letzten LEDs auf dem letzten Baum sonst nicht sauber erlischen */

	if (LED_8 == 144)
	{
		LED_8 = 5000;
	}
	if (LED_9 == 144)
	{
		LED_9 = 5000;
	}
	if (LED_10 == 144)
	{
		LED_10 = 5000;
	}
	if (LED_11 == 144)
	{
		LED_11 = 5000;
	}
	if (LED_12 == 144)
	{
		LED_12 = 5000;
	}
	if (LED_13 == 144)
	{
		LED_13 = 5000;
	}			
	if (LED_14 == 144)
	{
		LED_14 = 5000;
	}
	if (LED_15 == 144)
	{
		LED_15 = 5000;
	}

	
	com74hc595_out();
	com74hc595_unsetBit(LED_8);
	com74hc595_unsetBit(LED_9);
	com74hc595_unsetBit(LED_10);
	com74hc595_unsetBit(LED_11);
	com74hc595_unsetBit(LED_12);
	com74hc595_unsetBit(LED_13);
	com74hc595_unsetBit(LED_14);
	com74hc595_unsetBit(LED_15);

	/* Counter zur Verzgerung des Schiebens der Brightnesssstufe zu den LED's */
	Counter_2 ++;
	
	
	/* Counter_1 dient der Zufälligen Geschwindigkeit der Tropfensimulation */
	
	Counter_3 ++;
	
	/* change_Counter_1 dient der zufälligen Änderung der Geschwindigkeit für die Tropfensimulation nach einer zufälligen Zeit */
	
	if (Counter_3 == change_Counter_2)
	{
		change_Counter_2 = (rand() % 4000)+500;
		Counter_3 = 0;
		speed_1 = (rand() % 700)+200;
	}
	
	/* Schiebevorgang der Brightnesssstufen von einer LED zur nächsten*/
	if (Counter_2 >= speed_1)
	{
		Counter_2 = 0;
		LED_8 ++;
		LED_9 ++;
		LED_10 ++;
		LED_11 ++;
		LED_12 ++;
		LED_13 ++;
		LED_14 ++;
		LED_15 ++;
	}
}

/* simulated LED sparkle */
void LED_Sparkle(void)
{

	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		Brightness_1 = 8;
		Brightness_2 = 1;
		Brightness_3 = 1;
		Brightness_4 = 8;
		Brightness_5 = 8;
		Counter_For_Brightness_1 = 0;
		Counter_For_Brightness_2 = 0;
		Counter_For_Brightness_3 = 0;
		Counter_For_Brightness_4 = 0;
		Counter_For_Brightness_5 = 0;	
		Counter = 0;
		Counter_1 = 0;
		Counter_2 = 0;
		Counter_3 = 0;
		Counter_4 = 0;
		all_LED_OFF();
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(3);
		}			
	}
	
	Counter ++;
	Counter_1 ++;
	Counter_2 ++;
	Counter_3 ++;
	Counter_4 ++;
	
	Counter_For_Brightness_1 ++;
	Counter_For_Brightness_2 ++;
	Counter_For_Brightness_3 ++;
	Counter_For_Brightness_4 ++;
	Counter_For_Brightness_5 ++;

	
	if (Brightness_1 == 8)
	{
		Brightness_Flag_1 = 1;
	}
	if (Brightness_1 == 1)
	{
		Brightness_Flag_1 = 2;
	}
	
	
	
	/* Runter-Faden */
	if (Brightness_Flag_1 == 1)
	{
		if (Counter_For_Brightness_1 == 1100)
		{
			Counter_For_Brightness_1 = 0;
			Brightness_1 --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_Flag_1 == 2)
	{
		if (Counter_For_Brightness_1 == 1100)
		{
			Counter_For_Brightness_1 = 0;
			Brightness_1 ++;
		}
	}

	/* PWM-Routine */
	if (Counter >= 75)
	{
		Counter = 0;
		
		if (Brightness_1 > 0)
		{
			com74hc595_setBit(0);
			com74hc595_setBit(5);
			com74hc595_setBit(10);
			com74hc595_setBit(15);
			com74hc595_setBit(20);
			com74hc595_setBit(25);
			com74hc595_setBit(30);
			com74hc595_setBit(35);
			com74hc595_setBit(40);
			com74hc595_setBit(45);
			com74hc595_setBit(50);
			com74hc595_setBit(55);
			com74hc595_setBit(60);
			com74hc595_setBit(65);
			com74hc595_setBit(70);
			com74hc595_setBit(75);
			com74hc595_setBit(80);
			com74hc595_setBit(85);
			com74hc595_setBit(90);
			com74hc595_setBit(95);
			com74hc595_setBit(100);
			com74hc595_setBit(105);
			com74hc595_setBit(110);
			com74hc595_setBit(115);
			com74hc595_setBit(120);
			com74hc595_setBit(125);
			com74hc595_setBit(130);
			com74hc595_setBit(135);
			com74hc595_setBit(140);
		}
	}
	else
	{
		if (Counter == Brightness_1)
		{
			com74hc595_unsetBit(0);
			com74hc595_unsetBit(5);
			com74hc595_unsetBit(10);
			com74hc595_unsetBit(15);
			com74hc595_unsetBit(20);
			com74hc595_unsetBit(25);
			com74hc595_unsetBit(30);
			com74hc595_unsetBit(35);
			com74hc595_unsetBit(40);
			com74hc595_unsetBit(45);
			com74hc595_unsetBit(50);
			com74hc595_unsetBit(55);
			com74hc595_unsetBit(60);
			com74hc595_unsetBit(65);
			com74hc595_unsetBit(70);
			com74hc595_unsetBit(75);
			com74hc595_unsetBit(80);
			com74hc595_unsetBit(85);
			com74hc595_unsetBit(90);
			com74hc595_unsetBit(95);
			com74hc595_unsetBit(100);
			com74hc595_unsetBit(105);
			com74hc595_unsetBit(110);
			com74hc595_unsetBit(115);
			com74hc595_unsetBit(120);
			com74hc595_unsetBit(125);
			com74hc595_unsetBit(130);
			com74hc595_unsetBit(135);
			com74hc595_unsetBit(140);			
		}
	}


	if (Brightness_2 == 8)
	{
		Brightness_Flag_2 = 1;
	}
	if (Brightness_2 == 1)
	{
		Brightness_Flag_2 = 2;
	}
	
	
	
	/* Runter-Faden */
	if (Brightness_Flag_2 == 1)
	{
		if (Counter_For_Brightness_2 == 700)
		{
			Counter_For_Brightness_2 = 0;
			Brightness_2 --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_Flag_2 == 2)
	{
		if (Counter_For_Brightness_2 == 700)
		{
			Counter_For_Brightness_2 = 0;
			Brightness_2 ++;
		}
	}

	/* PWM-Routine */
	if (Counter_1 >= 75)
	{
		Counter_1 = 0;
		
		if (Brightness_2 > 0)
		{
			com74hc595_setBit(1);
			com74hc595_setBit(6);
			com74hc595_setBit(11);
			com74hc595_setBit(16);
			com74hc595_setBit(21);
			com74hc595_setBit(26);
			com74hc595_setBit(31);
			com74hc595_setBit(36);
			com74hc595_setBit(41);
			com74hc595_setBit(46);
			com74hc595_setBit(51);
			com74hc595_setBit(56);
			com74hc595_setBit(61);
			com74hc595_setBit(66);
			com74hc595_setBit(71);
			com74hc595_setBit(76);
			com74hc595_setBit(81);
			com74hc595_setBit(86);
			com74hc595_setBit(91);
			com74hc595_setBit(96);
			com74hc595_setBit(101);
			com74hc595_setBit(106);
			com74hc595_setBit(111);
			com74hc595_setBit(116);
			com74hc595_setBit(121);
			com74hc595_setBit(126);
			com74hc595_setBit(131);
			com74hc595_setBit(136);
			com74hc595_setBit(141);			
		}
	}
	else
	{
		if (Counter_1 == Brightness_2)
		{
			com74hc595_unsetBit(1);
			com74hc595_unsetBit(6);
			com74hc595_unsetBit(11);
			com74hc595_unsetBit(16);
			com74hc595_unsetBit(21);
			com74hc595_unsetBit(26);
			com74hc595_unsetBit(31);
			com74hc595_unsetBit(36);
			com74hc595_unsetBit(41);
			com74hc595_unsetBit(46);
			com74hc595_unsetBit(51);
			com74hc595_unsetBit(56);
			com74hc595_unsetBit(61);
			com74hc595_unsetBit(66);
			com74hc595_unsetBit(71);
			com74hc595_unsetBit(76);
			com74hc595_unsetBit(81);
			com74hc595_unsetBit(86);
			com74hc595_unsetBit(91);
			com74hc595_unsetBit(96);
			com74hc595_unsetBit(101);
			com74hc595_unsetBit(106);
			com74hc595_unsetBit(111);
			com74hc595_unsetBit(116);
			com74hc595_unsetBit(121);
			com74hc595_unsetBit(126);
			com74hc595_unsetBit(131);
			com74hc595_unsetBit(136);
			com74hc595_unsetBit(141);
		}
	}


	if (Brightness_3 >= 8)
	{
		Brightness_flag_3 = 1;
	}
	if (Brightness_3 <= 1)
	{
		Brightness_flag_3 = 2;
	}
	
	
	
	/* Runter-Faden */
	if (Brightness_flag_3 == 1)
	{
		if (Counter_For_Brightness_3 == 900)
		{
			Counter_For_Brightness_3 = 0;
			Brightness_3 --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_flag_3 == 2)
	{
		if (Counter_For_Brightness_3 == 900)
		{
			Counter_For_Brightness_3 = 0;
			Brightness_3 ++;
		}
	}

	/* PWM-Routine */
	if (Counter_2 >= 75)
	{
		Counter_2 = 0;
		
		if (Brightness_3 > 0)
		{
			com74hc595_setBit(2);
			com74hc595_setBit(7);
			com74hc595_setBit(12);
			com74hc595_setBit(17);
			com74hc595_setBit(22);
			com74hc595_setBit(27);
			com74hc595_setBit(32);
			com74hc595_setBit(37);
			com74hc595_setBit(42);
			com74hc595_setBit(47);
			com74hc595_setBit(52);
			com74hc595_setBit(57);
			com74hc595_setBit(62);
			com74hc595_setBit(67);
			com74hc595_setBit(72);
			com74hc595_setBit(77);
			com74hc595_setBit(82);
			com74hc595_setBit(87);
			com74hc595_setBit(92);
			com74hc595_setBit(97);
			com74hc595_setBit(102);
			com74hc595_setBit(107);
			com74hc595_setBit(112);
			com74hc595_setBit(117);
			com74hc595_setBit(122);
			com74hc595_setBit(127);
			com74hc595_setBit(132);
			com74hc595_setBit(137);
			com74hc595_setBit(142);		
		}
	}
	else
	{
		if (Counter_2 == Brightness_3)
		{
			com74hc595_unsetBit(2);
			com74hc595_unsetBit(7);
			com74hc595_unsetBit(12);
			com74hc595_unsetBit(17);
			com74hc595_unsetBit(22);
			com74hc595_unsetBit(27);
			com74hc595_unsetBit(32);
			com74hc595_unsetBit(37);
			com74hc595_unsetBit(42);
			com74hc595_unsetBit(47);
			com74hc595_unsetBit(52);
			com74hc595_unsetBit(57);
			com74hc595_unsetBit(62);
			com74hc595_unsetBit(67);
			com74hc595_unsetBit(72);
			com74hc595_unsetBit(77);
			com74hc595_unsetBit(82);
			com74hc595_unsetBit(87);
			com74hc595_unsetBit(92);
			com74hc595_unsetBit(97);
			com74hc595_unsetBit(102);
			com74hc595_unsetBit(107);
			com74hc595_unsetBit(112);
			com74hc595_unsetBit(117);
			com74hc595_unsetBit(122);
			com74hc595_unsetBit(127);
			com74hc595_unsetBit(132);
			com74hc595_unsetBit(137);
			com74hc595_unsetBit(142);			
		}
	}



	if (Brightness_4 >= 8)
	{
		Brightness_flag_4 = 1;
	}
	if (Brightness_4 <= 1)
	{
		Brightness_flag_4 = 2;
	}
	
	
	
	/* Runter-Faden */
	if (Brightness_flag_4 == 1)
	{
		if (Counter_For_Brightness_4 == 600)
		{
			Counter_For_Brightness_4 = 0;
			Brightness_4 --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_flag_4 == 2)
	{
		if (Counter_For_Brightness_4 == 600)
		{
			Counter_For_Brightness_4 = 0;
			Brightness_4 ++;
		}
	}

	/* PWM-Routine */
	if (Counter_3 >= 75)
	{
		Counter_3 = 0;
		
		if (Brightness_4 > 0)
		{
			com74hc595_setBit(3);
			com74hc595_setBit(8);
			com74hc595_setBit(13);
			com74hc595_setBit(18);
			com74hc595_setBit(23);
			com74hc595_setBit(28);
			com74hc595_setBit(33);
			com74hc595_setBit(38);
			com74hc595_setBit(43);
			com74hc595_setBit(48);
			com74hc595_setBit(53);
			com74hc595_setBit(58);
			com74hc595_setBit(63);
			com74hc595_setBit(68);
			com74hc595_setBit(73);
			com74hc595_setBit(78);
			com74hc595_setBit(83);
			com74hc595_setBit(88);
			com74hc595_setBit(93);
			com74hc595_setBit(98);
			com74hc595_setBit(103);
			com74hc595_setBit(108);
			com74hc595_setBit(113);
			com74hc595_setBit(118);	
			com74hc595_setBit(123);
			com74hc595_setBit(128);
			com74hc595_setBit(133);	
			com74hc595_setBit(138);
			com74hc595_setBit(143);
		}
	}
	else
	{
		if (Counter_3 == Brightness_4)
		{
			com74hc595_unsetBit(3);
			com74hc595_unsetBit(8);
			com74hc595_unsetBit(13);
			com74hc595_unsetBit(18);
			com74hc595_unsetBit(23);
			com74hc595_unsetBit(28);
			com74hc595_unsetBit(33);
			com74hc595_unsetBit(38);
			com74hc595_unsetBit(43);
			com74hc595_unsetBit(48);
			com74hc595_unsetBit(53);
			com74hc595_unsetBit(58);
			com74hc595_unsetBit(63);
			com74hc595_unsetBit(68);
			com74hc595_unsetBit(73);
			com74hc595_unsetBit(78);
			com74hc595_unsetBit(83);
			com74hc595_unsetBit(88);
			com74hc595_unsetBit(93);
			com74hc595_unsetBit(98);
			com74hc595_unsetBit(103);
			com74hc595_unsetBit(108);
			com74hc595_unsetBit(113);
			com74hc595_unsetBit(118);
			com74hc595_unsetBit(123);
			com74hc595_unsetBit(128);
			com74hc595_unsetBit(133);
			com74hc595_unsetBit(138);
			com74hc595_unsetBit(143);
			
		}
	}



	if (Brightness_5 >= 8)
	{
		Brightness_flag_5 = 1;
	}
	if (Brightness_5 <= 1)
	{
		Brightness_flag_5 = 2;
	}
	
	
	
	/* Runter-Faden */
	if (Brightness_flag_5 == 1)
	{
		if (Counter_For_Brightness_5 == 800)
		{
			Counter_For_Brightness_5 = 0;
			Brightness_5 --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_flag_5 == 2)
	{
		if (Counter_For_Brightness_5 == 800)
		{
			Counter_For_Brightness_5 = 0;
			Brightness_5 ++;
		}
	}

	/* PWM-Routine */
	if (Counter_4 >= 75)
	{
		Counter_4 = 0;
		
		if (Brightness_5 > 0)
		{
			com74hc595_setBit(4);
			com74hc595_setBit(9);
			com74hc595_setBit(14);
			com74hc595_setBit(19);
			com74hc595_setBit(24);
			com74hc595_setBit(29);
			com74hc595_setBit(34);
			com74hc595_setBit(39);
			com74hc595_setBit(44);
			com74hc595_setBit(49);
			com74hc595_setBit(54);
			com74hc595_setBit(59);
			com74hc595_setBit(64);
			com74hc595_setBit(69);
			com74hc595_setBit(74);
			com74hc595_setBit(79);
			com74hc595_setBit(84);
			com74hc595_setBit(89);
			com74hc595_setBit(94);
			com74hc595_setBit(99);
			com74hc595_setBit(104);
			com74hc595_setBit(109);
			com74hc595_setBit(114);
			com74hc595_setBit(119);
			com74hc595_setBit(124);
			com74hc595_setBit(129);
			com74hc595_setBit(134);
			com74hc595_setBit(139);
		}
	}
	else
	{
		if (Counter_4 == Brightness_5)
		{
			com74hc595_unsetBit(4);
			com74hc595_unsetBit(9);
			com74hc595_unsetBit(14);
			com74hc595_unsetBit(19);
			com74hc595_unsetBit(24);
			com74hc595_unsetBit(29);
			com74hc595_unsetBit(34);
			com74hc595_unsetBit(39);
			com74hc595_unsetBit(44);
			com74hc595_unsetBit(49);
			com74hc595_unsetBit(54);
			com74hc595_unsetBit(59);
			com74hc595_unsetBit(64);
			com74hc595_unsetBit(69);
			com74hc595_unsetBit(74);
			com74hc595_unsetBit(79);
			com74hc595_unsetBit(84);
			com74hc595_unsetBit(89);
			com74hc595_unsetBit(94);
			com74hc595_unsetBit(99);
			com74hc595_unsetBit(104);
			com74hc595_unsetBit(109);
			com74hc595_unsetBit(114);
			com74hc595_unsetBit(119);
			com74hc595_unsetBit(124);
			com74hc595_unsetBit(129);
			com74hc595_unsetBit(134);
			com74hc595_unsetBit(139);
		}
	}

	com74hc595_out();




}		

/* simulated LED flare */
void LED_Flare(void)
{
	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		Counter_2 = 0;
		Counter_1 = 0;
		Counter_For_Brightness_1 = 0;
		Brightness = 1;	
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(4);
		}		
	}	

	
	Counter_1 ++;
	Counter_2 ++;
	Counter_For_Brightness_1++;

	
	if (Brightness == 8)
	{
		Brightness_Flag_1 = 1;
	}
	if (Brightness == 1)
	{
		Brightness_Flag_1 = 2;
	}
		
	
	/* Runter-Faden */
	if (Brightness_Flag_1 == 1)
	{
		if (Counter_For_Brightness_1 >= 1)
		{
			Counter_For_Brightness_1 = 0;
			Brightness --;
		}
	}
	
	
	/* Hoch-Faden */
	if (Brightness_Flag_1 == 2)
	{
		if (Counter_For_Brightness_1 >= 1)
		{

			Counter_For_Brightness_1 = 0;
			Brightness ++;
		}
	}
	
	
	for (uint16_t x = 0; x <= Duration; x ++)
	{
		Counter ++;

		if (Counter >= 100)
		{
			Counter = 0;

			if (Brightness > 0)
			{
				com74hc595_setall();
			}
		}
		else
		{
			if (Counter >= Brightness)
			{
				com74hc595_unsetall();
			}
		}
			com74hc595_out();
	}	
	
	
	if (Counter_1 >= Change_Duration)
	{
		Counter_1 = 0;
		Change_Duration = (rand() % 8)+1;	//sollte zwischen 1 und 8 liegen
		Duration = (rand() % 800)+200;			//sollte zwischen 500 und 3000 liegen
	}
	
	if (Counter_2 >= 5)
	{
		Counter_2 = 0;
		Brightness_Flag_1 = (rand() % 2)+1;
	}
			
}

void LED_Circle_Down (uint16_t speed, uint8_t bright)
{
	

	if (LED_Status != LED_Status_Old)
	{
		all_LED_OFF();
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		Counter = 0;
		Counter_1 = 0;
		LED_0 = 0;
		LED_1 = 12;
		LED_2 = 24;
		LED_3 = 36;
		LED_4 = 48;
		LED_5 = 60;		
		LED_6 = 72;
		LED_7 = 84;
		LED_8 = 96;	
		LED_9 = 108;
		LED_10 = 120;
		LED_11 = 132;	
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(7);
		}			
	}
	
	Counter ++;
	Counter_1 ++;
	
	
	/* PWM-Routine */
	if (Counter >= 60)
	{
		Counter = 0;
	
		if (Brightness > 0)
		{
			com74hc595_setBit(LED_0);
			com74hc595_setBit(LED_1);
			com74hc595_setBit(LED_2);
			com74hc595_setBit(LED_3);
			com74hc595_setBit(LED_4);
			com74hc595_setBit(LED_5);
			com74hc595_setBit(LED_6);
			com74hc595_setBit(LED_7);
			com74hc595_setBit(LED_8);
			com74hc595_setBit(LED_9);
			com74hc595_setBit(LED_10);
			com74hc595_setBit(LED_11);
		}
	}
	else
	{
		if (Counter == bright)
		{
			com74hc595_unsetBit(LED_0);
			com74hc595_unsetBit(LED_1);
			com74hc595_unsetBit(LED_2);
			com74hc595_unsetBit(LED_3);
			com74hc595_unsetBit(LED_4);
			com74hc595_unsetBit(LED_5);
			com74hc595_unsetBit(LED_6);
			com74hc595_unsetBit(LED_7);
			com74hc595_unsetBit(LED_8);
			com74hc595_unsetBit(LED_9);
			com74hc595_unsetBit(LED_10);
			com74hc595_unsetBit(LED_11);
		}
	}
		com74hc595_out();	
	
	
	
	
	
	if (Counter_1 == speed)
	{
		com74hc595_unsetall();
		Counter_1 = 0;
		LED_0 ++;
		LED_1 ++;
		LED_2 ++;
		LED_3 ++;
		LED_4 ++;
		LED_5 ++;
		LED_6 ++;
		LED_7 ++;
		LED_8 ++;
		LED_9 ++;
		LED_10 ++;
		LED_11 ++;						
	}
	
	if (LED_11 == 144)
	{
		LED_0 = 0;
		LED_1 = 12;
		LED_2 = 24;
		LED_3 = 36;
		LED_4 = 48;
		LED_5 = 60;
		LED_6 = 72;
		LED_7 = 84;
		LED_8 = 96;
		LED_9 = 108;
		LED_10 = 120;
		LED_11 = 132;
	}
		
}
	
void LED_Down (uint16_t speed)
{
	
	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		LED_0 = 0;
		Counter = 0;
		Counter_1 = 0;
		Brightness = 8;		
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(8);
		}		
	}
	
	Counter ++;
	Counter_1 ++;	
	
		
 	switch (LED_0)
	{
		case 0:
		{
			LED_0 = 0;
			LED_1 = 12;
		}
		break;
	
		case 144:
		{			
			LED_0 = 1;
			LED_1 = 13;
		}
		break;
		
		case 145:
		{			
			LED_0 = 2;
			LED_1 = 14;
		}
		break;	
		
		case 146:
		{			
			LED_0 = 3;
			LED_1 = 15;
		}
		break;
		
		case 147:
		{			
			LED_0 = 4;
			LED_1 = 16;
		}
		break;	
		
		case 148:
		{	
			LED_0 = 5;
			LED_1 = 17;
		}
		break;	
		
		case 149:
		{
			LED_0 = 6;
			LED_1 = 18;
		}
		break;	
		
		case 150:
		{			
			LED_0 = 7;
			LED_1 = 19;
		}
		break;
		
		case 151:
		{
			LED_0 = 8;
			LED_1 = 20;
		}
		break;
		
		case 152:
		{	
			LED_0 = 9;
			LED_1 = 21;
		}
		break;	
		
		case 153:
		{			
			LED_0 = 10;
			LED_1 = 22;
		}
		break;	

		case 154:
		{		
			LED_0 = 11;
			LED_1 = 23;
		}
		break;										
		
						
	}	

	

	/* PWM-Routine */
	if (Counter >= 60)
	{
		Counter = 0;
	
		if (Brightness > 0)
		{
	 		com74hc595_setBit(LED_0);
	 		com74hc595_setBit(LED_1);
		}
	}
	else
	{
		if (Counter == Brightness)
		{
	 		com74hc595_unsetBit(LED_0);
	 		com74hc595_unsetBit(LED_1);
		}
	}
		com74hc595_out();		
	

	if (Counter_1 >= speed)
	{
		com74hc595_unsetBit(LED_0);
		com74hc595_unsetBit(LED_1);
		LED_1 = LED_1 +24;
		LED_0 = LED_0 +24;
		Counter_1 = 0;
	}
	
	if (LED_0 >= 155)
	{
		LED_0 = 0;
	}
	
	if (LED_1 >= 167)
	{
		LED_1 = 0;
	}
}

void LED_Round(uint16_t speed)
{
	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
		LED_3 = 9;
		LED_4 = 10;
		LED_5 = 11;
		Counter = 0;
		Brightness = 8;
		Counter_1 = 0;
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status == 7)
		{
			if (LED_Status != 12)
			{
				Display_a_Character(9);
			}			
		}
				
	}
	
	
	Counter ++;
	Counter_1 ++;
	

	/* PWM routine */
	if (Counter >= 60)
	{
		Counter = 0;
	
		if (Brightness > 0)
		{
			com74hc595_setPort(LED_0,0xFF);
			com74hc595_setPort(LED_1,0xFF);
			com74hc595_setPort(LED_2,0xFF);
			com74hc595_setPort(LED_3,0xFF);
			com74hc595_setPort(LED_4,0xFF);
			com74hc595_setPort(LED_5,0xFF);
		}
	}
	else
	{
		if (Counter >= Brightness)
		{
			com74hc595_unsetPort(LED_0);
			com74hc595_unsetPort(LED_1);
			com74hc595_unsetPort(LED_2);
			com74hc595_unsetPort(LED_3);
			com74hc595_unsetPort(LED_4);
			com74hc595_unsetPort(LED_5);
		}
	}
	com74hc595_out();


	
	if (Counter_1 >= speed)
	{
		com74hc595_unsetall();
		Counter_1 = 0;
		LED_0 = LED_0 +3;
		LED_1 = LED_1 +3;
		LED_2 = LED_2 +3;
		LED_3 = LED_3 +3;
		LED_4 = LED_4 +3;
		LED_5 = LED_5 +3;		
	}
	
	if (LED_0 == 18)
	{
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
	}
	
	if (LED_3 == 18)
	{
		LED_3 = 0;
		LED_4 = 1;
		LED_5 = 2;
	}

				
}

void LED_Round_Test(void)
{	
	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
		LED_3 = 9;
		LED_4 = 10;
		LED_5 = 11;
		speed = 3000;
		speed_flag = 1;
		Counter = 0;
		Brightness = 8;
		Counter_1 = 0;
		Counter_2 = 0;
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		
		if (LED_Status != 12)
		{
			Display_a_Character(73);
		}
		
		
	}
	
	
	Counter ++;
	Counter_1 ++;
	Counter_2 ++;
	

	/* PWM routine */
	if (Counter >= 60)
	{
		Counter = 0;
		
		if (Brightness > 0)
		{
			com74hc595_setPort(LED_0,0xFF);
			com74hc595_setPort(LED_1,0xFF);
			com74hc595_setPort(LED_2,0xFF);
			com74hc595_setPort(LED_3,0xFF);
			com74hc595_setPort(LED_4,0xFF);
			com74hc595_setPort(LED_5,0xFF);
		}
	}
	else
	{
		if (Counter >= Brightness)
		{
			com74hc595_unsetPort(LED_0);
			com74hc595_unsetPort(LED_1);
			com74hc595_unsetPort(LED_2);
			com74hc595_unsetPort(LED_3);
			com74hc595_unsetPort(LED_4);
			com74hc595_unsetPort(LED_5);
		}
	}
	com74hc595_out();






	if (speed <= 300)
	{
		speed_flag = 2;
	}
	if (speed >= 2500)
	{
		speed_flag = 1;
	}
	
	
	
	/* Runter-Faden */
	if (speed_flag == 1)
	{
		if (Counter_2 >= 10)
		{
			Counter_2 = 0;
			speed --;
		}
	}
	
	
	/* Hoch-Faden */
	if (speed_flag == 2)
	{
		if (Counter_2 >= 10)
		{
			Counter_2 = 0;
			speed ++;
		}
	}
	
	
	
	
	
	
	if (Counter_1 >= speed)
	{
		com74hc595_unsetall();
		Counter_1 = 0;
		LED_0 = LED_0 +3;
		LED_1 = LED_1 +3;
		LED_2 = LED_2 +3;
		LED_3 = LED_3 +3;
		LED_4 = LED_4 +3;
		LED_5 = LED_5 +3;
	}
	
	if (LED_0 == 18)
	{
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
	}
	
	if (LED_3 == 18)
	{
		LED_3 = 0;
		LED_4 = 1;
		LED_5 = 2;
	}

	
}
	
void LED_Change(uint16_t speed, uint8_t bright)
{

	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		all_LED_OFF();
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
		LED_3 = 6;
		LED_4 = 7;
		LED_5 = 8;
		LED_6 = 12;
		LED_7 = 13;
		LED_8 = 14;
		Counter = 0;
		Counter_1 = 0;
		Brightness = bright;
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(11);
		}			
	}
	
	
	Counter ++;
	Counter_1 ++;


	if (Counter_1 <= speed)
	{		
		
			/* PWM-Routine */
		if (Counter >= 80)
		{
			Counter = 0;
	
			if (Brightness > 0)
			{
				com74hc595_setPort(0,0b10101010);
				com74hc595_setPort(1,0b10101010);
				com74hc595_setPort(2,0b10101010);
				com74hc595_setPort(3,0b10101010);
				com74hc595_setPort(4,0b10101010);
				com74hc595_setPort(5,0b10101010);
				com74hc595_setPort(6,0b10101010);
				com74hc595_setPort(7,0b10101010);
				com74hc595_setPort(8,0b10101010);
				com74hc595_setPort(9,0b10101010);
				com74hc595_setPort(10,0b10101010);
				com74hc595_setPort(11,0b10101010);	
				com74hc595_setPort(12,0b10101010);
				com74hc595_setPort(13,0b10101010);
				com74hc595_setPort(14,0b10101010);
				com74hc595_setPort(15,0b10101010);
				com74hc595_setPort(16,0b10101010);
				com74hc595_setPort(17,0b10101010);													
			}
		}
		else
		{
			if (Counter == Brightness)
			{
				com74hc595_unsetPort(0);
				com74hc595_unsetPort(1);
				com74hc595_unsetPort(2);
				com74hc595_unsetPort(3);
				com74hc595_unsetPort(4);
				com74hc595_unsetPort(5);
				com74hc595_unsetPort(6);
				com74hc595_unsetPort(7);
				com74hc595_unsetPort(8);
				com74hc595_unsetPort(9);
				com74hc595_unsetPort(10);
				com74hc595_unsetPort(11);
				com74hc595_unsetPort(12);
				com74hc595_unsetPort(13);
				com74hc595_unsetPort(14);
				com74hc595_unsetPort(15);
				com74hc595_unsetPort(16);
				com74hc595_unsetPort(17);
			}		
		}
		com74hc595_out();
	}






	if (Counter_1 >= speed)
	{
		/* PWM-Routine */
		if (Counter >= 80)
		{		
			Counter = 0;
			
			if (Brightness > 0)
			{
				com74hc595_setPort(0,0b01010101);
				com74hc595_setPort(1,0b01010101);
				com74hc595_setPort(2,0b01010101);
				com74hc595_setPort(3,0b01010101);
				com74hc595_setPort(4,0b01010101);
				com74hc595_setPort(5,0b01010101);
				com74hc595_setPort(6,0b01010101);
				com74hc595_setPort(7,0b01010101);
				com74hc595_setPort(8,0b01010101);
				com74hc595_setPort(9,0b01010101);
				com74hc595_setPort(10,0b01010101);
				com74hc595_setPort(11,0b01010101);
				com74hc595_setPort(12,0b01010101);
				com74hc595_setPort(13,0b01010101);
				com74hc595_setPort(14,0b01010101);
				com74hc595_setPort(15,0b01010101);
				com74hc595_setPort(16,0b01010101);
				com74hc595_setPort(17,0b01010101);
			}
		}
		else
		{
			if (Counter == Brightness)
			{
				com74hc595_unsetPort(0);
				com74hc595_unsetPort(1);
				com74hc595_unsetPort(2);
				com74hc595_unsetPort(3);
				com74hc595_unsetPort(4);
				com74hc595_unsetPort(5);
				com74hc595_unsetPort(6);
				com74hc595_unsetPort(7);
				com74hc595_unsetPort(8);
				com74hc595_unsetPort(9);
				com74hc595_unsetPort(10);
				com74hc595_unsetPort(11);
				com74hc595_unsetPort(12);
				com74hc595_unsetPort(13);
				com74hc595_unsetPort(14);
				com74hc595_unsetPort(15);
				com74hc595_unsetPort(16);
				com74hc595_unsetPort(17);
			}
		}
		com74hc595_out();
	}


	if (Counter_1 == speed * 2)
	{
		Counter_1 = 0;
	}


		
}

void LED_Blink(uint16_t speed)
{
	if (LED_Status != LED_Status_Old)
	{
 		LED_Status_Old = LED_Status;
		
		/* save the actual "LED Status" */
 		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		
		Brightness_1 = 2;
		Brightness_2 = 4;
		Brightness_3 = 8;
		Brightness_4 = 11;
		Counter_For_Brightness_1 = 0;
		Counter_For_Brightness_2 = 0;
		Counter_For_Brightness_3 = 0;
		Counter_For_Brightness_4 = 0;
		Brightness_Flag_1 = 1;
		Brightness_Flag_2 = 2;
		Brightness_flag_3 = 1;
		Brightness_flag_4 = 2;
		Counter = 0;
		Counter_1 = 0;
		Counter_2 = 0;
		Counter_3 = 0;
		all_LED_OFF();
		LED_0 = (rand() % 143)+0;
		LED_1 = (rand() % 143)+0;
		LED_2 = (rand() % 143)+0;
		LED_3 = (rand() % 143)+0;	
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(74);			
		}
	}

/* Funktionen sorgen dafür, dass die LEDs nicht die gleichen Werte annehmen können */	
	while (LED_0 == LED_1)
	{
		LED_0 = (rand() % 143)+0;
	}
	while (LED_0 == LED_2)
	{
		LED_0 = (rand() % 143)+0;
	}
	while (LED_0 == LED_3)
	{
		LED_0 = (rand() % 143)+0;
	}
	while (LED_1 == LED_2)
	{
		LED_1 = (rand() % 143)+0;
	}
	while (LED_1 == LED_3)
	{
		LED_1 = (rand() % 143)+0;
	}		
	while (LED_2 == LED_3)
	{
		LED_2 = (rand() % 143)+0;
	}		
	
	
 	Counter ++;
 	Counter_1 ++;
 	Counter_2 ++;
  	Counter_3 ++;
 	
	Counter_For_Brightness_1 ++;
	Counter_For_Brightness_2 ++;
	Counter_For_Brightness_3 ++;
 	Counter_For_Brightness_4 ++;
	
	
	if (Brightness_1 >= 12)
	{
		Brightness_Flag_1 = 1;
	}	
	
	
	
	
	
	/* fading "down" */
	if (Brightness_Flag_1 == 1)
	{
		if (Counter_For_Brightness_1 == speed)
		{
			Counter_For_Brightness_1 = 0;
			Brightness_1 --;
		}
	}
	
	
	/* fading "up" */
	if (Brightness_Flag_1 == 2)
	{
		if (Counter_For_Brightness_1 == speed)
		{
			Counter_For_Brightness_1 = 0;
			Brightness_1 ++;
		}
	}

	/* PWM routine */
	if (Counter >= 80)
	{
		Counter = 0;
		
		if (Brightness_1 > 0)
		{
			com74hc595_setBit(LED_0);			
		}
	}
	else
	{
		if (Counter >= Brightness_1)
		{
			com74hc595_unsetBit(LED_0);
		}
	}


	if (Brightness_2 >= 12)
	{
		Brightness_Flag_2 = 1;
	}	
	
	
	
	/* fading "down" */
	if (Brightness_Flag_2 == 1)
	{
		if (Counter_For_Brightness_2 == speed)
		{
			Counter_For_Brightness_2 = 0;
			Brightness_2 --;
		}
	}
	
	
	/* fading "up" */
	if (Brightness_Flag_2 == 2)
	{
		if (Counter_For_Brightness_2 == speed)
		{
			Counter_For_Brightness_2 = 0;
			Brightness_2 ++;
		}
	}

	/* PWM-routine */
	if (Counter_1 >= 80)
	{
		Counter_1 = 0;
		
		if (Brightness_2 > 0)
		{
			com74hc595_setBit(LED_1);
		}
	}
	else
	{
		if (Counter_1 >= Brightness_2)
		{
			com74hc595_unsetBit(LED_1);
		}
	}

	
	if (Brightness_3 >= 12)
	{
		Brightness_flag_3 = 1;
	}

	
	/* fading "down" */
	if (Brightness_flag_3 == 1)
	{
		if (Counter_For_Brightness_3 == speed)
		{
			Counter_For_Brightness_3 = 0;
			Brightness_3 --;
		}
	}
	
	
	/* fading "up" */
	if (Brightness_flag_3 == 2)
	{
		if (Counter_For_Brightness_3 == speed)
		{
			Counter_For_Brightness_3 = 0;
			Brightness_3 ++;
		}
	}

	/* PWM routine */
	if (Counter_2 >= 80)
	{
		Counter_2 = 0;
		
		if (Brightness_3 > 0)
		{
			com74hc595_setBit(LED_2);
		}
	}
	else
	{
		if (Counter_2 >= Brightness_3)
		{
			com74hc595_unsetBit(LED_2);
		}
	}


	
	if (Brightness_4 >= 12)
	{
		Brightness_flag_4 = 1;
	}
	
	
	
	/* fading "down" */
	if (Brightness_flag_4 == 1)
	{
		if (Counter_For_Brightness_4 >= speed)
		{
			Counter_For_Brightness_4 = 0;
			Brightness_4 --;
		}
	}
	
	
	/* fading "up" */
	if (Brightness_flag_4 == 2)
	{
		if (Counter_For_Brightness_4 >= speed)
		{
			Counter_For_Brightness_4 = 0;
			Brightness_4 ++;
		}
	}

	/* PWM routine */
	if (Counter_3 >= 50)
	{
		Counter_3 = 0;
		
		if (Brightness_4 > 0)
		{
			com74hc595_setBit(LED_3);
		}
	}
	else
	{
		if (Counter_3 >= Brightness_4)
		{
			com74hc595_unsetBit(LED_3);
		}
	}
	
	com74hc595_out();
	
			
	if (Brightness_1 == 0)
	{
		Brightness_1 = 1;
		Brightness_Flag_1 = 2;
		com74hc595_unsetBit(LED_0);
		LED_0 = (rand() % 143)+0;
	}
	
	if (Brightness_2 == 0)
	{
		Brightness_2 = 1;
		Brightness_Flag_2 = 2;
		com74hc595_unsetBit(LED_1);
		LED_1 = (rand() % 143)+0;
	}

	if (Brightness_3 == 0)
	{
		Brightness_3 = 1;
		Brightness_flag_3 = 2;
		com74hc595_unsetBit(LED_2);
		LED_2 = (rand() % 143)+0;
	}
	
	if (Brightness_4 == 0)
	{
		Brightness_4 = 1;
		Brightness_flag_4 = 2;
		com74hc595_unsetBit(LED_3);
		LED_3 = (rand() % 143)+0;
	}
		
	
}

/* form the Average from Accu Supply (ADC) */ 
void Average_from_Accu_Supply(void)
{
 	/* Ausleseroutine des ADC-Wertes des Akkus alle 1,5 s */
	if (Counter_For_Reading_ADC >= 100)
	{
		Supply_ADC_Value = read_ADC(7);	
	}

}

/* If a Switch long pressed, then the µC go sleep */
void sleep_after_long_press(void)
{	
		if (Long_Key_Pressed >= 160)
		{
			Long_Key_Pressed = 0;
			go_sleep();
		}	
}

/* if the µC >= 12 h at work the go sleep and show remaining Time */
void sleep_after_12h(void)
{
	
#ifdef WITH_REMAINING

	
	/* 1 hour past */	
	if (Sleep_Counter == (144000))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  11 hour remaining";
	
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		    ENABLE_25ms_ISR;

	}
	
	/* 2 hour past */
	if (Sleep_Counter == (144000*2))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  10 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 3 hour past */
	if (Sleep_Counter == (144000*3))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  9 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}

	/* 4 hour past */
	if (Sleep_Counter == (144000*4))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  8 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 5 hour past */
	if (Sleep_Counter == (144000*5))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  7 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 6 hour past */
	if (Sleep_Counter == (144000*6))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  6 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 7 hour past */
	if (Sleep_Counter == (144000*7))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  5 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 8 hour past */
	if (Sleep_Counter == (144000*8))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  4 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 9 hour past */
	if (Sleep_Counter == (144000*9))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  3 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 10 hour past */
	if (Sleep_Counter == (144000*10))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  2 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
	/* 11 hour past */
	if (Sleep_Counter == (144000*11))
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
				
		char s[] = "  1 hour remaining";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;
	}
	
#endif
// Zählwert = 12 Stunden	
	if (Sleep_Counter >= 1728000)
	{
		DISABLE_25ms_ISR;
		all_LED_OFF();
		clear_screen();
		
		char s[] = "  sleep after 12 hour activated";
		
		for (uint16_t x=0; x<(strlen(s)*8); x++)
		{
			DisplayString(x, s);
			_delay_ms(80);
		}
		ENABLE_25ms_ISR;		
		Sleep_Counter = 0;
		go_sleep();
	}	
}

/* check the accu after 100ms */
void check_accu_after_100ms(void)
{
	if (Counter_For_Reading_Accu >= 4)
	{
		Counter_For_Reading_Accu = 0;
		check_Accu();
	}	
}

/* check the overflow from the Menü points ( LED Status 0 - 9 ) */
void check_overflow(void)
{
	
#ifdef MENUE_CHANGE_ONE

	if (LED_Status >= 12)
	{
		LED_Status = 12;
	}
		
	if (LED_Status <= 0)
	{
		LED_Status = 1;
	}
		
#endif

#ifdef MENUE_CHANGE_TWO

	if (LED_Status >= 13)
	{
		LED_Status = 1;
	}

	if (LED_Status <= 0)
	{
		LED_Status = 12;
	}

#endif

}

/* generating a PWM for one LED */
void PWM_for_LED(uint8_t LED, uint8_t bright)
{
	Counter ++;
	
	/* PWM-Routine */
	if (Counter >= 100)
	{
		Counter = 0;
	
		if (Brightness > 0)
		{
			com74hc595_setBit(LED);
			com74hc595_out();	
		}
	}
	else
	{
		if (Counter >= bright)
		{
			com74hc595_unsetBit(LED);
			com74hc595_out();	
		}
	}
	//com74hc595_out();	
}

/* testmode */
void Every_Third_LED_Down(void)
{

	if (LED_Status != LED_Status_Old)
	{
		LED_Status_Old = LED_Status;
		eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
		all_LED_OFF();
		LED_0 = 0;
		LED_1 = 1;
		LED_2 = 2;
		LED_3 = 6;
		LED_4 = 7;
		LED_5 = 8;
		LED_6 = 12;
		LED_7 = 13;
		LED_8 = 14;
		Counter = 0;
		Brightness = 8;
		Counter_For_Display = 0;
		counter_for_change_accu_symbol = 0;
		if (LED_Status != 12)
		{
			Display_a_Character(10);
		}
	}
	Counter ++;
	Counter_1 ++;



	if (Counter_1 <= 5000)
	{
		
		/* PWM-Routine */
		if (Counter >= 80)
		{
			Counter = 0;
			
			if (Brightness > 0)
			{
				com74hc595_setPort(0,0b01001001);
				com74hc595_setPort(1,0b10010010);
				com74hc595_setPort(2,0b00100100);
				com74hc595_setPort(3,0b01001001);
				com74hc595_setPort(4,0b10010010);
				com74hc595_setPort(5,0b00100100);
				com74hc595_setPort(6,0b01001001);
				com74hc595_setPort(7,0b10010010);
				com74hc595_setPort(8,0b00100100);
				com74hc595_setPort(9,0b01001001);
				com74hc595_setPort(10,0b10010010);
				com74hc595_setPort(11,0b00100100);
				com74hc595_setPort(12,0b01001001);
				com74hc595_setPort(13,0b10010010);
				com74hc595_setPort(14,0b00100100);
				com74hc595_setPort(15,0b01001001);
				com74hc595_setPort(16,0b10010010);
				com74hc595_setPort(17,0b00100100);
			}
		}
		else
		{
			if (Counter == Brightness)
			{
				com74hc595_unsetPort(0);
				com74hc595_unsetPort(1);
				com74hc595_unsetPort(2);
				com74hc595_unsetPort(3);
				com74hc595_unsetPort(4);
				com74hc595_unsetPort(5);
				com74hc595_unsetPort(6);
				com74hc595_unsetPort(7);
				com74hc595_unsetPort(8);
				com74hc595_unsetPort(9);
				com74hc595_unsetPort(10);
				com74hc595_unsetPort(11);
				com74hc595_unsetPort(12);
				com74hc595_unsetPort(13);
				com74hc595_unsetPort(14);
				com74hc595_unsetPort(15);
				com74hc595_unsetPort(16);
				com74hc595_unsetPort(17);
			}
		}
		com74hc595_out();
	}






	if (Counter_1 <= 10000)
	{
		if (Counter_1 >= 5000)
		{			
			/* PWM-Routine */
			if (Counter >= 80)
			{
				Counter = 0;
			
				if (Brightness > 0)
				{
					com74hc595_setPort(0,0b10010010);
					com74hc595_setPort(1,0b00100100);
					com74hc595_setPort(2,0b01001001);
					com74hc595_setPort(3,0b10010010);
					com74hc595_setPort(4,0b00100100);
					com74hc595_setPort(5,0b01001001);
					com74hc595_setPort(6,0b10010010);
					com74hc595_setPort(7,0b00100100);
					com74hc595_setPort(8,0b01001001);
					com74hc595_setPort(9,0b10010010);
					com74hc595_setPort(10,0b00100100);
					com74hc595_setPort(11,0b01001001);
					com74hc595_setPort(12,0b10010010);
					com74hc595_setPort(13,0b00100100);
					com74hc595_setPort(14,0b01001001);
					com74hc595_setPort(15,0b10010010);
					com74hc595_setPort(16,0b00100100);
					com74hc595_setPort(17,0b01001001);
				}
			}
			else
			{
				if (Counter == Brightness)
				{
					com74hc595_unsetPort(0);
					com74hc595_unsetPort(1);
					com74hc595_unsetPort(2);
					com74hc595_unsetPort(3);
					com74hc595_unsetPort(4);
					com74hc595_unsetPort(5);
					com74hc595_unsetPort(6);
					com74hc595_unsetPort(7);
					com74hc595_unsetPort(8);
					com74hc595_unsetPort(9);
					com74hc595_unsetPort(10);
					com74hc595_unsetPort(11);
					com74hc595_unsetPort(12);
					com74hc595_unsetPort(13);
					com74hc595_unsetPort(14);
					com74hc595_unsetPort(15);
					com74hc595_unsetPort(16);
					com74hc595_unsetPort(17);
				}
			}
			com74hc595_out();
		}
		
	}





	if (Counter_1 <= 15000)
	{
		if (Counter_1 >= 10000)
		{
			/* PWM-Routine */
			if (Counter >= 80)
			{
				Counter = 0;
				
				if (Brightness > 0)
				{
					com74hc595_setPort(0,0b00100100);
					com74hc595_setPort(1,0b01001001);
					com74hc595_setPort(2,0b10010010);
					com74hc595_setPort(3,0b00100100);
					com74hc595_setPort(4,0b01001001);
					com74hc595_setPort(5,0b10010010);
					com74hc595_setPort(6,0b00100100);
					com74hc595_setPort(7,0b01001001);
					com74hc595_setPort(8,0b10010010);
					com74hc595_setPort(9,0b00100100);
					com74hc595_setPort(10,0b01001001);
					com74hc595_setPort(11,0b10010010);
					com74hc595_setPort(12,0b00100100);
					com74hc595_setPort(13,0b01001001);
					com74hc595_setPort(14,0b10010010);
					com74hc595_setPort(15,0b00100100);
					com74hc595_setPort(16,0b01001001);
					com74hc595_setPort(17,0b10010010);
				}
			}
			else
			{
				if (Counter == Brightness)
				{
					com74hc595_unsetPort(0);
					com74hc595_unsetPort(1);
					com74hc595_unsetPort(2);
					com74hc595_unsetPort(3);
					com74hc595_unsetPort(4);
					com74hc595_unsetPort(5);
					com74hc595_unsetPort(6);
					com74hc595_unsetPort(7);
					com74hc595_unsetPort(8);
					com74hc595_unsetPort(9);
					com74hc595_unsetPort(10);
					com74hc595_unsetPort(11);
					com74hc595_unsetPort(12);
					com74hc595_unsetPort(13);
					com74hc595_unsetPort(14);
					com74hc595_unsetPort(15);
					com74hc595_unsetPort(16);
					com74hc595_unsetPort(17);
				}
			}
			com74hc595_out();
		}
		
	}






		

	if (Counter_1 == 15000)
	{
		Counter_1 = 0;
	}


	
}

/* change automatic the simulation every one minute */
void Auto_Change_Simulation()
{
		if (LED_Status != LED_Status_Old)
		{
			Counter_For_Display = 0;
			counter_for_change_accu_symbol = 0;
			eeprom_write_byte(&EEPROM_save_LED_Status, LED_Status);
			Auto_Change = 0;
			Auto_Change_ISR = 0;
			all_LED_OFF();
			Display_a_Character(75);
			LED_Status_Old = LED_Status;
			Auto_Change_Old = 100;
		}
		
		while (LED_Status == 12)
		{
			
			/* If a Switch long pressed, then the µC go sleep */
			sleep_after_long_press();
			
			/* if the µC >= 12 h at work the go sleep */
			sleep_after_12h();
			
			check_accu_after_100ms();	
			
			check_Test_Mode();	
			
			
	 		switch(Auto_Change)
			{
				case 0: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 22;					
					}
					LED_Blink(300);
				}
				break;
			
				case 1: 
				{	
 					if (Auto_Change != Auto_Change_Old)
 					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 23;			
 					}
					LED_Sparkle();
				}
				break;
			
				case 2:
				{	
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 24;
 					}
					Raindrop_Simulation_1(40,10,4,2,40,10,4,2);
					Raindrop_Simulation_2(40,10,4,2,40,10,4,2);
				}
				break;
			
				case 3: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 25;
					}
					LED_Flare();
				}
				break;
			
				case 4: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;						
						LED_Status_Old = 26;
					}					
					LED_Circle_Down(5000,8);
				}
				break;
			
				case 5: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;						
						LED_Status_Old = 27;
					}					
					LED_Fading(800,800);
				}
				break;
			
				case 6: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;						
						LED_Status_Old = 28;
					}					
					Every_Third_LED_Down();
				}
				break;
			
				case 7: 
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;						
						LED_Status_Old = 29;
					}					
					LED_Round(5000);
				}
				break;
				
				case 8:
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 30;
					}
					LED_Down(1000);
				}
				break;
				
				case 9:
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 31;
					}
					LED_Change(2000,8);
				}	
				break;
				
				case 10:
				{
					if (Auto_Change != Auto_Change_Old)
					{
						Auto_Change_Old = Auto_Change;
						LED_Status_Old = 32;
					}
					LED_Round_Test();
				}
				break;				
				
									
				case 11: 
				{Auto_Change = 0;}
				break;
			
				check_overflow();
			
			}		
		}
		


}

void check_Test_Mode()
{
	/* if the first switch is pressed for 8 sec and the second switch is pressed the first 5 second, change to Test_Mode */

	if (variable_for_check_test_mode >= 100)
	{
		variable_for_check_test_mode = 0;
		counter_for_Test_Mode = 0;
		Test_Mode();
	}
}
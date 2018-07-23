#ifndef _com74hc595_H_
#define _com74hc595_H_





// Numbers of 74HC595
#define com74hc595_SIZE 18

// PORTS
#define PORT_com74hc595 PORTD
#define PORT_SER 		PD5		//74HC595 PIN 14
#define PORT_SCK 		PD6		//74HC595 PIN 12
#define PORT_RCK 		PD4		//74HC595 PIN 11
#define PORT_OE 				//74HC595 PIN 13

#define DDR_com74hc595  DDRD
#define DDR_SER 		PD5	//74HC595 PIN 14
#define DDR_SCK 		PD6	//74HC595 PIN 12
#define DDR_RCK 		PD4	//74HC595 PIN 11
#define DDR_OE 				//74HC595 PIN 13


// use with /OE
//#define WITH_OE


//********** only for internal use. Don't change any value ***************

// Numbers of Bits over all 74HC595
#define com74hc595_BYTES com74hc595_SIZE * 8

// global array 
extern unsigned char com74hc595[];

// initialize the mikrocontroller
extern void com74hc595_init();

// output to 74HC595
extern void com74hc595_out();

// set a single Bit in arrays
extern void com74hc595_setBit( unsigned char BitNumber );

// unset a single Bit in arrays
extern void com74hc595_unsetBit( unsigned char BitNumber );

// set a port with a 8 Bit value
extern void com74hc595_setPort( unsigned char Port, unsigned char Bits );

// unset a port
extern void com74hc595_unsetPort( unsigned char Port );

// set all bit to logical High = 1
extern void com74hc595_setall();

// set all bit to logical Low = 0
extern void com74hc595_unsetall();

#endif /* _com74hc595_H_ */

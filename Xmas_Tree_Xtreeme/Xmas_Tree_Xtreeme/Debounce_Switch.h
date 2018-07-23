/*
 * Copyright:      Daniel Welther & Jan Homann
 * Author:         Daniel Welther & Jan Homann
 * Version:        1.0
 * Description:    Function for Debounce "Swichtes" it´s based on TIMER with 25 ms Interrupt
 */

#ifndef F_CPU
#define F_CPU 8000000
#endif

/**
 @brief    Debounce the Switch ( start the function on 25 ms based Interrupt )
 @param    PORT = z.B PINA, PIN = z.B 1DEC or 0x01HEX or 0b00000001BIN, ms = 1 = 25ms || 2 = 50ms || 3 = 75                                        
 @return   1 = Switch is Debounced or 0 = Switch is Bounced
*/
uint8_t Debounce_Switch(uint8_t PORT, uint8_t PIN, uint8_t ms);

#ifndef AVR_ADC
#define AVR_ADC

#include <avr/io.h>

/**
 @brief    Initialize the AD-Converter
 @param    none                                        
 @return   none
*/
void init_ADC ();

/**
 @brief    Disable the ADC Unit
 @param    none                                    
 @return   none
*/
void Disable_ADC(void);

/**
 @brief    Read the ADC-value from PORTX and PINX
 @param    channel                                        
 @return   the function "read_ADC" restore the value from the reading channel
*/
uint16_t read_ADC(uint8_t ch);

#endif
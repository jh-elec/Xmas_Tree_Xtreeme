
#include <avr/io.h>
#include "adc.h"

void init_ADC () {
  ADMUX |= (1 << REFS0) | (1<< REFS1);
  ADCSRA |= (1 << ADEN) | (1 << ADPS2);
}

uint16_t read_ADC(uint8_t ch) {
  ADMUX |= ch;
  ADCSRA |= (1 << ADSC);
  while (!(ADCSRA & (1 << ADIF)));
  ADCSRA |= (1 << ADIF);

  return (ADC);
}

void Disable_ADC(void)
{
  ADMUX &= ~((1 << REFS0) | (1<< REFS1));
  ADCSRA &= ~((1 << ADEN) | (1 << ADPS1));	
}
/*
 * Copyright:      Daniel Welther
 * Author:         Daniel Welther
 * Version:        1.0
 * Description:    Function for Debounce "Swichtes" it´s based on TIMER with 25 ms Interrupt
 */

#include <avr/io.h>




uint8_t Debounce_Switch(uint8_t PORT, uint8_t PIN, uint8_t ms)
{
	static uint8_t Switch_Flag;
	uint8_t Switch_Debounced = 0;
	
	if (!(PORT & (1<<PIN)))
	{
		Switch_Flag++;
	}
		
	if (Switch_Flag >= ms)
	{
		if (PORT & (1<<PIN))
		{
			Switch_Flag = 0;
			Switch_Debounced = 1;
		}
		else
		{
			Switch_Debounced = 0;
		}
	}
		
		return(Switch_Debounced);

		Switch_Debounced = 0;

}


/*
 if switch is "Debounced" the "Function" return "1" else return "0" 
		
	while(1)
	{
		Switch_ONE_State = Debounce_Switch(PIND, 4, 1);
		Switch_TWO_State = Debounce_Switch(PIND, 3, 1);
		
		
		if (Switch_ONE_State == 1)
		{
			// action...
		}
		
		if (Switch_TWO_State == 1)
		{
			// action...
		}
	}
*/
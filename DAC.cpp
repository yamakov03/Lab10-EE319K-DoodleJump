// put implementations for functions, explain how it works
// put your names here, date
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

#include "DAC.h"

// **************DAC_Init*********************
// Initialize 6-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x02;		// clock FE DCBA
	delay = SYSCTL_RCGCGPIO_R;		// NOP
	GPIO_PORTB_DIR_R |= 0x3F;			// initialize outputs (1) on PB 0-5 0011 1111
  GPIO_PORTB_DEN_R |= 0x3F;			// digital enable PB 0-5
	GPIO_PORTB_DR8R_R |= 0x3F;	
}

// **************DAC_Out*********************
// output to DAC
// Input: 6-bit data, 0 to 63 
// Input=n is converted to n*3.3V/63
// Output: none
void DAC_Out(uint32_t data){
	GPIO_PORTB_DATA_R = data;
}

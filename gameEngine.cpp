#include <stdint.h>
#include <stdio.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "SlidePot.h"
#include "Images.h"
#include "UART.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "gameengine.h"

// Initialize Port F so PF1, PF2 and PF3 are heartbeats
void PortF_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x20;
	delay = SYSCTL_RCGCGPIO_R;
	GPIO_PORTF_DIR_R   |= 0x0E;
	GPIO_PORTF_DEN_R 	 |= 0x0E;
	GPIO_PORTF_AFSEL_R &= ~0x0E;
	GPIO_PORTF_PUR_R 	 |= 0x0E;
}

// Initialize Port E so PE0, PE1 are buttons
void PortE_Init(void){
	volatile uint32_t delay;

	SYSCTL_RCGCGPIO_R |= 0x10;

	delay = SYSCTL_RCGCGPIO_R;

	GPIO_PORTE_DIR_R   &= ~0x03;
	GPIO_PORTE_DEN_R 	 |= 0x03;
	GPIO_PORTE_AFSEL_R &= ~0x03;
	GPIO_PORTE_PUR_R 	 &= ~0x03;
}

// Initialize Port B so PB3-0 are for the DAC
void PortB_Init(void){
	volatile uint32_t delay;
	SYSCTL_RCGCGPIO_R |= 0x02;
	delay = SYSCTL_RCGCGPIO_R; 

  GPIO_PORTB_DIR_R |= 0x0F;    // 5) outputs on PB3-0
	GPIO_PORTB_DEN_R |= 0x0F;    // 7) enable digital on PB3-0
  GPIO_PORTB_AFSEL_R &= ~0x0F; // 6) regular function on PB3-0
	GPIO_PORTB_DR8R_R |= 0x0F;   
}

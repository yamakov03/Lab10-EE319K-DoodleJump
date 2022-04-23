// SlidePot.cpp
// Runs on TM4C123
// Provide functions that initialize ADC0
// Modified: 1/12/2022 
// Student names: change this to your names or look very silly
// Last modification date: change this to the last modification date or look very silly

#include <stdint.h>
#include "SlidePot.h"
#include "../inc/tm4c123gh6pm.h"


// ADC initialization function 
// Input: none
// Output: none
// measures from PD2, analog channel 5
void ADC_Init(void){ volatile uint32_t delay;
  
  // you write this
  SYSCTL_RCGCGPIO_R |= 0x8;      // 1) activate clock for Port D
  while((SYSCTL_PRGPIO_R&0x8) == 0){};
  GPIO_PORTD_DIR_R &= ~0x4;      // 2) make PD2 input
  GPIO_PORTD_AFSEL_R |= 0x4;     // 3) enable alternate fun on PD2
  GPIO_PORTD_DEN_R &= ~0x4;      // 4) disable digital I/O on PD2
  GPIO_PORTD_AMSEL_R |= 0x4;     // 5) enable analog fun on PD2
  SYSCTL_RCGCADC_R |= 0x01;       // 6) activate ADC0 
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;
  ADC0_PC_R = 0x01;               // 7) configure for 125K 
  ADC0_SSPRI_R = 0x0123;          // 8) Seq 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+5;  // 11) Ain5 (PD2)
  ADC0_SSCTL3_R = 0x0006;         // 12) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~0x0008;           // 13) disable SS3 interrupts
	//ADC0_SAC_R |= 0x4;
  ADC0_ACTSS_R |= 0x0008;         // 14) enable sample sequencer 3
}

//------------ADCIn------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// measures from PD2, analog channel 5
uint32_t ADC_In(void){  
  // you write this
  uint32_t data;
  ADC0_PSSI_R = 0x0008;            
  while((ADC0_RIS_R&0x08)==0){};   
  data = ADC0_SSFIFO3_R&0xFFF; 
  ADC0_ISC_R = 0x0008; 
  return data;
}

// constructor, invoked on creation of class
// m and b are linear calibration coeffients 
SlidePot::SlidePot(uint32_t m, uint32_t b){
  // you write this
	slope = m;
	offset = b;	
}

void SlidePot::Save(uint32_t n){
	distance = (120*n)/4095;
// 3) set semaphore flag = 1
	flag = 1;

}
uint32_t SlidePot::Convert(uint32_t n){
    // you write this

	return (1893*n)/4096 + 57;
}

void SlidePot::Sync(void){
  // you write this
	while(flag == 0){
	}
// 2) set semaphore flag to 0
		flag = 0;
}

uint32_t SlidePot::ADCsample(void){ // return ADC sample value (0 to 4095)
    // you write this

  return data;
}

uint32_t SlidePot::Distance(void){  // return distance value (0 to 2000), 0.001cm
  return distance;
}

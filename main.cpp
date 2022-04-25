// hello this is a test


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
#include "gameEngine.h"

SlidePot my(1500,0);

extern "C" void DisableInterrupts(void);
extern "C" void EnableInterrupts(void);
extern "C" void SysTick_Handler(void);

void SysTick_Init(uint32_t period){
  //*** students write this ******
	NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2          
  NVIC_ST_CTRL_R = 0x07; // enable SysTick with core clock and interrupts
  // enable interrupts after all initialization is finished
}

// Creating a struct for the Sprite.
typedef enum {dead,alive} status_t;

//-------------------------------doodler--------------------------
struct sprite{
  uint32_t x;      // x coordinate
  uint32_t y;      // y coordinate
  const unsigned short *image; // ptr->image
  status_t life;            // dead/alive
};          
typedef struct sprite sprite_t;
int position;

sprite_t bill={60,9,SmallEnemy20pointB,alive};
sprite_t doodler={60, 100, PlayerShip0, alive};

uint32_t time = 0;
uint32_t score = 0;
volatile uint32_t flag;

void background(void){
  flag = 1; // semaphore
  if(doodler.life == alive){
    doodler.x = position;
  }
  if(doodler.y>155){
    doodler.life = dead;
  }
}

void clock(void){
  time++;
}

int main(void){
	
	DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  TExaS_Init();
  Random_Init(1);
  ADC_Init(); 
	ST7735_InitR(INITR_REDTAB);
  Timer0_Init(&background,800000); // 50 Hz
  Timer1_Init(&clock,80000000); // 1 Hz
	SysTick_Init(4000000);
	
	PortF_Init();
	PortE_Init();
	PortB_Init();
  EnableInterrupts();
	
	
  ST7735_DrawBitmap(52, 159, PlayerShip0, 18,8); // player ship middle bottom
	
  while(doodler.life == alive){
    while(flag== 1){
			ST7735_DrawBitmap(doodler.x,doodler.y,doodler.image,16, 10);
		  flag = 0;
		};

    //ST7735_DrawBitmap(bill.x,bill.y,bill.image,16,10);
		
		
  }

  ST7735_FillScreen(0x0000);            // set screen to black
  ST7735_SetCursor(1, 1);
  ST7735_OutString((char*)"GAME OVER");
  ST7735_SetCursor(1, 2);
  ST7735_SetTextColor(ST7735_WHITE);
  ST7735_OutString((char*)"Score: ");
  ST7735_SetCursor(1, 3);
  ST7735_OutUDec(score);
  while(1){
    while(flag==0){};
    flag = 0;
    ST7735_SetCursor(2, 4);
    ST7735_OutUDec(time);
  }

}

int Data; //test
void SysTick_Handler(void){ // every sample
    //*** students write this ******
// toggle heartbeat LED (change from 0 to 1, or from 1 to 0)
	//GPIO_PORTF_DATA_R ^= 0x04;
// sample the ADC calling ADC_In()
	Data = ADC_In();
	flag = 1;
// save the 12-bit ADC sample using the member function Sensor.Save()
	my.Save(Data);
}

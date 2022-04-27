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
typedef enum {dead, alive, dying} status_t;

//-------------------------------doodler--------------------------
struct sprite{
	int32_t oldx;
	int32_t oldy;
  int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	const uint16_t *image; // ptr->image
	int32_t w,h;
	int32_t vx;			// x velocity
	int32_t vy;			// y velocity
  status_t life;         // dead/alive
}; typedef struct sprite sprite_t;

struct platform{
	int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	const uint16_t *image; // ptr->image
	int32_t w,h;
	int32_t vx;			// x velocity
	int32_t vy;			// y velocity
  status_t life;         // dead/alive
}; typedef struct platform platform;

#define MAXGREENPLATFORMS 5
#define MAXBLUEPLATFORMS 2
#define MAXREDENEMIES 2
#define MAXBLUENEMIES 2
#define MAXWIDTH 128
#define MAXHEIGHT 160
#define PLATFORMSPEED 1

sprite_t doodler;
sprite_t redenemy[MAXREDENEMIES];
sprite_t blueenemy[MAXBLUENEMIES];
platform greenplatform[MAXGREENPLATFORMS];
platform blueplatform[MAXBLUEPLATFORMS];


int pauseState;
int fireState;
int nolongerstart = 0;

int score = 0; //global score displayed at top left corner of screen
uint32_t Data;

uint32_t time = 0;
volatile uint32_t flag;
volatile uint32_t slideflag;

void delay100ms(uint32_t count);
void delay10ms(uint32_t count);
void delay1ms(uint32_t count);
//==================================================initialization=========================================
void Init(void){ 
	int i;
	
	doodler.x = 50;
	doodler.y = 80;
	doodler.image = doodler_sprite;
	doodler.w = 23;
	doodler.h = 22;
	doodler.vx = 0; //(Random()%5)-2;	// -2 to 2
	doodler.vy = 1; //(Random()%3);	// 0 to 2
	doodler.life = alive;

	
	for(int i = 0; i < MAXGREENPLATFORMS; i++) {
		greenplatform[i].x = Random()%MAXWIDTH;
		greenplatform[i].y = Random()%MAXHEIGHT;
		greenplatform[i].image = green_platform_sprite;
		greenplatform[i].w = 30;
		greenplatform[i].h = 11;
		greenplatform[i].vx = 0; //green platforms don't move
		greenplatform[i].vy = 0; 	
	}
	
	for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
		blueplatform[i].x = Random()%MAXWIDTH;
		blueplatform[i].y = Random()%MAXHEIGHT;
		blueplatform[i].image = blue_platform_sprite;
		blueplatform[i].w = 31;
		blueplatform[i].h = 11;
		blueplatform[i].vx = 1; 
		blueplatform[i].vy = 0; 
	}
	
	for(int i = 0; i < MAXREDENEMIES; i++) {
		redenemy[i].life = alive;
	}
	
	for(int i = 0; i < MAXBLUENEMIES; i++) {
		blueenemy[i].life = alive;
	}
	
}

//===========================================handle screen updates=========================================================
void Draw(void) {
	
	flag = 1; // semaphore
	
	ST7735_SetCursor(0, 0);
	ST7735_OutUDec(score);
	

	ST7735_DrawBitmap(doodler.oldx, doodler.oldy, white, doodler.w, doodler.h);
	ST7735_DrawBitmap(doodler.x, doodler.y, doodler.image, doodler.w, doodler.h);
	
	
	

	
	if(doodler.x <= 0) doodler.x = MAXWIDTH;
	if(doodler.x > MAXWIDTH) doodler.x = 0;
	
	//collision logic
	for(int i = 0; i < MAXGREENPLATFORMS; i++) {
		if (doodler.y <= greenplatform[i].y + greenplatform[i].h) {
				int minX = greenplatform[i].x - doodler.w;
        int maxX = greenplatform[i].x + greenplatform[i].w;
				if (doodler.x >= minX && doodler.x <= maxX) {

		}
	}
}
		
	for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
		if (doodler.y <= blueplatform[i].y + blueplatform[i].h) {
				int minX = blueplatform[i].x - doodler.w;
        int maxX = blueplatform[i].x + blueplatform[i].w;
				if (doodler.x >= minX && doodler.x <= maxX) {
					nolongerstart = 1;
        }
		}
	}
}
	//fix doodle translate issues
//===============================================================================================

void clock(void){
  time++;
}

float globaldx = 0, globaldy = 0;
int globalx = 50, globaly = 121, globalh = 60;
int oldx, oldy;

int main(void){
	
	DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  TExaS_Init();
  Random_Init(1);
  ADC_Init(); 
	ST7735_InitR(INITR_REDTAB);
  //Timer0_Init(Draw,800000); // 50 Hz, calls draw to update screen  !!!originally was (Draw,800000) == 50 Hz
  //Timer1_Init(clock,80000000); // 1 Hz
	SysTick_Init(4000000);
	Init();
	PortF_Init();
	PortE_Init();
	PortB_Init();		
//==========================start screen============================================================
	ST7735_FillScreen(0xFFFF);
	ST7735_DrawBitmap(10, 40, titlelogo, 104, 26);
	ST7735_SetCursor(3, 14);
	ST7735_OutString("Press any button");
	ST7735_SetCursor(4,15);
	ST7735_OutString("to start game");
	ST7735_DrawBitmap(50, 130, green_platform_sprite, 30, 11);
	
	fireState = 0;
	pauseState = 0;
	doodler.y = 121;
	doodler.x = 50;
	
	while(fireState != 1 && pauseState != 1){
		doodler.y -= doodler.vy;
		
		if (doodler.y <=80){
			doodler.vy *= -1;
		}
		if(doodler.y >= 121){
			doodler.vy *= -1;
		}
		
		delay1ms(1);
	
	ST7735_DrawBitmap(50, doodler.y, doodler.image, 23, 22);
			fireState ^= ((GPIO_PORTE_DATA_R & 0x02) >> 1);
			pauseState ^= ((GPIO_PORTE_DATA_R & 0x01));
	}
//===========================================================================================
	ST7735_FillScreen(0xFFFF); 
	EnableInterrupts();
	
  while(1){
		while(doodler.life == alive){
				my.Sync();
				oldx = globalx;
				oldy = globaly;
				while(slideflag == 1){
					globalx = (120*Data/4095);
					slideflag = 0;
				}
		
				globaldy += 0.2;
				globaly -= globaldy;
				if(globaly < 120) {
					globaldy = 1;
				}
				
				for(int i = 0; i<MAXBLUEPLATFORMS; i++){
					if (doodler.y <= blueplatform[i].y + blueplatform[i].h) {
						int minX = blueplatform[i].x - doodler.w;
						int maxX = blueplatform[i].x + blueplatform[i].w;
						if (doodler.x >= minX && doodler.x <= maxX) {
							globaldy = -1;
						}
					}
				}
				
				if(globaly<globalh){
					for(int i = 0; i < MAXBLUEPLATFORMS; i++){
						globaly=globalh;
						blueplatform[i].y = blueplatform[i].y - globaldy;
						if(blueplatform[i].y > 160){
							blueplatform[i].y = 0;
							blueplatform[i].x = Random()%MAXWIDTH;
						
						}
					}
					
				}
				
				for(int i = 0; i < MAXGREENPLATFORMS; i++) {
					ST7735_DrawBitmap(greenplatform[i].x, greenplatform[i].y, greenplatform[i].image, greenplatform[i].w, greenplatform[i].h);
				}
	
				for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
					blueplatform[i].x += blueplatform[i].vx;
					if(blueplatform[i].x == 100){
						blueplatform[i].vx *= -1;
					}
					if(blueplatform[i].x == 0){
						blueplatform[i].vx *= -1;
					}
					ST7735_DrawBitmap(blueplatform[i].x, blueplatform[i].y, blueplatform[i].image, blueplatform[i].w, blueplatform[i].h);
				}
				
					doodler.y = globaly;
					doodler.x = globalx;
				
					ST7735_DrawBitmap(oldx, oldy, white, doodler.w, doodler.h);
					ST7735_DrawBitmap(doodler.x, doodler.y, doodler.image, doodler.w, doodler.h);
				
//		if((GPIO_PORTE_DATA_R & 0x01) == 1){
//				delay10ms(1);
//				DisableInterrupts();
//				ST7735_FillScreen(0xFFFF);
//				ST7735_DrawBitmap(25, 100, pause, 70, 30);
//				ST7735_SetCursor(4, 11);
//				ST7735_OutString("Press pause");
//				ST7735_SetCursor(4, 12);
//				ST7735_OutString("button to");
//				ST7735_SetCursor(4, 13);
//				ST7735_OutString("unpause");
//				while((GPIO_PORTE_DATA_R & 0x01) != 1){
//				delay10ms(1);
//				}
//			ST7735_FillScreen(0xFFFF);
//			EnableInterrupts();
//			}
		}
	}
	
	if(doodler.life == dead && nolongerstart == 1){
		ST7735_FillScreen(0x0000);            // set screen to black
		ST7735_SetCursor(1, 1);
		ST7735_FillScreen(0xFFFF);
		ST7735_DrawBitmap(10, 40, gameover, 100, 34);
		ST7735_SetCursor(3, 8);
		ST7735_OutString((char*)"Score: ");
		ST7735_SetCursor(4, 8);
		ST7735_OutUDec(score);
	}

    while(flag==0){};
    flag = 0;
    ST7735_SetCursor(2, 4);
    ST7735_OutUDec(time);
  }



void SysTick_Handler(void){ // every sample
	Data = ADC_In();
	slideflag = 1;
	my.Save(Data);
}

void delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
void delay10ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 72724;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
void delay1ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 7272;  // 0.01sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

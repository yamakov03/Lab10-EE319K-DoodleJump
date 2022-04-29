#include <stdint.h>
#include <stdio.h>
#include "../inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "SlidePot.h"
#include "Images.h"
#include "Images2.h"
#include "UART.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "gameEngine.h"

#include "math.h"

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

//========================================the structs===========================================
struct sprite{
	int32_t oldx;
	int32_t oldy;
  int32_t x;      // x coordinate
  float y;      // y coordinate
	const uint16_t *image; // ptr->image
	int32_t w,h;
	int32_t vx;			// x velocity
	float vy;			// y velocity
  status_t life;         // dead/alive
}; typedef struct sprite sprite_t;

struct platform{
	int32_t oldx;
	int32_t oldy;
	int32_t x;      // x coordinate
  int32_t y;      // y coordinate
	const uint16_t *image; // ptr->image
	int32_t w,h;
	int32_t vx;			// x velocity
	int32_t vy;			// y velocity
  status_t life;         // dead/alive
}; typedef struct platform platform;

#define MAXGREENPLATFORMS 8
#define MAXBLUEPLATFORMS 2
#define MAXREDENEMIES 1
#define MAXBLUENEMIES 1
#define MAXWIDTH 100 // 128 is actual width, make smaller so platforms aren't cut off
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

void delay5ms(uint32_t count);
void delay10ms(uint32_t count);
//=========================================initialization=========================================
void Init(void){ 
	int i;
	
	doodler.x = 50;
	doodler.y = 80;
	doodler.image = doodler_sprite;
	doodler.w = 23;
	doodler.h = 22;
	doodler.vx = 0; 
	doodler.vy = 1; 
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
		redenemy[i].x = Random()%MAXWIDTH;
		redenemy[i].y = Random()%MAXHEIGHT;
		redenemy[i].image = red_enemy_sprite;
		redenemy[i].w = 21;
		redenemy[i].h = 14;
		redenemy[i].vx = 0; 
		redenemy[i].vy = 0; 
	}
	
	for(int i = 0; i < MAXBLUENEMIES; i++) {
		blueenemy[i].life = alive;
		blueenemy[i].x = Random()%MAXWIDTH;
		blueenemy[i].y = Random()%MAXHEIGHT;
		blueenemy[i].image = blue_enemy_sprite;
		blueenemy[i].w = 26;
		blueenemy[i].h = 17;
		blueenemy[i].vx = 0; 
		blueenemy[i].vy = 0; 
	}
	
}

void clock(void){
  time++;
}

int height = 121; 
int oldx;
int oldy;
void game();

int main(void){
	
	DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  TExaS_Init();
  Random_Init(1);
  ADC_Init(); 
	ST7735_InitR(INITR_REDTAB);
  //Timer0_Init(Draw,800000); // 50 Hz, calls draw to update screen  !!!originally was (Draw,800000) == 50 Hz
  Timer1_Init(clock,10000000); // 1 Hz
	SysTick_Init(4000000);
	Init();
	PortF_Init();
	PortE_Init();
	PortB_Init();		
//====================================================start screen================================
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
		
		if (doodler.y <=80){    //--------80
			doodler.vy *= -1;			//--------121
		}
		if(doodler.y >= 121){
			doodler.vy *= -1;
		}
		
		delay10ms(1);
	
	ST7735_DrawBitmap(50, doodler.y, doodler.image, 23, 22);
			fireState ^= ((GPIO_PORTE_DATA_R & 0x02) >> 1);
			pauseState ^= ((GPIO_PORTE_DATA_R & 0x01));
	}
	ST7735_FillScreen(0xFFFF);
	EnableInterrupts();
	game();
}
//=======================================main loop====================================================
void game(){
	EnableInterrupts();

					int globalheight = 100;
					doodler.vy = 0; //doodler speed
					
					score = 0;
					sprite_t peashot;
					int dy = -10;
	
	//!! 	COORDINATES START AT BOTTOM LEFT CORNER OF PICTURE

  while(1){
		while(doodler.life == alive){
			
			//slidepot
				my.Sync();
				//doodler.oldx = doodler.x;
				while(slideflag == 1){
					doodler.x = (120*Data/4095);
					slideflag = 0;
				}					
				
			//move the doodler
				//doodler.oldy = doodler.y;
				doodler.vy += 1;
				doodler.y += doodler.vy;		
				if(doodler.y > 160){
					doodler.vy = dy;
				}
				
			//manage platform collisions
				for(int i = 0; i<MAXBLUEPLATFORMS; i++){
					if ((doodler.y > blueplatform[i].y - blueplatform[i].h && doodler.y <= blueplatform[i].y) && doodler.vy > 0 ) {
						int min = blueplatform[i].x;
						int max = blueplatform[i].x + blueplatform[i].w;
						if (doodler.x + doodler.w/2 >= min && doodler.x + doodler.w/2 <= max ){
							doodler.vy = dy;
						}
					}
				}
				
				
				for(int i = 0; i<MAXGREENPLATFORMS; i++){
					if ((doodler.y > greenplatform[i].y - greenplatform[i].h && doodler.y <= greenplatform[i].y) && doodler.vy > 0) {
						int min = greenplatform[i].x;
						int max = greenplatform[i].x + greenplatform[i].w;
						if (doodler.x + doodler.w/2 >= min && doodler.x + doodler.w/2 <= max) {
							doodler.vy = dy;
						}
					}
				}
				
			//update platform x and y
				if(doodler.y < globalheight){
					doodler.y -= dy -1;
					
					score++;
					for(int i = 0; i < MAXBLUEPLATFORMS; i++){
						blueplatform[i].oldy = blueplatform[i].y;
						blueplatform[i].oldx = blueplatform[i].x;
						if(doodler.vy < 0){
							blueplatform[i].y -= dy - 1;
						}
						if(blueplatform[i].y > 181){
							blueplatform[i].y = 0;
							blueplatform[i].x = Random()%MAXWIDTH;
						}
					}
					
					for(int i = 0; i < MAXGREENPLATFORMS; i++){
						greenplatform[i].oldy = greenplatform[i].y;
						greenplatform[i].oldx = greenplatform[i].x;
						if(doodler.vy < 0){
							greenplatform[i].y -= dy - 1;
						}
						if(greenplatform[i].y > 181){
							greenplatform[i].y = 0;
							greenplatform[i].x = Random()%MAXWIDTH;
						}
					}	
					
				}
				
				//draw sprites
				for(int i = 0; i < MAXGREENPLATFORMS; i++) {
					ST7735_DrawBitmap(greenplatform[i].oldx, greenplatform[i].oldy, clearedplatform, greenplatform[i].w, greenplatform[i].h);
					ST7735_DrawBitmap(greenplatform[i].x, greenplatform[i].y, greenplatform[i].image, greenplatform[i].w, greenplatform[i].h);
				}
	
				for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
					blueplatform[i].oldx = blueplatform[i].x;
					blueplatform[i].x += blueplatform[i].vx;
					if(blueplatform[i].x == 100){
						blueplatform[i].vx *= -1;
					}
					if(blueplatform[i].x == 0){
						blueplatform[i].vx *= -1;
					}
					ST7735_DrawBitmap(blueplatform[i].oldx, blueplatform[i].oldy, clearedplatform, blueplatform[i].w, blueplatform[i].h);
					ST7735_DrawBitmap(blueplatform[i].x, blueplatform[i].y, blueplatform[i].image, blueplatform[i].w, blueplatform[i].h);
				}
				
					//draw doodler
				
//				if(doodler.y > 160) {
//					doodler.life = dead;
//				}
				ST7735_DrawBitmap(doodler.oldx, doodler.oldy, white, doodler.w, doodler.h);
				ST7735_DrawBitmap(doodler.x, doodler.y, doodler.image, doodler.w, doodler.h);
				doodler.oldx = doodler.x;
				doodler.oldy = doodler.y;
				
					//draw enemies
//					for(int i = 0; i < MAXREDENEMIES; i++){
//						redenemy[i].oldy = redenemy[i].y;
//						redenemy[i].oldx = redenemy[i].x;
//						if(doodler.vy < 0){
//							redenemy[i].y -= doodler.vy;
//						}
//						if(redenemy[i].y > 181){
//							redenemy[i].y = 0;
//							redenemy[i].x = Random()%MAXWIDTH;
//						}
//						ST7735_DrawBitmap(redenemy[i].x, redenemy[i].y, redenemy[i].image, redenemy[i].w, redenemy[i].h);
//					}	
//					
//					for(int i = 0; i < MAXBLUENEMIES; i++){
//						blueenemy[i].oldy = blueenemy[i].y;
//						blueenemy[i].oldx = blueenemy[i].x;
//						if(doodler.vy < 0){
//							blueenemy[i].y -= doodler.vy;
//						}
//						if(redenemy[i].y > 181){
//							blueenemy[i].y = 0;
//							blueenemy[i].x = Random()%MAXWIDTH;
//						}
//						ST7735_DrawBitmap(blueenemy[i].x, blueenemy[i].y, blueenemy[i].image, blueenemy[i].w, blueenemy[i].h);
//					}
				
					
				
					//peashot logic
					if(peashot.life == alive){
						peashot.y -= peashot.vy;
						ST7735_DrawBitmap(peashot.x, peashot.y, peashot_sprite, 10, 10);
					}
					
//					for(int i = 0; i < MAXREDENEMIES; i++){
//						if((peashot.x > redenemy[i].x) && (peashot.x < redenemy[i].x + redenemy[i].w)){
//							peashot.life = dead;
//							redenemy[i].life = dead;
//						}
//						if ((doodler.y > redenemy[i].y - 20 && doodler.y <= redenemy[i].y)) {
//							int min = redenemy[i].x;
//							int max = redenemy[i].x + redenemy[i].w;
//							if (doodler.x + doodler.w/2 > min && doodler.x + doodler.w/2 < max ){
//								doodler.life = dead;
//							}
//						}
//					}	
//					
//					for(int i = 0; i < MAXBLUENEMIES; i++){
//						if((peashot.x > blueenemy[i].x) && (peashot.x < blueenemy[i].x + blueenemy[i].w)){
//							peashot.life = dead;
//							blueenemy[i].life = dead;
//						}
//						if ((doodler.y > blueenemy[i].y - 20 && doodler.y <= blueenemy[i].y)) {
//							int min = blueenemy[i].x;
//							int max = blueenemy[i].x + blueenemy[i].w;
//							if (doodler.x + doodler.w/2 > min && doodler.x + doodler.w/2 < max ){
//								doodler.life = dead;
//							}
//						}
//					}
					
					//shoot the peashot
					if((GPIO_PORTE_DATA_R & 0x02) == 1){
						delay5ms(1);
						peashot.life = alive;
						peashot.y = doodler.y;
						peashot.x = doodler.x;
						peashot.vy = 4;
						peashot.vx = 0;
					}
				
				//display score and time
				ST7735_SetCursor(0, 0);					
				ST7735_OutString("Score: ");
				ST7735_SetCursor(7, 0);					
				ST7735_OutUDec(score);		
				ST7735_SetCursor(0, 1);					
				ST7735_OutString("Time: ");				
				ST7735_SetCursor(6, 1);					
				ST7735_OutUDec(time);
					
					//pause screen
			if((GPIO_PORTE_DATA_R & 0x01) == 1){
					delay5ms(1);
					DisableInterrupts();
					ST7735_FillScreen(0xFFFF);
					ST7735_SetCursor(4, 5);
					ST7735_OutString("---PAUSED---");
					ST7735_SetCursor(3, 7);
				  ST7735_OutString("Score: ");
				  ST7735_SetCursor(3, 8);
					ST7735_OutUDec(score);
					ST7735_SetCursor(3, 11);
					ST7735_OutString("Press pause");
					ST7735_SetCursor(3, 12);
					ST7735_OutString("button to");
					ST7735_SetCursor(3, 13);
					ST7735_OutString("unpause");
					while((GPIO_PORTE_DATA_R & 0x01) != 1){
					delay10ms(1);
					}
				ST7735_FillScreen(0xFFFF);
				EnableInterrupts();
				}
		}
		
		//print end screen
			if(doodler.life == dead){
				ST7735_FillScreen(0xFFFF);
				
				ST7735_SetCursor(3, 5);
							ST7735_OutString("---GAME OVER!---");
							ST7735_SetCursor(3, 7);
							ST7735_OutString("Score: ");
							ST7735_SetCursor(3, 8);
							ST7735_OutUDec(score);
							ST7735_SetCursor(3, 11);
							ST7735_OutString("Press pause");
							ST7735_SetCursor(3, 12);
							ST7735_OutString("button to");
							ST7735_SetCursor(3, 13);
							ST7735_OutString("play again");
				
				while((GPIO_PORTE_DATA_R & 0x01) != 1){
				delay10ms(1);
					
				}
				doodler.life = alive;
				game();
			}
		}
	}



void SysTick_Handler(void){ // every sample
	Data = ADC_In();
	slideflag = 1;
	my.Save(Data);
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

void delay5ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 72724/2;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

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
#include "Sound.h"

SlidePot my(1500,0);
extern "C" void DisableInterrupts(void);
extern "C" void EnableInterrupts(void);
extern "C" void SysTick_Handler(void);

void SysTick_Init(uint32_t period){
	NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2          
  NVIC_ST_CTRL_R = 0x07; // enable SysTick with core clock and interrupts
}

// Creating a struct for the Sprite.
typedef enum {dead, alive, dying} status_t;

struct sprite{
	int32_t oldx;
	int32_t oldy;
  int32_t x;      // x coordinate
  float y;      // y coordinate
	const uint16_t *image; // ptr->image
	int32_t w,h;
	int32_t vx;			// x velocity
	int32_t vy;			// y velocity
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

#define MAXGREENPLATFORMS 6
#define MAXBLUEPLATFORMS 2

sprite_t doodler;
sprite_t redenemy;
sprite_t blueenemy;
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
void delay50ms(uint32_t count);

int MAXWIDTH = 100; // 128 is actual width, make smaller so platforms aren't cut off
int MAXHEIGHT = 160;
int PLATFORMSPEED =1;
//=========================================initialization=========================================
void Init(void){
	doodler.x = 50;
	doodler.y = 80;
	doodler.image = doodler_sprite;
	doodler.w = 19;
	doodler.h = 18;
	doodler.vx = 0; 
	doodler.vy = 1; 
	doodler.life = alive;

	for(int i = 0; i < MAXGREENPLATFORMS; i++) {
		greenplatform[i].x = Random()%MAXWIDTH;
		greenplatform[i].y = Random()%MAXHEIGHT;
		greenplatform[i].image = green_platform_sprite;
		greenplatform[i].w = 26;
		greenplatform[i].h = 7;
		greenplatform[i].vx = 0; //green platforms don't move
		greenplatform[i].vy = 0; 	
	}
	
	for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
		blueplatform[i].x = Random()%MAXWIDTH;
		blueplatform[i].y = Random()%MAXHEIGHT;
		blueplatform[i].image = blue_platform_sprite;
		blueplatform[i].w = 27;
		blueplatform[i].h = 7;
		blueplatform[i].vx = 1; 
		blueplatform[i].vy = 0; 
	}
	
	//make sure that there are reachable platforms
	int badcount = 0;
	for(int i = 0; i < MAXGREENPLATFORMS; i++) {
		if(greenplatform[i].y < 60){
			badcount++;
		}
	}
	for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
		if(blueplatform[i].y < 60){
			badcount++;
		}
	}
	if(badcount > 4){
		Init();
	}
	badcount = 0;
	
	for(int i = 0; i < MAXGREENPLATFORMS; i++) {
		if(greenplatform[i].y > 120){
			badcount++;
		}
	}
	for(int i = 0; i < MAXBLUEPLATFORMS; i++) {
		if(blueplatform[i].y < 120){
			badcount++;
		}
	}
	if(badcount > 4){
		Init();
	}
	
	redenemy.life = alive;
	redenemy.x = Random()%MAXWIDTH;
	redenemy.y = Random()%MAXHEIGHT;
	redenemy.image = red_enemy_sprite;
	redenemy.w = 17;
	redenemy.h = 12;
	redenemy.vx = 0; 
	redenemy.vy = 0; 


	blueenemy.life = alive;
	blueenemy.x = Random()%MAXWIDTH;
	blueenemy.y = Random()%MAXHEIGHT;
	blueenemy.image = blue_enemy_sprite;
	blueenemy.w = 22;
	blueenemy.h = 13;
	blueenemy.vx = 0; 
	blueenemy.vy = 0; 
}

void clock(void){
  time++;
	score++;
}

int height = 121; 
int oldx;
int oldy;
void game();

int language = 0x0; //0 for english, 1 for spanish

void printlanguage(){
	ST7735_FillRect(0, 120, 128, 40, ST7735_WHITE);
	if(language == 0){	
		ST7735_SetCursor(0, 13);
		ST7735_OutString("Press pause to start");
		ST7735_SetCursor(4, 14);
		ST7735_OutString("Press shoot");
		ST7735_SetCursor(4,15);
		ST7735_OutString("for Spanish");
	} 
	else if(language == 1){
		ST7735_SetCursor(3, 12);
		ST7735_OutString("Presione pausa");
		ST7735_SetCursor(3, 13);
		ST7735_OutString("para comenzar");
		ST7735_SetCursor(3, 14);
		ST7735_OutString("Sesi\xA2n de prensa");
		ST7735_SetCursor(3,15);
		ST7735_OutString("para espa\xA4ol");
	}
}

//doodler death by enemies
void Death() {
	if ((doodler.y > redenemy.y && doodler.y < (redenemy.y + redenemy.h)) && doodler.vy < 0) {
		int min = redenemy.x;
		int max = redenemy.x + redenemy.w;
		if (doodler.x + doodler.w > min && doodler.x < max && redenemy.life == alive){
			doodler.life = dead;
		}
	}

	if ((doodler.y > blueenemy.y && doodler.y < (blueenemy.y + blueenemy.h)) && doodler.vy < 0) {
		int min = blueenemy.x;
		int max = blueenemy.x + blueenemy.w;
		if (doodler.x + doodler.w > min && doodler.x  < max && blueenemy.life == alive){
			doodler.life = dead;
		}
	}
}
int main(void){
	DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  TExaS_Init();
  Random_Init(1);
  ADC_Init(); 
	ST7735_InitR(INITR_REDTAB);
	Timer0_Init(Death,800000);
  Timer1_Init(clock,64000000); // 1 Hz
	
	Sound_Init();
	SysTick_Init(4000000);
	Init();
	PortF_Init();
	PortE_Init();
	PortB_Init();		
//====================================================start screen================================
	ST7735_FillScreen(0xFFFF);
	ST7735_DrawBitmap(10, 40, titlelogo, 104, 26);
	printlanguage();
	ST7735_DrawBitmap(50, 120, green_platform_sprite, greenplatform[0].w, greenplatform[0].h);
	
	fireState = 0;
	pauseState = 0;
	doodler.y = 110;
	doodler.x = 50;
	
	while(pauseState != 1){
		doodler.y -= doodler.vy;
		
		if (doodler.y <=70){    //--------80
			doodler.vy *= -1;			//--------121
		}
		if(doodler.y >= 110){
			doodler.vy *= -1;
		}
		delay5ms(1);
	
		ST7735_DrawBitmap(doodler.oldx, doodler.oldy, white, doodler.w, doodler.h);
		ST7735_DrawBitmap(doodler.x, doodler.y, doodler.image, doodler.w, doodler.h);
		doodler.oldx = doodler.x;
		doodler.oldy = doodler.y;
		fireState ^= ((GPIO_PORTE_DATA_R & 0x02) >> 1);
		pauseState ^= ((GPIO_PORTE_DATA_R & 0x01));
			
		if((GPIO_PORTE_DATA_R & 0x02) >> 1 == 1){
			delay50ms(1);
			language ^= 0x1;
			printlanguage();
		}
	}
	
	ST7735_FillScreen(0xFFFF);
	EnableInterrupts();
	game();
}

//=======================================main loop====================================================
void game(){
	delay50ms(2);
	EnableInterrupts();

	int globalheight = 100;
	doodler.vy = 0; //doodler speed
	
	score = 0;
	sprite_t peashot;
	int vy = -12;
	int gamestarted = 0;
	
	//!! 	COORDINATES START AT BOTTOM LEFT CORNER OF PICTURE

  while(1){
		while(doodler.life == alive){
			if(score < 5) {
				blueenemy.life = dead;
				redenemy.life = dead;
			}
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
				playsound(jump);
				doodler.vy = vy;
			}
				
			//manage platform collisions
			for(int i = 0; i<MAXBLUEPLATFORMS; i++){
				if ((doodler.y > blueplatform[i].y - blueplatform[i].h && doodler.y <= blueplatform[i].y) && doodler.vy > 0 ) {
					if (doodler.x + doodler.w/2 >= blueplatform[i].x && doodler.x + doodler.w/2 <= blueplatform[i].x + blueplatform[i].w ){
						playsound(jump);
						doodler.vy = vy;
						gamestarted = 1;
					}
				}
			}
			for(int i = 0; i<MAXGREENPLATFORMS; i++){
				if ((doodler.y > greenplatform[i].y - greenplatform[i].h && doodler.y <= greenplatform[i].y) && doodler.vy > 0) {
					if (doodler.x + doodler.w/2 >= greenplatform[i].x && doodler.x + doodler.w/2 <= greenplatform[i].x + greenplatform[i].w) {
						playsound(jump);
						doodler.vy = vy;
						gamestarted = 1;
					}
				}
			}
					
			//jumping on top of enemies
			if ((doodler.y > redenemy.y - redenemy.h && doodler.y <= redenemy.y) && doodler.vy > 0) {
				if (doodler.x + doodler.w/2 >= redenemy.x && doodler.x + doodler.w/2 <= redenemy.x + redenemy.w && redenemy.life == alive) {
					playsound(jump);
					doodler.vy = vy;
					gamestarted = 1;
					redenemy.life = dead;
				}
			}
			if ((doodler.y > blueenemy.y - blueenemy.h && doodler.y <= blueenemy.y) && doodler.vy > 0 ) {
				if (doodler.x + doodler.w/2 >= blueenemy.x && doodler.x + doodler.w/2 <= blueenemy.x + blueenemy.w && blueenemy.life == alive){
					playsound(jump);
					doodler.vy = vy;
					gamestarted = 1;
					blueenemy.life = dead;
				}
			}
			
			//peashot logic
			if(peashot.life == alive){
				peashot.oldy = peashot.y;
				peashot.oldx = peashot.x;
				peashot.y -= peashot.vy;
				ST7735_DrawBitmap(peashot.oldx, peashot.oldy, peashotclear, 10, 10);
				ST7735_DrawBitmap(peashot.x, peashot.y, peashot_sprite, 10, 10);
			}
				
			//shoot the peashot
			if((GPIO_PORTE_DATA_R & 0x02)>>1 == 1){
				playsound(shoot);
				delay5ms(1);
				ST7735_DrawBitmap(peashot.x, peashot.y, peashotclear, 10, 10);
				peashot.life = alive;
				peashot.y = doodler.y;
				peashot.x = doodler.x;
				peashot.vy = 4;
				peashot.vx = 0;
				
			}
					
			if(((peashot.x + 5> redenemy.x) && (peashot.x + 5< redenemy.x + redenemy.w)) && peashot.y < redenemy.y){
				peashot.life = dead;
				redenemy.life = dead;
				score += 10;
			}

			if(((peashot.x + 5> blueenemy.x) && (peashot.x + 5 < blueenemy.x + blueenemy.w)) && peashot.y < blueenemy.y){
				peashot.life = dead;
				blueenemy.life = dead;
				score += 10;
			}

				
				
			//update sprite x and y
			if(doodler.y < globalheight){
				doodler.y -= vy;
				
				score++;
				for(int i = 0; i < MAXBLUEPLATFORMS; i++){
					
					if(doodler.vy < 0){
						blueplatform[i].y -= vy;
					}
					if(blueplatform[i].y > 165 + blueplatform[i].h){
						blueplatform[i].y = 0;
						blueplatform[i].x = Random()%MAXWIDTH;
					}
				}
				
				for(int i = 0; i < MAXGREENPLATFORMS; i++){
					if(doodler.vy < 0){
						greenplatform[i].y -= vy;
					}
					if(greenplatform[i].y > 165 + greenplatform[i].h){
						greenplatform[i].y = 0;
						greenplatform[i].x = Random()%MAXWIDTH;
					}
				}	
					
				if(doodler.vy < 0){
					redenemy.y  -= vy;
				}
				if(redenemy.y > 160){
					redenemy.life = alive;
					redenemy.y = 0;
					redenemy.x = Random()%MAXWIDTH;
				}

			

				if(doodler.vy < 0){
					blueenemy.y  -= vy;
				}
				if(blueenemy.y > 160){
					blueenemy.life = alive;
					blueenemy.y = 0;
					blueenemy.x = Random()%MAXWIDTH;
				}
			}

				
			//draw platforms
			for(int i = 0; i < MAXGREENPLATFORMS; i++) {
				ST7735_DrawBitmap(greenplatform[i].oldx, greenplatform[i].oldy, clearedplatform, greenplatform[i].w, greenplatform[i].h);
				ST7735_DrawBitmap(greenplatform[i].x, greenplatform[i].y, greenplatform[i].image, greenplatform[i].w, greenplatform[i].h);
				greenplatform[i].oldy = greenplatform[i].y;
				greenplatform[i].oldx = greenplatform[i].x;
				
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
				blueplatform[i].oldy = blueplatform[i].y;
				blueplatform[i].oldx = blueplatform[i].x;
			}
				
				
			//draw enemies	
			ST7735_DrawBitmap(redenemy.oldx, redenemy.oldy, redclear, redenemy.w, redenemy.h);
			if(redenemy.life == alive){
				ST7735_DrawBitmap(redenemy.x, redenemy.y, redenemy.image, redenemy.w, redenemy.h);
					redenemy.oldy = redenemy.y;
					redenemy.oldx = redenemy.x;
				
			}
			else {ST7735_DrawBitmap(redenemy.oldx, redenemy.oldy, redclear, redenemy.w, redenemy.h);}
		
			ST7735_DrawBitmap(blueenemy.oldx, blueenemy.oldy, blueclear, blueenemy.w, blueenemy.h);
			if(blueenemy.life == alive){
				ST7735_DrawBitmap(blueenemy.x, blueenemy.y, blueenemy.image, blueenemy.w, blueenemy.h);
					blueenemy.oldy = blueenemy.y;
					blueenemy.oldx = blueenemy.x;
			}
			else {ST7735_DrawBitmap(blueenemy.oldx, blueenemy.oldy, blueclear, blueenemy.w, blueenemy.h);}

			
			//draw doodler
			if(doodler.y > 160 && gamestarted == 1) {
				doodler.life = dead;
			}
			
			ST7735_DrawBitmap(doodler.oldx, doodler.oldy, white, doodler.w, doodler.h);
			ST7735_DrawBitmap(doodler.x, doodler.y, doodler.image, doodler.w, doodler.h);
			doodler.oldx = doodler.x;
			doodler.oldy = doodler.y;

				
			//display score and time
			if(language == 0){
				ST7735_SetCursor(0, 0);					
				ST7735_OutString("Score: ");
				ST7735_SetCursor(7, 0);					
				ST7735_OutUDec(score);		
				ST7735_SetCursor(0, 1);					
				ST7735_OutString("Time: ");				
				ST7735_SetCursor(6, 1);					
				ST7735_OutUDec(time);
			}
			else if(language == 1){
				ST7735_SetCursor(0, 0);					
				ST7735_OutString("Puntaje: ");
				ST7735_SetCursor(8, 0);					
				ST7735_OutUDec(score);		
				ST7735_SetCursor(0, 1);					
				ST7735_OutString("Tiempo: ");				
				ST7735_SetCursor(7, 1);					
				ST7735_OutUDec(time);
			}
			
					
			//pause screen
			if((GPIO_PORTE_DATA_R & 0x01) == 1){
				DisableInterrupts();				
				delay50ms(1);
				ST7735_FillScreen(0xFFFF);
				
				if(language == 0){
					ST7735_SetCursor(4, 4);
					ST7735_OutString("---PAUSED---");
					ST7735_SetCursor(3, 6);
				  ST7735_OutString("Score: ");
				  ST7735_SetCursor(3, 7);
					ST7735_OutUDec(score);
					ST7735_SetCursor(3, 9);
					ST7735_OutString("Press pause");
					ST7735_SetCursor(3, 10);
					ST7735_OutString("button to");
					ST7735_SetCursor(3, 11);
					ST7735_OutString("unpause");
					ST7735_SetCursor(3, 13);
					ST7735_OutString("Press shoot");
					ST7735_SetCursor(3,14);
					ST7735_OutString("for Spanish");
				}
				else if(language == 1){
					ST7735_SetCursor(5, 4);
					ST7735_OutString("--EN PAUSA--");
					ST7735_SetCursor(3, 6);
				  ST7735_OutString("Puntaje: ");
				  ST7735_SetCursor(3, 7);
					ST7735_OutUDec(score);
					ST7735_SetCursor(3, 9);
					ST7735_OutString("Pulse pausa");
					ST7735_SetCursor(3, 10);
					ST7735_OutString("para reanudar");
					ST7735_SetCursor(3, 11);
					ST7735_OutString("la pausa");
					ST7735_SetCursor(3, 13);
					ST7735_OutString("Sesi\xA2n de prensa");
					ST7735_SetCursor(4,14);
					ST7735_OutString("para espa\xA4ol");
				}
				while((GPIO_PORTE_DATA_R & 0x01) != 1){
					if((GPIO_PORTE_DATA_R & 0x02) >> 1 == 1){
						delay50ms(1);
						language ^= 0x1;
						ST7735_FillRect(0, 0, 128, 10, ST7735_WHITE);
						ST7735_SetCursor(0, 0);
						if(language == 1){
							ST7735_OutString("Espa\xA4ol");
						} else if (language == 0){
							ST7735_OutString("English");
						}
					}
					delay50ms(1);
				}
				delay50ms(1);
				ST7735_FillScreen(0xFFFF);
				EnableInterrupts();
			}
		}
		
		//print end screen
		if(doodler.life == dead){
			DisableInterrupts();
			ST7735_FillScreen(0xFFFF);
			
			if(language == 0){				
				ST7735_SetCursor(3, 5);
				ST7735_OutString("---GAME OVER!---");
				ST7735_SetCursor(3, 7);
				ST7735_OutString("Score: ");
				ST7735_SetCursor(3, 8);
				ST7735_OutUDec(score);
				ST7735_SetCursor(3, 11);
				ST7735_OutString("Press any");
				ST7735_SetCursor(3, 12);
				ST7735_OutString("button to");
				ST7735_SetCursor(3, 13);
				ST7735_OutString("play again");
			} 
			else if(language == 1){
				ST7735_SetCursor(1, 5);
				ST7735_OutString("-\xADJUEGO TERMINADO!-");
				ST7735_SetCursor(3, 7);
				ST7735_OutString("Puntaje: ");
				ST7735_SetCursor(3, 8);
				ST7735_OutUDec(score);
				ST7735_SetCursor(2, 11);
				ST7735_OutString("Presiona cualquier");
				ST7735_SetCursor(2, 12);
				ST7735_OutString("cordero para");
				ST7735_SetCursor(2, 13);
				ST7735_OutString("volver a jugar");
			}

			while(((GPIO_PORTE_DATA_R & 0x01) != 1) && ((GPIO_PORTE_DATA_R & 0x02)>>1 != 1)){
			delay50ms(1);
			}
			delay50ms(2);
			score = 0;
			time = 0;
			doodler.life = alive;
			ST7735_FillScreen(0xFFFF);
			Init();
			EnableInterrupts();
			game();
		}
	}
}



void SysTick_Handler(void){ // every sample
	Data = ADC_In();
	slideflag = 1;
	my.Save(Data);
}

void delay50ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 72724*5;  // 0.1sec at 80 MHz
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

void delay5ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 72724/2;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"

void delays(int t);
void initGPIO();
void step1();
void step2();
void step3();
void step4();
void cwRotation();
void ccwRotation();
int delayCount = 0;

int main(void){
	initGPIO();

	// GPIOA->ODR |= 0b110011;
	while(1){
		cwRotation();
		delayCount = 0 ;
		delays(500); //half second delay
		ccwRotation();
		delayCount = 0;
	}
	return 0;
}

//16 MHz SYSCLK delay
void delays(int t){
	SysTick->LOAD = 16000;
	SysTick->VAL = 0;
	SysTick->CTRL = 0x5;
	for(int i=0; i<t; i++){
		while((SysTick->CTRL & 0x10000)==0){}
	}
	SysTick->CTRL = 0;
}

void initGPIO(){
	RCC->AHB1ENR |= 1; //enables GPIOA clock
	GPIOA->MODER &= ~0xF0F; /*clears the pin mode*/
	GPIOA->MODER |= 0x505; //sets pins A0, A1, A4, and A5 to output
}

void step1(){
	GPIOA->ODR |= 0b100001; //turns on PA5 and PA0
	delays(10);
	GPIOA->ODR &= ~0b100001;
	delayCount += 1;
}

void step2(){
	GPIOA->ODR |= 0x3; //turns on PA1 and PA0
	delays(10);
	GPIOA->ODR &= ~0x3;
	delayCount += 1;
}

void step3(){
	GPIOA->ODR |= 0x12; //turns on PA1 and PA4
	delays(10);
	GPIOA->ODR &= ~0x12;
	delayCount += 1;
}

void step4(){
	GPIOA->ODR |= 0x30; //turns on PA5 and PA4
	delays(10);
	GPIOA->ODR &= ~0x30;
	delayCount += 1;
}

void cwRotation(){ //clockwise rotation
	while(delayCount<626){ //runs the while loop until 125 delays (5 seconds)
		step1();
		step2();
		step3();
		step4();
	}
}

void ccwRotation(){ //counterclockwise rotation
	while(delayCount<626){ //runs the while loop until 125 delays (5 seconds)
		step4();
		step3();
		step2();
		step1();
	}
}

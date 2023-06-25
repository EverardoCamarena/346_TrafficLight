// CECS346 Project 1: Traffic light controller with pedestrian interaction
// Student Name: Everardo Camarena
// Lab description: We created a program which simulates a safe traffic light for pedestrians and cars. Buttons provide pedestrians with the ability to change the flow of traffic

// Hardware Design
// 1)	Port E will be used for the three switches/sensors: Pedestrian walk button(PE0), West sensor(PE1), South sensor(PE2)
// 2)	Port B will be used to control 6 LEDs: Green south(PB0), Yellow south(PB1), Red south(PB2), Green west(PB3), Yellow west(PB4), Red west(PB5)
// 3)	Port F will be used to control 2 LEDs: Red don't walk(PF1), Green walk(PF3)

// For data type alias
#include <stdint.h>
#include "SysTick.h"
#include "tm4c123gh6pm.h"

// Registers for switches
#define SENSORS									(*((volatile unsigned long *)0x4002401C)) // bit addresses for PE0-PE2
#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
//#define GPIO_PORTE_PDR_R        (*((volatile unsigned long *)0x40004000))

// Registers for LEDs
#define T_LIGHT                 (*((volatile unsigned long *)0x400050FC)) // bit addresses for PB0-PB5
#define GPIO_PORTB_DATA_R       (*((volatile unsigned long *)0x400053FC))
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))
	
// Registers for LEDs
#define P_LIGHT                 (*((volatile unsigned long *)0x40025038)) // bit addresses for PF1 & PF3
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))

// System control register
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

void Sensors_Init(void);
void T_LIGHT_Init(void);
void P_LIGHT_Init(void);

// FSM state data structure
struct State {
  uint32_t Out1;
	uint32_t Out2;
  uint32_t Time;  
  uint32_t Next[8];
}; 
typedef const struct State STyp;
// Constants definitions
//enum my_states {};
#define GoS       0
#define WaitS     1
#define GoW       2
#define WaitW     3
#define GoP       4
#define WaitPOn1  5
#define WaitPOff1 6
#define WaitPOn2  7
#define WaitPOff2 8

// Output pins are: Lights 8(WaitPOff2), 7(WaitPOn2), 6(WaitPOff1), 5(WaitPOn1), 4(GoP), 3(WaitW), 2(GoW), 1(WaitS), 0(GoS)
// Input pins are: Switches 2:south, 1:west, 0:pedestrian 
STyp FSM[9]={
	{0x21, 0x02, 8,{GoS, WaitS, WaitS, WaitS, GoS, WaitS, WaitS, WaitS}},
	{0x22, 0x02, 4,{GoW, GoP, GoW, GoW, GoP, GoP, GoW, GoW}},
	{0x0C, 0x02, 8,{GoW, WaitW, GoW, WaitW, WaitW, WaitW, WaitW, WaitW}},
	{0x14, 0x02, 4,{GoP, GoP, GoP, GoP, GoS, GoP, GoS, GoP}},
	{0x24, 0x08, 8,{GoP, GoP, WaitPOn1, WaitPOn1, WaitPOn1, WaitPOn1, WaitPOn1, WaitPOn1}},
	{0x24, 0x02, 1,{WaitPOff1, WaitPOff1, WaitPOff1, WaitPOff1, WaitPOff1, WaitPOff1, WaitPOff1, WaitPOff1}},
	{0x24, 0x00, 1,{WaitPOn2, WaitPOn2, WaitPOn2, WaitPOn2, WaitPOn2, WaitPOn2, WaitPOn2, WaitPOn2}},
	{0x24, 0x02, 1,{WaitPOff2, WaitPOff2, WaitPOff2, WaitPOff2, WaitPOff2, WaitPOff2, WaitPOff2, WaitPOff2}},
	{0x24, 0x00, 1,{GoS, GoS, GoW, GoW, GoS, GoS, GoS, GoS}}
};   
int main(void){ 
  // Index to the current state
	uint32_t S; 
  uint32_t Input;
	
	// Call initialization functions
	SysTick_Init();
	Sensors_Init();
	T_LIGHT_Init();
	P_LIGHT_Init();
	
	// FSM start with green
  S = GoS;
  T_LIGHT = 0xFF;
	
  while(1){
	  // Put your FSM engine here
		T_LIGHT = FSM[S].Out1;
		P_LIGHT = FSM[S].Out2;
		Wait_QuarterSecond(FSM[S].Time); // 0.25 seconds
		Input = SENSORS;
		S = FSM[S].Next[Input];
  }
}
void Sensors_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;                              // Activate Port E clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOE)!=SYSCTL_RCGC2_GPIOE){}  // wait for clock to be active
		
	GPIO_PORTE_AMSEL_R &= ~0x07;                       // Disable analog function on PE2-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF;                  // Enable regular GPIO
  GPIO_PORTE_DIR_R &= ~ 0x07;                        // Inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= 0x07;                        // Regular function on PE2-0
  GPIO_PORTE_DEN_R |= 0x07;                          // Enable digital signals on PE2-0
//GPIO_PORTE_PDR_R                                       Optional: Enable pull-down resistors for PE2-0
}
void T_LIGHT_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;                              // Activate Port B clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOB)!=SYSCTL_RCGC2_GPIOB){}  // wait for clock to be active
		
  GPIO_PORTB_AMSEL_R &= ~0x3F;                       // Disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x000000FF;                  // Enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;                          // Outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F;                       // Regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;                          // Enable digital on PB5-0
}
void P_LIGHT_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;                              // Activate Port F clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOF)!=SYSCTL_RCGC2_GPIOF){}  // wait for clock to be active
		
  GPIO_PORTF_AMSEL_R &= ~0x0A;                       // Disable analog function on PF3 & PF1
  GPIO_PORTF_PCTL_R &= ~0x000000FF;                  // Enable regular GPIO
  GPIO_PORTF_DIR_R |= 0x0A;                          // Outputs on PF3 & PF1
  GPIO_PORTF_AFSEL_R &= ~0x0A;                       // Regular function on PF3 & PF1
  GPIO_PORTF_DEN_R |= 0x0A;                          // Enable digital on PF3 & PF1
}
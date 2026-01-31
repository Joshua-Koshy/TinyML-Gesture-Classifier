// ----------------------------------------------------------------------------
// Lab1.c
// Jonathan Valvano
// July 8, 2024
// Possible main program to test the lab
// Feel free to edit this to match your specifications
#include <stdint.h>
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"

#define Input_Button_Pin (1<<4) // 0001 0000
#define Output_LED_Pin (1<<7)	 // 1000 0000

void Clock_Delay1ms(uint32_t n);
void LED_Init(void);
void LED_Out(uint32_t data);
void Switch_Init(void);
uint32_t Switch_In(void);

typedef struct{
  uint8_t out;
  uint8_t next[2];
} State_t;


#define OFF_R0 0
#define OFF_T1 1
#define OFF_R1 2
#define OFF_T2 3
#define ON_R2 4
#define ON_T3 5
#define ON_R3 6
#define ON_T0 7


State_t FSM[8]={
  {0,{OFF_R0,OFF_T1}},
  {0,{OFF_R1,OFF_T1}},
  {0,{OFF_R1,OFF_T2}},
  {0,{ON_R2, OFF_T2}},
	{1,{ON_R2, ON_T3}},
	{1,{ON_R3, ON_T3}},
	{1,{ON_R3, ON_T0}},
	{0,{OFF_R0, ON_T0}},
};












int main(void){
  PLL_Init(Bus80MHz); 
  LED_Init();
  Switch_Init();
  // Write something to declare required variables
  uint8_t state = OFF_R0;
  // Write something to initalize the state of the FSM, LEDs, and variables as needed

  while(1){
      // Write something using Switch_In() and LED_Out() to implement the behavior in the lab doc
		  LED_Out(FSM[state].out);     // output depends on state
      Clock_Delay1ms(10);          // debounce delay (Option 1)    //delay after first edge
      state = FSM[state].next[Switch_In()];
  } 
} 
//DIR, DEN, DATA
//AFSEL, PCTL, AMSEL
void LED_Init(void){
	
	SYSCTL_RCGCGPIO_R |= 0x04;                 // enable Port C clock
  while((SYSCTL_PRGPIO_R & 0x04) == 0){};    // ready?
		
	GPIO_PORTC_AMSEL_R &= ~Output_LED_Pin;     // enables GPIO by setting to 0
  GPIO_PORTC_PCTL_R  &= ~(0xF << 28);        // clears PCTL but lowk not needed, just good practice
  GPIO_PORTC_AFSEL_R &= ~Output_LED_Pin;     // analog mode select to 0
	GPIO_PORTC_DIR_R   |=  Output_LED_Pin;     // set PC7 as output (direction)
  GPIO_PORTC_DEN_R   |=  Output_LED_Pin;     // digital enable
  GPIO_PORTC_DATA_R  &= ~Output_LED_Pin;     // start data as low   //LOWPASS FILTER REVIEW< HOW TO RESIST IF I WANT TO DEBOUJNCE HARDWARE 
    
}
void LED_Out(uint32_t data){
    // write something that sets the state of the GPIO pin as required
	if(data) GPIO_PORTC_DATA_R |= Output_LED_Pin;     // set PC7 high
  else     GPIO_PORTC_DATA_R &= ~Output_LED_Pin; 
}
void Switch_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x02;                 // enable Port B clock
  while((SYSCTL_PRGPIO_R & 0x02) == 0){};    // wait ready

  GPIO_PORTB_AMSEL_R &= ~Input_Button_Pin;             // enables GPIO by setting to 0
  GPIO_PORTB_PCTL_R  &= ~(0xF << 16);        // clears PCTL but lowk not needed, just good practice
  GPIO_PORTB_AFSEL_R &= ~Input_Button_Pin;             // analog mode select to 0
  GPIO_PORTB_DIR_R   &= ~Input_Button_Pin;             // set PB4 as output (direction)
  GPIO_PORTB_DEN_R   |=  Input_Button_Pin;             // digital enable
  GPIO_PORTB_PDR_R   |=  Input_Button_Pin;
    
}
uint32_t Switch_In(void){
  // if Button is pressed then 1, else 0
	if(GPIO_PORTB_DATA_R & Input_Button_Pin){  
    return 1;
  } else {
    return 0;
  }
}

void Clock_Delay(uint32_t ulCount){
  while(ulCount){
    ulCount--;
  }
}

// ------------Clock_Delay1ms------------
// Simple delay function which delays about n milliseconds.
// Inputs: n, number of msec to wait
// Outputs: none
void Clock_Delay1ms(uint32_t n){
  while(n){
    Clock_Delay(23746);  // 1 msec, tuned at 80 MHz, originally part of LCD module
    n--;
  }
}

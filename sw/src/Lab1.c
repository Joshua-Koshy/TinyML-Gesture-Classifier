// ----------------------------------------------------------------------------
// Lab1.c
// Jonathan Valvano
// July 8, 2024
// Possible main program to test the lab
// Feel free to edit this to match your specifications
#include <stdint.h>
#include "PLL.h"
#include "../inc/tm4c123gh6pm.h"

void Clock_Delay1ms(uint32_t n);
void LED_Init(void);
void LED_Out(uint32_t data);
void Switch_Init(void);
uint32_t Switch_In(void);

int main(void){
  PLL_Init(Bus80MHz); 
  LED_Init();
  Switch_Init();
  // Write something to declare required variables
  // Write something to initalize the state of the FSM, LEDs, and variables as needed

  while(1){
      // Write something using Switch_In() and LED_Out() to implement the behavior in the lab doc
  } 
} 

void LED_Init(void){
    // Write something to initalize the GPIOs that drive the LEDs based on your EID as defined in the lab doc.
}
void LED_Out(uint32_t data){
    // write something that sets the state of the GPIO pin as required
}
void Switch_Init(void){
    // write something to initalize the GPIO that take input from the switches based on your EID as defined in the lab doc
}
uint32_t Switch_In(void){
  // write something that reads the state of the GPIO pin as required
  return 42;
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

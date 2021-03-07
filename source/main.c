/*	Author: Nicole Navarro
 *  Partner(s) Name: 
 *	Lab Section:21
 *	Assignment: Lab #13  Exercise #2
 *	Video: https://youtu.be/H0cNLkUngjQ
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "timer.h"
#include "scheduler.h"
#endif

unsigned short joystick;
unsigned char pattern = 0x80;
unsigned char row = 0xFE;
unsigned long speed = 100;
//for my hardware, at neutral the ADC is 504 = 0x1F8

void transmit_data(unsigned char data, unsigned char reg) {
    //for some reason they values come out weird so you have to take each nibble, switch them and flip each nibble but not in the sense that you just flip 1 to 0 and 0 to 1, more like making abcd to dcba 
    data = (data & 0xF0) >> 4 | (data & 0x0F) << 4; 
    data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
    data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
	
    int i;
    if (reg == 1) {
        for (i = 0; i < 8 ; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        PORTC = 0x08;
            // set SER = next bit of data to be sent.
            PORTC |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTC |= 0x02;  
        }
        // set RCLK = 1. 
        PORTC |= 0x04;
    }

    else if (reg == 2) {
        for (i = 0; i < 8 ; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        PORTD = 0x08;
            // set SER = next bit of data to be sent.
            PORTD |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTD |= 0x02;  
        }
        // set RCLK = 1. 
        PORTD |= 0x04;
    }
}

void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}

enum Shift_States{wait, left, right}Shift_State;
int Shift_Tick(int Shift_State){
	switch(Shift_State){
		case wait:
			if(joystick == 0x1F8){
				Shift_State = wait;
			}
			else if(joystick < (0x1F8 - 0x0F)){
				Shift_State = left;
			}
			else if(joystick > (0x1F8 + 0x0F)){
				Shift_State = right;
			}
			break;
		case left:
			if(pattern == 0x80){
				pattern = 0x01;
			}
			else{
				pattern = pattern << 1; 
			}
			Shift_State = wait;
			break;
		case right:
			if(pattern == 0x01){
				pattern = 0x80;
			}
			else{
				pattern = pattern >> 1;
			}
			Shift_State = wait;
		       break;	
		default: Shift_State = wait; break;
	}
	return Shift_State;
}

enum Speed_States{stop, range1000, range500, range250, range100}Speed_State;
int Speed_Tick(int Speed_State){
	//here are the different sectors since i use the norm +- 15 just as an offset meaning for moving left and righ the mins are
	//504 +- 15 --> 489, 519 
	//max for right: 1008
	//max for left: 15
	//489-15 -> 474 /4 --> 118.5 --> 489-371, 371-253, 253-135, 135 & below 
	//1008-519 ->489/4 --> 122.25 --> 519-641, 641-763, 763-885, 885 & above
	
	switch(Speed_State){
		case stop:
			if((joystick >= 885) || (joystick <= 135)){
				Speed_State = range100;
			}
			else if((joystick >= 763) || (joystick <= 253)){
				Speed_State = range250;	
			}
			else if((joystick >= 641) || (joystick <= 371)){
				Speed_State = range500;
			}
			else if((joystick >= 519) || (joystick <= 489)){
				Speed_State = range1000;
			}
			else{
				speed = 1000;
				Speed_State = stop;
			}
		case range1000:
			speed = 1000;
			Speed_State = stop;
			break;
		case range500:
			speed = 500;
			Speed_State = stop;
			break;
		case range250:
			speed = 250;
			Speed_State = stop;
			break;
		case range100:
			speed = 100;
			Speed_State = stop;
			break;
		default: Speed_State = stop; break;
	}		
	return Speed_State;
}

enum Display_States{matrix}Display_State;
int Display_Tick(int LED_State){
	switch(LED_State){
		case matrix:
			transmit_data(row, 2);
			transmit_data(pattern, 1);
			LED_State = matrix;
			break;
		default: LED_State = matrix;
	}
	return LED_State;
}

int main(void) {
    DDRD = 0xFF; PORTD = 0x00;
    DDRC = 0xFF; PORTC = 0x00;


    static task task1, task2, task3;
    task *tasks[] = {&task1, &task2, &task3};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

    const char start = -1;
	
    //SHIFT RIGHT LEFT
    task1.state = start;
    task1.period = 100;
    task1.elapsedTime = task1.period;
    task1.TickFct = &Shift_Tick;
	
    //DISPLAY
    task3.state = start;
    task3.period = 1; 
    task3.elapsedTime = task3.period;
    task3.TickFct = &Display_Tick;
	
    //SPEED
    task2.state = start;
    task2.period = 1;
    task2.elapsedTime = task2.period;
    task2.TickFct = &Speed_Tick;
	
    A2D_init();

    TimerSet(1);
    TimerOn();
    unsigned short i;
    
    while (1) {
	    joystick = ADC;
	    task1.period = speed;
	    
	    for(i=0; i<numTasks; i++){ //Scheduler code
			if(tasks[i]->elapsedTime == tasks[i]->period){
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
    }
    return 1;
}

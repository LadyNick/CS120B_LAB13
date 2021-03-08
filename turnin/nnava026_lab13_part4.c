
/*	Author: Nicole Navarro
 *  Partner(s) Name: 
 *	Lab Section:21
 *	Assignment: Lab #13  Exercise #4
 *	Video: https://youtu.be/iYBhmNdyxCU
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

//unsigned char update = 0;
unsigned short leftright;
unsigned short updown;
unsigned char pattern = 0x80;
unsigned char currow = 0;
unsigned char row[5] = {0x1E, 0x1D, 0x1B, 0x17, 0x0F}; 
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

// Pins on PORTA are used as input for A2D conversion
	//    The default channel is 0 (PA0)
	// The value of pinNum determines the pin on PORTA
	//    used for A2D conversion
	// Valid values range between 0 and 7, where the value
	//    represents the desired pin for A2D conversion
void Set_A2D_Pin(unsigned char pinNum) {
ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
// Allow channel to stabilize
static unsigned char i = 0;
for ( i=0; i<15; i++ ) { asm("nop"); } 
}


enum Shift_States{wait, shift}Shift_State;
int Shift_Tick(int Shift_State){
	switch(Shift_State){
		case wait:
			if((leftright > 498) && (leftright < 519) && (updown > 498) && (updown < 519)){
				Shift_State = wait; //there is no movement
			}
			else{
				Shift_State = shift;
			}
			break;
		case shift:
			if(leftright <= 498){//its going to the left
				if(pattern == 0x80){
					//edge
				}
				else{
					pattern = pattern << 1;
				}
			}
			if(leftright >= 519){//its going to the right
				if(pattern == 0x01){
					//edge
				}
				else{
					pattern = pattern >> 1;
				}
			}
			if(updown >= 498){//its going up
				if(currow == 0){
					//edge
				}
				else{
					--currow;
				}
			}
			if(updown <= 489){//its going down
				if(currow == 4){
					//edge
				}
				else{
					++currow;
				}
			}	
		default: Shift_State = wait; break;
	}
	return Shift_State;
}

enum Speed_States{range}Speed_State;
int Speed_Tick(int Speed_State){
	//here are the different sectors since i use the norm +- 15 just as an offset meaning for moving left and righ the mins are
	//504 +- 15 --> 489, 519 
	//max for right: 1008
	//max for left: 15
	//489-15 -> 474 /4 --> 118.5 --> 489-371, 371-253, 253-135, 135 & below 
	//1008-519 ->489/4 --> 122.25 --> 519-641, 641-763, 763-885, 885 & above
	
	/*switch(Speed_State){
		case range: //519
			if(joystick >= 519){
				if(joystick >= 885){
					speed = 100;
				}
				else if(joystick >= 763){
					speed = 250;
				}
				else if(joystick >= 641){
					speed = 500;
				}
				else{
					speed = 1000;
				}
			}//489
			else if(joystick < 489){
				if(joystick <= 135){
					speed = 100;
				}
				else if(joystick <= 253){
					speed = 250;
				}
				else if(joystick <= 371){
					speed = 500;
				}
				else{
					speed = 1000;
				}
			}
			Speed_State = range; break;
		default: Speed_State = range; break; 
	} 		*/
	return Speed_State;
}

enum Display_States{matrix}Display_State;
int Display_Tick(int LED_State){
	switch(LED_State){
			
		case matrix:
//			if(update == currow){
				transmit_data(row[currow],2);
				transmit_data(pattern, 1);
//			}
//			else{
//				transmit_data(row[update],2);
//				transmit_data(0,1);
//			}
//			++update;
//			if(update > 4){
//				update = 0;
//			}
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
	    Set_A2D_Pin(0);
	    leftright = ADC;
	    Set_A2D_Pin(1);
	    updown = ADC;
	    //task1.period = speed;
	    
	    for(i=0; i<numTasks; i++){ //Scheduler code
			if(tasks[i]->elapsedTime >= tasks[i]->period){
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

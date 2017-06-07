/*	Carlos Santillana
 *	Lab Section: 23
 *	Assignment: Lab Final
 *	Exercise Description: [optional - include for your own benefit]
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include "keypad.h"
#include "io.c"
#include "usart.h"
#include <avr/pgmspace.h>

//Global Variables
short temperature;
unsigned short scales[8] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};
unsigned char soundCounter;
unsigned short timer;
unsigned char setAlarm;
unsigned char irInput;
unsigned short alarmCount;
unsigned short alarmEnd;
unsigned short alarmIncrementer;
unsigned short minutes;
unsigned short hours;
unsigned char bluetoothInput;	
unsigned char initializeAlarm;
unsigned char displayAlarm;

const unsigned char celToFarArray[100] PROGMEM = {32, 33, 35, 37, 39, 41, 42, 44, 46, 48, 50, 51, 53, 55, 57, 59, 60, 62, 64, 66, 68, 69, 71, 73,
	 75, 77, 78, 80, 82, 84, 86, 87, 89, 91, 93, 95, 96, 98, 100, 102, 104, 105, 107, 109, 111, 113, 114, 116, 118, 120, 122, 123, 125,
	  127, 129, 131, 132, 134, 136, 138, 140, 141, 143, 145, 147, 149, 150, 152, 154, 156, 158, 159, 161, 163, 165, 167, 168, 170, 172,
	  174, 176, 177, 179, 181, 183, 185, 186, 188, 190, 192, 194, 195, 197, 199, 201, 203, 204, 206, 208, 210};

//Function Prototypes
inline unsigned char celToFar(unsigned char C);
unsigned long int findGCD(unsigned long int a, unsigned long int b);
void ADC_init();
void set_PWM_speaker(double frequency);
void PWM_on_speaker() ;
void PWM_off_speaker();

// --------Start of User defined FSMs-----------------------------------------------

//IR Receiver
int SMTick1(int state) {
	irInput = PIND & 0x40;//used to turn off alarm
	return state;
	
}
// Temperature sensor
int SMTick2(int state) {
	unsigned short voltage = ADC;// read from sensor
	unsigned short tmpVoltage = voltage;
	voltage =  (tmpVoltage >> 2) + (tmpVoltage << 2);// first divide by 4 to get .25 * volt then add that value by 4 * volt
	temperature = (voltage - 500) / 10;//turn signal to C
	temperature = celToFar(temperature);
	return state;
}

//LED Matrix
enum SM3_States {SM3_init, SM3_read, SM3_send_temp/*, SM3_set_alarm*/, SM3_set_hour, SM3_set_minute, SM3_start_alarm, SM3_wake_up};
int SMTick3(int state) {
	static unsigned char first;
	static unsigned char minuteEntered;//delay between hours inputted shows up and minutes key word shows up
	static unsigned char hourEntered; 

	//------------Transition State Machine-----------------------------------
	switch(state){
		case(SM3_init):
		if (first == 0){//to make sure init executes once
			state = SM3_init;
		}
		else
			state = SM3_read;
		break;
		
		case(SM3_read):
		if ((bluetoothInput == 0x54 || bluetoothInput == 0x74) && !displayAlarm){// if inputted 't'
			state = SM3_send_temp;
		}
		else if ((bluetoothInput == 0x41 || bluetoothInput == 0x61) && !displayAlarm){//if inputted 'a'
			state = SM3_set_hour;
			bluetoothInput = 0;
			hours=0;
			minutes =0;
		}
		else if (displayAlarm >= 1){
			state = SM3_wake_up;
		}
		else
			state = SM3_read;
		break;
		
		case(SM3_send_temp):
			state = SM3_read;
		break;
		
// 		case(SM3_set_alarm)://outputs 'hour'
// 		state = SM3_set_hour;
// 		bluetoothInput = 0;
// 		hours=0;
// 		minutes =0;
// 		break;
// 		
		case(SM3_set_hour)://gets hours
		if (bluetoothInput != 0x0A && bluetoothInput != 0x45 && bluetoothInput != 0x65 && !displayAlarm){//while not carriage return and not e
			state = SM3_set_hour;
		}
		else if ((bluetoothInput == 0x45 || bluetoothInput == 0x65) && !displayAlarm){//if e entered exit
			state = SM3_read;
			hours = 0;
			bluetoothInput = 0;
			first = 0;
			hourEntered = 0;//newly added
		}
		else if (displayAlarm){
			state = SM3_wake_up;
		}
		else{
			state = SM3_set_minute;
			first = 0;
			bluetoothInput = 0;
			minutes =0;
			hourEntered = 0;//newly added
		}
		break; 
		
		case(SM3_set_minute)://gets minutes
		if (bluetoothInput != 0x0A && bluetoothInput != 0x45 && bluetoothInput != 0x65 && !displayAlarm){//while not carriage return and not e
			state = SM3_set_minute;
		}
		else if ((bluetoothInput == 0x45 || bluetoothInput == 0x65) && !displayAlarm){//if e entered exit
			state = SM3_read;
			bluetoothInput = 0;
			minutes =0;
			minuteEntered = 0;
		}
		else if (displayAlarm){
			state = SM3_wake_up;
			}
		else
		state = SM3_start_alarm;
		bluetoothInput = 0;
		first = 0;
		minuteEntered = 0;
		break;
		
		case(SM3_start_alarm)://starts alarm
 		if (displayAlarm){
			state = SM3_wake_up;
		}
		else{
			state = SM3_read;
			bluetoothInput = 0;
		}
		break;
		
		case(SM3_wake_up):
		if (irInput != 0 && displayAlarm){
			state = SM3_init;
			initializeAlarm = 0;
			displayAlarm = 0;
			PORTC = 0x80;//newly added
		}
		else
			state = SM3_wake_up;
		break;
		
		default:
		/*state = SM3_init;*/
		break;
	}
	//------------Action State Machine-----------------------------------
	
	switch(state){
		case(SM3_init)://outputs temperature and initializes variables to zero
		PORTC = 0x80;
		if(USART_IsSendReady()){
			USART_Send(temperature);//change back to temp later
		}

		initializeAlarm = 0;
		hours =10;
		bluetoothInput =0;
		minutes=40;
		alarmCount=0;
		alarmEnd=0;
		minuteEntered=0;
		hourEntered = 0;
		break;
		
		case(SM3_read):// reads input for temp or alarm
		if(USART_HasReceived()){
			bluetoothInput = USART_Receive();
		}
		PORTC = 0x80;
		if(USART_IsSendReady()){
			USART_Send(temperature);//change back to temp later
		}
		break;
		
		case(SM3_send_temp)://sends temp
		PORTC = 0x80;
		if(USART_IsSendReady()){
			USART_Send(temperature);
		}
		break;
		
// 		case(SM3_set_alarm)://send message telling to set hour on led matrix
// 		PORTC &= 0x7F;// changes from temperature to alarm input
// 		if (USART_IsSendReady()){
// 			USART_Send(200);// 200 == code for hours
// 		}
// 		break;
		
		case(SM3_set_hour)://input hour until carriage return or e
		PORTC &= 0x7F;// changes from temperature to alarm input
		if (hourEntered == 0){
			if (USART_IsSendReady()){
				USART_Send(200);// 200 == code for minute input
			}
		}
		if(USART_HasReceived()){
			bluetoothInput = USART_Receive();//does work
			if (bluetoothInput >= 0x30 && bluetoothInput <= 0x39){//changed to hex
			hours += (bluetoothInput - 0x30);//changed to hex
			hourEntered = 1;
			}
		}
		if (bluetoothInput == 0x0A || bluetoothInput == 0x65){// checks for carrage return and e
			if (USART_IsSendReady()){
				USART_Send(hours);
			}
		}
		if (hours >=16){
			if (USART_IsSendReady()){
				USART_Send(220);// 220 == code for error
			}
			hours = 0;
			hourEntered = 0;
		}
		break;
		
		case(SM3_set_minute)://input minute until carriage return or e
		if (minuteEntered == 0){
			if (USART_IsSendReady()){
				USART_Send(201);// 201 == code for minute input
			}
		}
		if(USART_HasReceived()){
			bluetoothInput = USART_Receive();
			if (bluetoothInput >= 0x30 && bluetoothInput <= 0x39){//changed to hex
				minutes += (bluetoothInput - 0x30);
				minuteEntered = 1;
			}
		}
		if (bluetoothInput == 0x0A || bluetoothInput == 0x65){
			if (USART_IsSendReady()){
				USART_Send(minutes);
			}
		}
		if (minutes >= 60){
			if (USART_IsSendReady()){
				USART_Send(220);// 220 == code for error
			}
			minutes = 0;
			minuteEntered = 0;//restarts minute input
		}
		break;
		
		case(SM3_start_alarm)://set alarm end
			alarmCount =0;
			alarmEnd = (hours * 60 + minutes) * 60;//sets alarm end to minutes
			initializeAlarm = 1;
		break;
		
		case(SM3_wake_up):
		if (USART_IsSendReady()){
			PORTC &= 0x7F;// changes from temperature to alarm input newly added
			USART_Send(150);// 150 == code for wake up
		}
		break;
		
		default:
		PORTC = 0x80;
		if(USART_IsSendReady()){
			USART_Send(temperature);//change back to temp later
		}
		break;
	}
	first++;
	return state;
}



// Alarm ring
int SMTick4(int state) {//
	if (initializeAlarm == 1){
		if (alarmCount >= alarmEnd){
			PORTC &= 0x7F;// changes from temperature to alarm input newly added
			if(USART_IsSendReady()){
				USART_Send(150);
				initializeAlarm = 0;
				displayAlarm = 1;
				}
		}
	}
	if ((alarmCount > (alarmEnd + 60)) && displayAlarm == 1){//newly added
		set_PWM_speaker(scales[1]);
	}
	else{
		set_PWM_speaker(0);
	}
	alarmCount++;
	return state;
}
// --------END User defined FSMs-----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;
// -------- Start of main -----------------------------------------------
int main()
{
// Set Data Direction Registers
// Buttons PORTA[0-7], set AVR PORTA to pull down logic
//0 == input; 1 == output
DDRA = 0x00; PORTA = 0xFF;
DDRB = 0xFF; PORTB = 0x00;
DDRC = 0xFF; PORTC = 0x00;
DDRD = 0xBF; PORTD = 0x40;

// Period for the tasks
unsigned long int SMTick1_calc = 10;
unsigned long int SMTick2_calc = 10;
unsigned long int SMTick3_calc = 100;
unsigned long int SMTick4_calc = 1000;

//Calculating GCD
unsigned long int tmpGCD = 1;
tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
tmpGCD = findGCD(tmpGCD, SMTick3_calc);
tmpGCD = findGCD(tmpGCD, SMTick4_calc);

//Greatest common divisor for all tasks or smallest time unit for tasks.
unsigned long int GCD = tmpGCD;

//Recalculate GCD periods for scheduler
unsigned long int SMTick1_period = SMTick1_calc/GCD;
unsigned long int SMTick2_period = SMTick2_calc/GCD;
unsigned long int SMTick3_period = SMTick3_calc/GCD;
unsigned long int SMTick4_period = SMTick4_calc/GCD;

//Declare an array of tasks 
static task task1, task2, task3, task4;
task *tasks[] = { &task1, &task2, &task3, &task4};
const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

// Task 1
task1.state = -1;//Task initial state.
task1.period = SMTick1_period;//Task Period.
task1.elapsedTime = SMTick1_period;//Task current elapsed time.
task1.TickFct = &SMTick1;//Function pointer for the tick.

// Task 2
task2.state = -1;//Task initial state.
task2.period = SMTick2_period;//Task Period.
task2.elapsedTime = SMTick2_period;//Task current elapsed time.
task2.TickFct = &SMTick2;//Function pointer for the tick.

// Task 3
task3.state = SM3_init;//Task initial state.
task3.period = SMTick3_period;//Task Period.
task3.elapsedTime = SMTick3_period;//Task current elapsed time.
task3.TickFct = &SMTick3;//Function pointer for the tick.

// Task 4
task4.state = -1;//Task initial state.
task4.period = SMTick4_period;//Task Period.
task4.elapsedTime = SMTick4_period;//Task current elapsed time.
task4.TickFct = &SMTick4;//Function pointer for the tick.


//Initializing functions
initUSART();
ADC_init();
PWM_on_speaker();
set_PWM_speaker(0);
TimerSet(GCD);
TimerOn();

//Initializing variables
temperature =0;
alarmCount=0;
alarmEnd = 20000;
timer =0;
unsigned short i; // Scheduler for-loop iterator

while(1) {
	// Scheduler code
	for ( i = 0; i < numTasks; i++ ) {
		// Task is ready to tick
		if ( tasks[i]->elapsedTime == tasks[i]->period ) {
			// Setting next state for task
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			// Reset the elapsed time for next tick.
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += 1;
	}

	while(!TimerFlag);
	TimerFlag = 0;
}

// Error: Program should not exit!
return 0;
}
//----------------------------- Function calls --------------------------------------------

//Celsius to Fahrenheit look up table call
inline unsigned char celToFar(unsigned char C){
	return pgm_read_word(&celToFarArray[C]);
}
//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
b = c;
	}
	return 0;
}


//Initializes the ADC
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM_speaker(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR0B &= 0x08; } //stops timer/counter
		else { TCCR0B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR0A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR0A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR0A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT0 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on_speaker() {
	TCCR0A = (1 << COM0A0 | 1 << WGM00) ;
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM_speaker(0);
}

void PWM_off_speaker() {
	TCCR0A = 0x00;
	TCCR0B = 0x00;
}

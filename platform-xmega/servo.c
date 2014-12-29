/*
 * Copyright (c) 2014, Manuel Vetterli
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "eeprom.h"
#include "servo.h"
#include "port.h"
#include "sys/process.h"

#if defined(__AVR__)
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#if defined(__18CXX) || defined(__AVR__)
#define DBG(...)
#else
#define DBG(...)        printf(__VA_ARGS__)
#endif

PROCESS(servo_process,"Servo Process");

unsigned int calc_servo_next_val(unsigned char nr);

volatile uint8_t servo_status = 0;
volatile uint16_t servo_timer = 0;
volatile uint8_t servo_timer2 = 0;
uint8_t servo_delay = 3;
uint8_t servo_start_method = 1;

volatile enum _ServoIsrState
{
	SI_IDLE = 0,
	SI_TURNON_1,
	SI_TURNON_WAIT_1,
	SI_TURNON_2,
	SI_TURNON_WAIT_2,
	SI_TURNON_3,
	SI_RUNNING
} ServoIsrState = SI_IDLE;

//---------------------------------------------------------------------
// Timing Definitions:
// (all values given in us)

#define TICK_PERIOD       20000L    // 20ms tick for Timing Engine
                                    // => possible values for timings up to
                                    //    5.1s (=255/0.020)
                                    // note: this is also used as frame for
                                    // Servo-Outputs (OCR1A and OCR1B)

t_servo servo[SERVO_COUNT];


//---------------------------------------------------------------------------------
// calc_servo_next_val(unsigned char nr)
//      generates the actual setting of OCR out of actual position.
//      does the following:
//      a) makes a linear interpolation on the loaded curve
//      b) check for total limits recalcs the interpolated value according to these
//         limits.
//      c) transforms the point to real servo position
//
//      This routine is to be called every timeslot (20ms).
//      
// Parameters:
//      nr: this servo is calculated
//      
// Return:
//      timer value (OCR1) of this servo; if retval = 0, movement is finished
//
// Scaling:
//      input positions (from curve table) are 8 bit;
//      intermediate interpolations are scaled up to 32 bits; headroom for signed types is reserved.
//      this scaling is neccesary to achieve a good resolution when moving;
//
uint16_t servo_transform_position(uint8_t num, uint8_t position)
{
	int16_t posi;
	int32_t posl;
	int16_t delta_pos;

    posl = (int32_t)position * 128;
	
	// now correct for 255 as full scale of curve

    posl = posl * 256L;
    posl = posl / 255L;                                                 // range 0 ... 32768 (2^15)

    // scale this position according to min/max
    // scaled = min + interpolated * (max-min)

    delta_pos = (int16_t)(servo[num].max / 4) - (int16_t)(servo[num].min / 4);    // range -16384 ... 16384 (2^14)

    posl = posl * delta_pos;                                            // range -2^29 ... 2^29

    posl = posl + (servo[num].min * (8192L));                            // gain: 13 Bits => range 2^29

    posl = posl / (16384L);                                              

    // now scale to timer

    posi = (int16_t) posl;                                  // range 0 ... 32767
    posl = (int32_t) posi * servo[num].pulse_delta;          // range 0 ... 32767000
    posi = posl >> 15;                                      // range 0 ... 1000  
    posi = posi + servo[num].pulse_min;                      //      + SERVO_MIN;  // add 1ms
	
	return posi;
}
uint16_t servo_calc_next_val(uint8_t num)
{
	int16_t posi;
	int16_t dt, delta_t;
	int32_t posl;
	int16_t delta_pos;
	
	if ((servo[num].active_time==0) && (servo[num].position_setpoint!=servo[num].position_start))
	{
		servo[num].status |= (1<<ST_BIT_MOVING);
		servo[num].active_time = (servo[num].time_delta) * servo[num].time_ratio;
	}
	
	// Return last known position if idle, or power disabled
	if ((servo[num].active_time == 0) || !(servo_status&(1<<SERVO_STATUS_PWR)))
	{	
		return servo_transform_position(num, servo[num].position_start);
	}
	
	servo[num].active_time--;
	
	// now calc linear interpolation
	// pos = pos_prev + (dt / delta_t) * (pos - pos_prev)

    delta_t = (int16_t)(servo[num].time_delta)*servo[num].time_ratio;

	// Avoid division by 0
	if (delta_t == 0)
		delta_t = 1;

	dt = delta_t - servo[num].active_time;

    delta_pos = ((int16_t)servo[num].position_setpoint
				-(int16_t)servo[num].position_start)
                * 128L;													// int (with gain 128)
                                                                        // range -32640 ... 32640
    posl = (int32_t)delta_pos * dt;
        
    posl = posl / delta_t;                                              // interpolated value
    posl = posl + (int16_t)servo[num].position_start * 128L;      // +offset (gain 128)
                                                                        // range 0 ... 32640
																		
	servo[num].position_actual = (uint8_t) (posl/128L);
    // now correct for 255 as full scale of curve

    posl = posl * 256L;
    posl = posl / 255L;                                                 // range 0 ... 32768 (2^15)

    // scale this position according to min/max
    // scaled = min + interpolated * (max-min)

    delta_pos = (int16_t)(servo[num].max / 4) - (int16_t)(servo[num].min / 4);    // range -16384 ... 16384 (2^14)

    posl = posl * delta_pos;                                            // range -2^29 ... 2^29

    posl = posl + (servo[num].min * (8192L));                            // gain: 13 Bits => range 2^29

    posl = posl / (16384L);                                              

    // now scale to timer

    posi = (int16_t) posl;                                  // range 0 ... 32767
    posl = (int32_t) posi * servo[num].pulse_delta;          // range 0 ... 32767000
    posi = posl >> 15;                                      // range 0 ... 1000  
    posi = posi + servo[num].pulse_min;                      //      + SERVO_MIN;  // add 1ms

	// Decide follow-up action
	if (servo[num].active_time==0)
	{
		servo[num].status &= ~(1<<ST_BIT_MOVING);
		servo[num].position_start = servo[num].position_setpoint;
	}
	
	return posi;
}

void servo_init(void)
{
	uint8_t index;
	// Startup sequence
	// Load settings
	servo_mode_update();
	
	for (index=0;index<2;index++)
	{
		servo[index].pulse_min = SERVO_MIN;
		servo[index].pulse_delta = SERVO_DELTA;
		servo[index].min = eeprom.servo_min[index];
		servo[index].max = eeprom.servo_max[index];
		servo[index].time_ratio = eeprom.servo_time_ratio[index];
		servo[index].time_delta = 4;
		servo[index].position_setpoint = eeprom_status.servo_position[index];
		servo[index].position_start = eeprom_status.servo_position[index];
		servo[index].position_actual = eeprom_status.servo_position[index];
		servo[index].active_time = 0;
	}
	
	// Calculate the first compare value before configuring the compare units
	servo[0].pulse_value = servo_calc_next_val(0);
	servo[1].pulse_value = servo_calc_next_val(1);
	
	// Init code for ATxmega
	#define PERVAL   (F_CPU / 1000000L * TICK_PERIOD / T1_PRESCALER)
	
	// Event channel 0: 2 MHz clock
	EVSYS.CH2MUX = EVSYS_CHMUX_PRESCALER_16_gc;
	
	// Initial conditions of timer
	TCC0.CNT = 0;
	TCC0.PER = PERVAL;
	TCC0.CCA = PERVAL;
	TCC0.CCB = PERVAL;
	TCC0.CCC = PERVAL;
	TCC0.CCD = PERVAL;
	
	// Mode: Single Slope PWM
	TCC0.CTRLB = TC_WGMODE_SINGLESLOPE_gc;
	// Clock source: Event channel 0
	TCC0.CTRLA = TC_CLKSEL_EVCH2_gc;
	// Clear pending interrupts
	TCC0.INTFLAGS = TC0_OVFIF_bm;
	// Enable low priority interrupt
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	
	//Note: Compare units can be enabled by the following statements
	//TCC0.CTRLB |= TC0_CCAEN_bm; etc
	
	PORTC.OUTSET = (1<<S12_PWM)|(1<<S22_PWM);
	PORTC.DIRSET = (1<<S12_PWM)|(1<<S22_PWM);
	
	TCC0.CTRLB = (1<<(S12_PWM+4)) | (1<<(S22_PWM+4)) | TC_WGMODE_SINGLESLOPE_gc;
	
	// Startup delay
	servo_timer = eeprom.servo_startup_delay;
	
	// Now start process
	process_start(&servo_process, NULL);
}

PROCESS_THREAD(servo_process, ev, data)
{
	uint8_t index;
	PROCESS_BEGIN();

	// Wait until the startup delay is over
	do {
		PROCESS_PAUSE();
	} while (servo_timer!=0);
		
	// Enable Power (for 1s)
	servo_status |= (1<<SERVO_STATUS_PWR_TEST);
	servo_timer = 50;
	
	do {
		PROCESS_PAUSE();
	} while (servo_timer!=0);
		
	while (1)
	{
		if (eeprom.servo_timeout==0)
			servo_status |= (1<<SERVO_STATUS_PWR_ALWAYS_ON);
		else
			servo_status &= ~(1<<SERVO_STATUS_PWR_ALWAYS_ON);
					
		// Copy status to command register
		for (index=0;index<2;index++)
		{
			if (servo[index].status&(1<<ST_BIT_MOVING))
			{
				servo[index].command |= (1<<SC_BIT_MOVING);
							
				// Write flags to eeprom after ~5s if the user has power always on
				// or after the timeout occurs
				if (eeprom.servo_timeout==0)
					servo_timer = 0xFF;
				else
					servo_timer = eeprom.servo_timeout;
			}
			else
			{
				// Servo is not moving
				servo[index].command &= ~(1<<SC_BIT_MOVING);
			}
		}
					
		if (servo_status&(1<<SERVO_STATUS_INT_FLAG))
		{
			servo_status &= ~(1<<SERVO_STATUS_INT_FLAG);
			servo[0].pulse_value = servo_calc_next_val(0);
			servo[1].pulse_value = servo_calc_next_val(1);
						
			if (servo_timer==0)
			{
				if (!(servo_status&(1<<SERVO_STATUS_PWR_ALWAYS_ON)) && !(servo[0].status&(1<<ST_BIT_MOVING)) && !(servo[1].status&(1<<ST_BIT_MOVING)))
					servo_status &= ~((1<<SERVO_STATUS_PWR)|(1<<SERVO_STATUS_PWR_TEST));
							
				eeprom_status.servo_position[0] = servo[0].position_setpoint;
				eeprom_status.servo_position[1] = servo[1].position_setpoint;
				eeprom_sync_status();
			}
		}
		
		PROCESS_PAUSE();
	}
	
	PROCESS_END();
}

typedef enum
{
	SERVO_SM_INIT = 0u,
	SERVO_SM_STARTUP_DELAY,
	SERVO_SM_RUN,
	SERVO_SM_TIMESTEP
	

} SERVO_SM;

#if defined (__AVR_XMEGA__)
ISR(TCC0_OVF_vect)
#elif defined(__AVR__)
ISR(TIMER1_OVF_vect)
#else
void servo_isr(void)
#endif
{
	static uint8_t servo_isr_timer = 0;

#if defined(__18CXX)
	// Stop Timer
    T3CONbits.TMR3ON = 0;
#endif
	
	servo_status |= (1<<SERVO_STATUS_INT_FLAG)|(1<<SERVO_STATUS_TICK_FLAG);
	
	if (servo_isr_timer)
		servo_isr_timer--;
		
	if (servo_timer)
		servo_timer--;

	switch (ServoIsrState)
	{
		case SI_IDLE:
			// Check if we have to turn on power to the servos
			if (!(servo_status&(1<<SERVO_STATUS_PWR)) && (
					(servo_status&(1<<SERVO_STATUS_PWR_ALWAYS_ON))
				||	(servo_status&(1<<SERVO_STATUS_PWR_TEST))
				||	(servo[0].status&(1<<ST_BIT_MOVING))
				||	(servo[1].status&(1<<ST_BIT_MOVING))
			))
			{
				ServoIsrState = SI_TURNON_1;
			}
		break;
		case SI_TURNON_1:
		
			// Reset Compare Units
#if defined(__18CXX)
		    CCP4CON = 0;
		    CCP3CON = 0;
#endif
			switch (servo_start_method)
			{
				default:
				case 0:
					servo_isr_timer = 0;
					break;
				case 1:
				case 2:
#if defined(__18CXX)
					LATDbits.LATD0 = 1;
					LATDbits.LATD2 = 1;
#elif defined (__AVR_XMEGA__)
					TCC0.CCA = PERVAL;
					TCC0.CCB = PERVAL;
					TCC0.CCC = PERVAL;
					TCC0.CCD = PERVAL;
#elif defined(__AVR__)
					OCR1A = TOPVAL;
					OCR1B = TOPVAL;
#endif
					servo_isr_timer = servo_delay;
					break;
				case 3:
#if defined(__18CXX)
					LATDbits.LATD0 = 0;
					LATDbits.LATD2 = 0;
#elif defined (__AVR_XMEGA__)
					TCC0.CCA = 0;
					TCC0.CCB = 0;
					TCC0.CCC = 0;
					TCC0.CCD = 0;
#elif defined(__AVR__)
					OCR1A = 0;
					OCR1B = 0;
#endif
					servo_isr_timer = servo_delay;
					break;
			}
			ServoIsrState = SI_TURNON_WAIT_1;

		case SI_TURNON_WAIT_1:
			if (servo_isr_timer)
			{
				break;
			}
			ServoIsrState = SI_TURNON_2;

		case SI_TURNON_2:
#if defined(__18CXX)
			TRISEbits.TRISE1 = 0;
#elif defined(__AVR_XMEGA__)
			servo_power_enable();
#elif defined(__AVR__)
			PORTB |= (1<<PB0);
#endif
			servo_isr_timer = servo_delay;
			ServoIsrState = SI_TURNON_WAIT_2;
		
		case SI_TURNON_WAIT_2:
			if (servo_isr_timer)
			{
				break;
			}
			ServoIsrState = SI_TURNON_3;
		case SI_TURNON_3:
			switch (servo_start_method)
			{
				case 2:
#if defined(__18CXX)
					LATDbits.LATD0 = 0;
					LATDbits.LATD2 = 0;

					// Reset Timer
				    TMR3H = 0;
				    TMR3L = 0;

				    // Start Timer
				    T3CONbits.TMR3ON = 1;
					
					while (TMR3L<128);
#endif
					break;
				default:
					break;
			}

			servo_status |= (1<<SERVO_STATUS_PWR);

			ServoIsrState = SI_RUNNING;
		case SI_RUNNING:

		    if (!(servo_status & ((1<<SERVO_STATUS_PWR)|(1<<SERVO_STATUS_PWR_ALWAYS_ON)))) {
				ServoIsrState = SI_IDLE;
#if defined(__18CXX)
		        TRISEbits.TRISE1 = 1;
				
				// Reset Timer
				TMR3H = 0;
				TMR3L = 0;
				
				// Start Timer
				T3CONbits.TMR3ON = 1;
#elif defined(__AVR_XMEGA__)
				TCC0.CCA = PERVAL;
				TCC0.CCB = PERVAL;
				TCC0.CCC = PERVAL;
				TCC0.CCD = PERVAL;
				
				if (!(servo_status&(1<<SERVO_STATUS_PWM_DISABLE)))
				{
					servo_power_disable();
				}
#elif defined(__AVR__)
				OCR1A = TOPVAL;
				OCR1B = TOPVAL;
				
				if (!(servo_status&(1<<SERVO_STATUS_PWM_DISABLE)))
				{
					PORTB &= ~(1<<PB0);
				}
#endif				
		        return;
		    }

#if defined(__18CXX)
			// Reset Compare Units
		    CCP4CON = 0;
		    CCP3CON = 0;
			// Set on init, clear on compare match
		    CCP4CON = (1 << 3) | (1 << 0);
		    CCP3CON = (1 << 3) | (1 << 0);
		
		    // Reset Timer
		    TMR3H = 0;
		    TMR3L = 0;
		
		    // Start Timer
		    T3CONbits.TMR3ON = 1;
		
		    CCPR4 = servo[0].pulse_value;
		    CCPR3 = servo[1].pulse_value;
#elif defined(__AVR_XMEGA__)
			TCC0.CCABUF = servo[1].pulse_value;
			TCC0.CCBBUF = servo[0].pulse_value;
			TCC0.CCCBUF = servo[0].pulse_value;
			TCC0.CCDBUF = servo[1].pulse_value;
#elif defined(__AVR__)
			OCR1A = servo[0].pulse_value;
			OCR1B = servo[1].pulse_value;
#else
			DBG("ISR: 0: %u 1: %u\r\n", servo[0].pulse_value, servo[1].pulse_value);
#endif
			break;
	}

#if defined(__18CXX)	
	// Start Timer if not done yet
	T3CONbits.TMR3ON = 1;
#endif
}

void servo_update_configuration(void)
{
	servo[0].min = eeprom.servo_min[0];
	servo[1].min = eeprom.servo_min[1];
    
	servo[0].max = eeprom.servo_max[0];
	servo[1].max = eeprom.servo_max[1];
    
	servo[0].time_ratio = eeprom.servo_time_ratio[0];
	servo[1].time_ratio = eeprom.servo_time_ratio[1];
}

void servo_mode_update(void)
{	
	servo_start_method = eeprom.servo_start_method;
	
	if (servo_status&(1<<SERVO_STATUS_ENABLE_PWM_A))
		TCC0.CTRLB |= ((1<<(S12_PWM+4))|(1<<(S22_PWM+4)));
	else
		TCC0.CTRLB &= ~((1<<(S12_PWM+4))|(1<<(S22_PWM+4)));
	
	if (servo_status&(1<<SERVO_STATUS_ENABLE_PWM_B))
		TCC0.CTRLB |= ((1<<(S1_PWM+4))|(1<<(S2_PWM+4)));
	else
		TCC0.CTRLB &= ~((1<<(S1_PWM+4))|(1<<(S2_PWM+4)));
	
}


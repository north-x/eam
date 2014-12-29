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

#ifndef __SERVO_HP_H
#define __SERVO_HP_H

#define SERVO_COUNT          2         // number of Servos 

// Timing Definition

// TCC0, PORTC
// S1/S2 need to be inverted
// S12/S22 are direct feedthrough
#define S2_PWM				0
#define S1_PWM				1
#define S12_PWM				2
#define S22_PWM				3

// Timing Borders for Servo Pulse
#define SERVO_MINTIME      1000L    // 1ms
#define SERVO_INTERVAL     1000L    // 1ms

// defines for the Timer and the OCR
// note: only valid for "rounded" F_CPU values
#define T1_PRESCALER		16

#define SERVO_MIN          (F_CPU / 1000000L * SERVO_MINTIME / T1_PRESCALER)
#define SERVO_DELTA        (F_CPU / 1000000L * SERVO_INTERVAL / T1_PRESCALER)
#define SERVO_MAX          (F_CPU / 1000000L * (SERVO_MINTIME+SERVO_INTERVAL) / T1_PRESCALER)

// bit field for servo.status:
//#define ST_BIT_POSITION		0		// Last valid position: 0: A/L , 1: B/R
#define ST_BIT_MOVING    7          // 0=stopped, 1=moving
#define ST_BIT_AT_SETPOINT	6
//#define ST_BIT_TRANSMIT_STATUS	5
//#define ST_BIT_MODE_MULTIPOSITION	4

// bit field for servo.command
//#define SC_BIT_POSITION	0
#define SC_BIT_MOVING	7

#define SERVO_STATUS_PWR                7
#define SERVO_STATUS_PWR_ALWAYS_ON      6
#define SERVO_STATUS_PWR_TEST			5
#define SERVO_STATUS_INT_FLAG			4
#define SERVO_STATUS_PWM_DISABLE		3
#define SERVO_STATUS_TICK_FLAG			2
#define SERVO_STATUS_ENABLE_PWM_B		1
#define SERVO_STATUS_ENABLE_PWM_A		0

typedef struct SERVO_T
{ 
	uint16_t pulse_min;         // lowest pulse width [0..65535]
    uint16_t pulse_delta;       // range for pulse width [0..65535]
    uint16_t min;               // lower move limit [0..65535]
    uint16_t max;               // upper move limit [0..65535]
    
    uint8_t status;          // Bit 0: ACTUAL:   0=pre A, 1=pre B movement
                                    // Bit 1: MOVING:   0=at endpoint, 1=currently moving
                                    // Bit 5: OUT_CTRL: 0=no, 1=yes
                                    // Bit 6: REPEAT:   0=single, 1=forever
                                    // Bit 7: TERMINATE: flag: terminate after next B
	uint8_t command;								
      
    uint16_t active_time;       // runtime: relative time to start point
                                    // 0xFFFF   = restart Servos
                                    // 0   		= finished

    uint8_t time_ratio;       	// ratio between runtime and curve time
	
	uint8_t time_delta;
	uint8_t position_setpoint;
	uint8_t position_start;
	uint8_t position_actual;
	uint16_t pulse_value;
} t_servo;

void servo_update_configuration(void);
void servo_mode_update(void);

extern t_servo servo[SERVO_COUNT];
extern volatile uint8_t servo_status;
extern volatile uint16_t servo_timer;
extern volatile uint8_t servo_timer2;

void servo_isr(void);
void servo_init(void);

#endif


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

#ifndef PORT_H_
#define PORT_H_

#define PORT_PIN0	(PORTB.IN&(1<<2)) // P1
#define PORT_PIN1	(PORTC.IN&(1<<2)) // P2
#define PORT_PIN2	(PORTB.IN&(1<<3)) // P3
#define PORT_PIN3	(PORTC.IN&(1<<3)) // P4
#define PORT_PIN4	(PORTC.IN&(1<<0)) // P5
#define PORT_PIN5	(PORTC.IN&(1<<4)) // P6
#define PORT_PIN6	(PORTC.IN&(1<<1)) // P7
#define PORT_PIN7	(PORTC.IN&(1<<5)) // P8

#define PORT_PIN8	(PORTC.IN&(1<<6)) // P9
#define PORT_PIN9	(PORTD.IN&(1<<2)) // P10
#define PORT_PIN10	(PORTC.IN&(1<<7)) // P11
#define PORT_PIN11	(PORTD.IN&(1<<3)) // P12
#define PORT_PIN12	(PORTD.IN&(1<<0)) // P13
#define PORT_PIN13	(PORTD.IN&(1<<4)) // P14
#define PORT_PIN14	(PORTD.IN&(1<<1)) // P15
#define PORT_PIN15	(PORTD.IN&(1<<5)) // P16

#define PORT_PIN_STATUS(VAR)	do { \
	uint16_t temp16;\
	temp16 = PORT_PIN0?1:0; \
	temp16 |= PORT_PIN1?2:0; \
	temp16 |= PORT_PIN2?4:0; \
	temp16 |= PORT_PIN3?8:0; \
	temp16 |= PORT_PIN4?16:0; \
	temp16 |= PORT_PIN5?32:0; \
	temp16 |= PORT_PIN6?64:0; \
	temp16 |= PORT_PIN7?128:0; \
	temp16 |= PORT_PIN8?256:0; \
	temp16 |= PORT_PIN9?512:0; \
	temp16 |= PORT_PIN10?1024:0; \
	temp16 |= PORT_PIN11?2048:0; \
	temp16 |= PORT_PIN12?4096:0; \
	temp16 |= PORT_PIN13?8192:0; \
	temp16 |= PORT_PIN14?16384:0; \
	temp16 |= PORT_PIN15?32768:0; \
	VAR = temp16; \
} while (0);


#define PORT_FLAGS_PWM_TICK		0

#define PORT_MODE_PWM_ENABLE		0
#define PORT_MODE_PWM_CH7_ENABLE	1
#define PORT_MODE_PULLUP_ENABLE		2
#define PORT_MODE_RELAY_MONOSTABLE	3

#define PWM_PORT_COUNT	16

#define PWM_TIMER_PRESCALER	8
#define PWM_TICK_PERIOD     250L        // 250us tick for PWM Engine
#define PWM_STEPS           60L
#define PWM_PERIOD          (PWM_TICK_PERIOD * PWM_STEPS)    // 15ms = 250 * 60L
#define DIMM_RANGE_MIN		100    // aktiver Bereich 100-160
#define DIMM_RANGE_MAX (DIMM_RANGE_MIN+PWM_STEPS+1)

#define PWM_PERVAL   (F_CPU / 1000000L * PWM_TICK_PERIOD / PWM_TIMER_PRESCALER)

#define RELAY_CMD_LEFT1		1
#define RELAY_CMD_RIGHT1	2
#define RELAY_CMD_LEFT2		4
#define RELAY_CMD_RIGHT2	8
#define RELAY_CMD_LEFT		1
#define RELAY_CMD_RIGHT		2

typedef struct t_pwm_port
{
	uint8_t dimm_current;
	uint8_t dimm_target;
	uint8_t dimm_delta;
} t_pwm_port;

void port_init(void);
void port_update_configuration(void);
void port_di_init(void);

void pwm_init(void);
void pwm_tick(void);
void pwm_step(void);

extern uint16_t port_do_select;
extern uint16_t port_do;
extern uint16_t port_di;

extern uint8_t port_mode;

extern t_pwm_port pwm_port[PWM_PORT_COUNT];
extern uint8_t pwm_target[PWM_PORT_COUNT];
extern uint8_t pwm_delta[PWM_PORT_COUNT];
extern uint16_t pwm_update_trig;
extern uint16_t pwm_update_cont;
extern uint16_t pwm_at_setpoint;

extern uint8_t relay_cmd;
extern uint8_t relay_request;
extern uint8_t relay_state;

void relay_init(void);
void relay_governor(void);
void relay_process(void);

void servo_power_enable(void);
void servo_power_disable(void);

#endif /* PORT_H_ */
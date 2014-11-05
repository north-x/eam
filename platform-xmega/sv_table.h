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

/*
 * sv_table.h
 *
 */ 

#ifndef PARAMETER_TABLE_H_
#define PARAMETER_TABLE_H_

#include "wa2.h"
#include "eeprom.h"
#include "platform.h"
#include "ubasic.h"
#include "servo.h"
#include "port.h"
#include "usb/usb.h"
#include <avr/wdt.h>
#include <string.h>

void cmd_exec(void);
void dimm_target_parameter_update(void);
void dimm_delta_parameter_update(void);
void ln_update_threshold(void);

extern uint8_t ln_gpio_status;
extern uint8_t ln_gpio_status;
extern uint8_t ln_gpio_status_pre;
extern uint8_t ln_gpio_status_tx;
extern uint8_t ln_gpio_opcode_tx;
extern uint8_t ln_gpio_opcode_tx2;

uint8_t dimm_target_temp;
uint8_t dimm_delta_temp;
uint16_t dimm_parameter_select;
uint8_t cmd_register = 0;

extern uint8_t ubasic_script_status;

// Block mapping of TSE scripts starting at SV 112/170/240/304 (0x70/0xB0/0xF0/0x130)
SV_BLOCK_TABLE_BEGIN()
SV_BLOCK_MAP(0, MAX_PROGRAM_LEN, "UB Prog 1", ubasic_scripts[0].mem)
SV_BLOCK_MAP(1, MAX_PROGRAM_LEN, "UB Prog 2", ubasic_scripts[1].mem)
SV_BLOCK_MAP(2, MAX_PROGRAM_LEN, "UB Prog 3", ubasic_scripts[2].mem)
SV_BLOCK_MAP(3, MAX_PROGRAM_LEN, "UB Prog 4", ubasic_scripts[3].mem)
SV_BLOCK_TABLE_END();

SV_TABLE_BEGIN()
SV_CONST(1, "EEPROM Size", 0)
SV_CONST(2, "SW Version", SOFTWARE_VERSION)
SV_LSB(3, "Serial Number L", eeprom.sv_serial_number, 0)
SV_MSB(4, "Serial Number H", eeprom.sv_serial_number, 0)
SV(5, "Command Register", cmd_register, cmd_exec)
SV(6, "Config Register 1", eeprom.configA, wa2_update_configuration)
SV(7, "Config Register 2", eeprom.configB, wa2_update_configuration)
SV_CONST(8, "User Register 1", 8)
SV_CONST(9, "User Register 2", 9)
SV(10, "LN GPIO Status", ln_gpio_status, 0)
SV(11, "LN GPIO Status Transmit", ln_gpio_status_tx, 0)
SV(12, "LN Threshold Voltage x10", eeprom.ln_threshold, ln_update_threshold)
SV_LSB(13, "Port Digital Output Select", port_do_select, 0)
SV_MSB(14, "Port Digital Output Select", port_do_select, 0)
SV_LSB(15, "Port Digital Input Status", port_di, 0)
SV_MSB(16, "Port Digital Input Status", port_di, 0)
SV_LSB(17, "Port Digital Output Cmd", port_do, 0)
SV_MSB(18, "Port Digital Output Cmd", port_do, 0)
SV_LSB(19, "PWM Port Select L", dimm_parameter_select, 0)
SV_MSB(20, "PWM Port Select H", dimm_parameter_select, 0)
SV(21, "PWM Port Target", dimm_target_temp, dimm_target_parameter_update)
SV(22, "PWM Port Delta", dimm_delta_temp, dimm_delta_parameter_update)
SV_LSB(23, "Servo 1 Minimum L", eeprom.servo_min[0], servo_update_configuration)
SV_LSB(24, "Servo 1 Maximum L", eeprom.servo_max[0], servo_update_configuration)
SV_MSB(25, "Servo 2 Minimum H", eeprom.servo_min[1], servo_update_configuration)
SV_MSB(26, "Servo 2 Maximum H", eeprom.servo_max[1], servo_update_configuration)
SV(27, "Servo 2 Speed", eeprom.servo_time_ratio[1], servo_update_configuration)
SV_LSB(28, "Servo 2 Minimum L", eeprom.servo_min[1], servo_update_configuration)
SV_LSB(29, "Servo 2 Maximum L", eeprom.servo_max[1], servo_update_configuration)
SV_LSB(30, "Standby Delay L", eeprom.servo_timeout, 0)
SV_MSB(31, "Standby Delay H", eeprom.servo_timeout, 0)
SV_LSB(32, "Startup Delay L", eeprom.servo_startup_delay, 0)
SV_MSB(33, "Startup Delay H", eeprom.servo_startup_delay, 0)
SV(34, "Servo Start Method", eeprom.servo_start_method, servo_mode_update)
SV(35, "UBasic Status", ubasic_script_status, 0)
SV(36, "UBasic Autostart", eeprom.ubasic_autostart, 0)
SV_CONST(37, "TSE Time Ratio 1", 37)
SV_CONST(38, "TSE Time Ratio 2", 38)
SV_CONST(39, "TSE Time Ratio 3", 39)
SV_CONST(40, "TSE Time Ratio 4", 40)
SV(41, "LN GPIO 1 On Opcode 1", eeprom.ln_gpio_opcode[0][0], 0)
SV(42, "LN GPIO 1 On Opcode 2", eeprom.ln_gpio_opcode[0][1], 0)
SV(43, "LN GPIO 1 On Opcode 3", eeprom.ln_gpio_opcode[0][2], 0)
SV(44, "LN GPIO 1 Off Opcode 1", eeprom.ln_gpio_opcode[1][0], 0)
SV(45, "LN GPIO 1 Off Opcode 2", eeprom.ln_gpio_opcode[1][1], 0)
SV(46, "LN GPIO 1 Off Opcode 3", eeprom.ln_gpio_opcode[1][2], 0)
SV(47, "LN GPIO 2 On Opcode 1", eeprom.ln_gpio_opcode[2][0], 0)
SV(48, "LN GPIO 2 On Opcode 2", eeprom.ln_gpio_opcode[2][1], 0)
SV(49, "LN GPIO 2 On Opcode 3", eeprom.ln_gpio_opcode[2][2], 0)
SV(50, "LN GPIO 2 Off Opcode 1", eeprom.ln_gpio_opcode[3][0], 0)
SV(51, "LN GPIO 2 Off Opcode 2", eeprom.ln_gpio_opcode[3][1], 0)
SV(52, "LN GPIO 2 Off Opcode 3", eeprom.ln_gpio_opcode[3][2], 0)
SV(53, "LN GPIO 3 On Opcode 1", eeprom.ln_gpio_opcode[4][0], 0)
SV(54, "LN GPIO 3 On Opcode 2", eeprom.ln_gpio_opcode[4][1], 0)
SV(55, "LN GPIO 3 On Opcode 3", eeprom.ln_gpio_opcode[4][2], 0)
SV(56, "LN GPIO 3 Off Opcode 1", eeprom.ln_gpio_opcode[5][0], 0)
SV(57, "LN GPIO 3 Off Opcode 2", eeprom.ln_gpio_opcode[5][1], 0)
SV(58, "LN GPIO 3 Off Opcode 3", eeprom.ln_gpio_opcode[5][2], 0)
SV(59, "LN GPIO 4 On Opcode 1", eeprom.ln_gpio_opcode[6][0], 0)
SV(60, "LN GPIO 4 On Opcode 2", eeprom.ln_gpio_opcode[6][1], 0)
SV(61, "LN GPIO 4 On Opcode 3", eeprom.ln_gpio_opcode[6][2], 0)
SV(62, "LN GPIO 4 Off Opcode 1", eeprom.ln_gpio_opcode[7][0], 0)
SV(63, "LN GPIO 4 Off Opcode 2", eeprom.ln_gpio_opcode[7][1], 0)
SV(64, "LN GPIO 4 Off Opcode 3", eeprom.ln_gpio_opcode[7][2], 0)
SV(65, "LN GPIO 5 On Opcode 1", eeprom.ln_gpio_opcode[8][0], 0)
SV(66, "LN GPIO 5 On Opcode 2", eeprom.ln_gpio_opcode[8][1], 0)
SV(67, "LN GPIO 5 On Opcode 3", eeprom.ln_gpio_opcode[8][2], 0)
SV(68, "LN GPIO 5 Off Opcode 1", eeprom.ln_gpio_opcode[9][0], 0)
SV(69, "LN GPIO 5 Off Opcode 2", eeprom.ln_gpio_opcode[9][1], 0)
SV(70, "LN GPIO 5 Off Opcode 3", eeprom.ln_gpio_opcode[9][2], 0)
SV(71, "LN GPIO 6 On Opcode 1", eeprom.ln_gpio_opcode[10][0], 0)
SV(72, "LN GPIO 6 On Opcode 2", eeprom.ln_gpio_opcode[10][1], 0)
SV(73, "LN GPIO 6 On Opcode 3", eeprom.ln_gpio_opcode[10][2], 0)
SV(74, "LN GPIO 6 Off Opcode 1", eeprom.ln_gpio_opcode[11][0], 0)
SV(75, "LN GPIO 6 Off Opcode 2", eeprom.ln_gpio_opcode[11][1], 0)
SV(76, "LN GPIO 6 Off Opcode 3", eeprom.ln_gpio_opcode[11][2], 0)
SV(77, "LN GPIO 7 On Opcode 1", eeprom.ln_gpio_opcode[12][0], 0)
SV(78, "LN GPIO 7 On Opcode 2", eeprom.ln_gpio_opcode[12][1], 0)
SV(79, "LN GPIO 7 On Opcode 3", eeprom.ln_gpio_opcode[12][2], 0)
SV(80, "LN GPIO 7 Off Opcode 1", eeprom.ln_gpio_opcode[13][0], 0)
SV(81, "LN GPIO 7 Off Opcode 2", eeprom.ln_gpio_opcode[13][1], 0)
SV(82, "LN GPIO 7 Off Opcode 3", eeprom.ln_gpio_opcode[13][2], 0)
SV(83, "LN GPIO 8 On Opcode 1", eeprom.ln_gpio_opcode[14][0], 0)
SV(84, "LN GPIO 8 On Opcode 2", eeprom.ln_gpio_opcode[14][1], 0)
SV(85, "LN GPIO 8 On Opcode 3", eeprom.ln_gpio_opcode[14][2], 0)
SV(86, "LN GPIO 8 Off Opcode 1", eeprom.ln_gpio_opcode[15][0], 0)
SV(87, "LN GPIO 8 Off Opcode 2", eeprom.ln_gpio_opcode[15][1], 0)
SV(88, "LN GPIO 8 Off Opcode 3", eeprom.ln_gpio_opcode[15][2], 0)
SV(89, "PWM Port 1 Target", pwm_port[0].dimm_target, 0)
SV(90, "PWM Port 2 Target", pwm_port[1].dimm_target, 0)
SV(91, "PWM Port 3 Target", pwm_port[2].dimm_target, 0)
SV(92, "PWM Port 4 Target", pwm_port[3].dimm_target, 0)
SV(93, "PWM Port 5 Target", pwm_port[4].dimm_target, 0)
SV(94, "PWM Port 6 Target", pwm_port[5].dimm_target, 0)
SV(95, "PWM Port 7 Target", pwm_port[6].dimm_target, 0)
SV(96, "PWM Port 1 Delta", pwm_port[0].dimm_delta, 0)
SV(97, "PWM Port 2 Delta", pwm_port[1].dimm_delta, 0)
SV(98, "PWM Port 3 Delta", pwm_port[2].dimm_delta, 0)
SV(99, "PWM Port 4 Delta", pwm_port[3].dimm_delta, 0)
SV(100, "PWM Port 5 Delta", pwm_port[4].dimm_delta, 0)
SV(101, "PWM Port 6 Delta", pwm_port[5].dimm_delta, 0)
SV(102, "PWM Port 7 Delta", pwm_port[6].dimm_delta, 0)
SV(103, "Servo 1 Setpoint", servo[0].position_setpoint, 0)
SV(104, "Servo 1 Delta", servo[0].time_delta, 0)
SV(105, "Servo 2 Setpoint", servo[1].position_setpoint, 0)
SV(106, "Servo 2 Delta", servo[1].time_delta, 0)
SV_CONST(107, "IR Read Parameter Address", 107)
SV_CONST(108, "IR Read Parameter", 108)
SV_CONST(109, "IR Read Parameter Value", 109)
SV(110, "LN GPIO Opcode Transmit L", ln_gpio_opcode_tx, 0)
SV(111, "LN GPIO Opcode Transmit H", ln_gpio_opcode_tx2, 0)
SV_TABLE_END()


void cmd_exec(void)
{
	switch (cmd_register)
	{
		default:
		case 0:
			break;
		case 1:
			eeprom_sync_storage();
			ubasic_save_scripts();
			break;
		case 2:
			wdt_enable(WDTO_1S);
			break;
		case 3:
			eeprom_load_defaults();
			ubasic_load_default_scripts();
			break;
		case 4:
			USB_enter_bootloader();
			break;
	}
	
	cmd_register = 0;
}

void dimm_target_parameter_update(void)
{
	uint8_t index;
	
	for (index=0;index<PWM_PORT_COUNT;index++)
	{
		if (dimm_parameter_select&(1<<index))
		{
			pwm_port[index].dimm_target = dimm_target_temp;
		}
	}
}

void dimm_delta_parameter_update(void)
{
	uint8_t index;

	for (index=0;index<PWM_PORT_COUNT;index++)
	{
		if (dimm_parameter_select&(1<<index))
		{
			pwm_port[index].dimm_delta = dimm_delta_temp;
		}
	}
}

void ln_update_threshold(void)
{
	ACA.CTRLB = ((eeprom.ln_threshold/4)-1)&0x3F;
}

#endif /* PARAMETER_TABLE_H_ */
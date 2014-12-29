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

#include <stdint.h>
#include <avr/eeprom.h>
#include "sv_support.h"
#include "eeprom.h"

#include "loconet.h"
#include "SVStorage.h"  // interface to public configuration storage in EEPROM
#include "IdStorage.h"

#include "sv_table.h"

volatile uint8_t svChangedFlag = 0;
uint16_t sv_count = 0;

inline byte readSVStorage(word offset)
{
	uint8_t index;
	sv_entry_t entry;
	
	if (offset<SV_COUNT)
	{
		memcpy_P(&entry,&sv_table[offset],sizeof(sv_entry_t));
		
		switch (entry.type&0xF)
		{
			case 1:
				return *((uint8_t *)entry.variable);
			case 5:
				return (uint8_t) (intptr_t) entry.variable;
		}
		return 0;
	}
	else
	{
		offset -= SV_COUNT;
		
		for (index=0;index<SV_BLOCK_COUNT;index++)
		{
			if (offset<sv_block[index].size)
			{
				switch (sv_block[index].type)
				{
					case 1:
						return ((uint8_t *)sv_block[index].variable)[offset];
						break;
					case 2:
					default:
						return 0;
				}
			}
			else
			{
				offset -= sv_block[index].size;
			}
		}
	}
	
	return 0;
}

byte writeSVStorage(word offset, byte value)
{
	uint8_t index;
	sv_entry_t entry;
	
	if (offset<SV_COUNT)
	{
		memcpy_P(&entry,&sv_table[offset],sizeof(sv_entry_t));
				
		switch (entry.type&0xF)
		{
			case 1:
				*((uint8_t *)entry.variable) = value;
				break;
		}
		
		if (entry.update!=0)
			entry.update();
	}
	else
	{
		offset -= SV_COUNT;
		
		for (index=0;index<SV_BLOCK_COUNT;index++)
		{
			if (offset<sv_block[index].size)
			{
				switch (sv_block[index].type)
				{
					case 1:
						((uint8_t *)sv_block[index].variable)[offset] = value;
						svChangedFlag |= 1;
						return 0;
						break;
					case 2:
					default:
						return 0;
				}
			}
			else
			{
				offset -= sv_block[index].size;
			}
		}
	}
	
	svChangedFlag |= 1;
	return 0;
}

byte isValidSVStorage(word Offset)
{
	if (sv_count==0)
		initSVCount();
	
	return (Offset<sv_count);
}

word readSVDestinationId(void)
{
	return eeprom.sv_destination_id;
}

word writeSVDestinationId(word usId)
{
	eeprom.sv_destination_id = usId;
	eeprom_shadow.sv_destination_id = usId;
	eeprom_update_word(&eeprom_eemem.sv_destination_id, usId);
	return 0;
}

void initSVCount(void)
{
	uint8_t index;
	
	sv_count = SV_COUNT;
	for (index=0;index<SV_BLOCK_COUNT;index++)
	{
		sv_count += sv_block[index].size;
	}
}
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

#ifndef __SV_SUPPORT_H
#define __SV_SUPPORT_H

#include "common_defs.h"
#include <avr/pgmspace.h>

void initSVCount(void);
byte writeSVStorage(word offset, byte value);
byte readSVStorage(word offset);

typedef struct sv_entry {
	unsigned char type;
	void *variable;
	void (*update)(void);
} sv_entry_t;

typedef struct sv_block_entry {
	uint8_t type;
	uint8_t size;
	void *variable;
} sv_block_entry_t;

#define SV_TABLE_BEGIN()   const struct sv_entry sv_table[] PROGMEM = {
#define SV_CONST(x, name, val)	[x] = { 5, (void *) val, 0},
#define SV(x, name, var, update)	[x] = { 1, &(var), update },
#define SV_LSB(x, name, var, update)	[x] = { 1, &(((uint8_t *)&var)[0]), update },
#define SV_MSB(x, name, var, update)	[x] = { 1, &(((uint8_t *)&var)[1]), update },
#define SV_TABLE_END()     };
#define SV_COUNT	(sizeof(sv_table)/sizeof(struct sv_entry))

#define SV_BLOCK_TABLE_BEGIN()	const struct sv_block_entry sv_block[] = {
#define SV_BLOCK_MAP(x, size, name, ptr) [x] = { 1, size, ptr },
#define SV_BLOCK(x, size, name, fun) [x] = { 2, size, fun },
#define SV_BLOCK_TABLE_END()	};
#define SV_BLOCK_COUNT	(sizeof(sv_block)/sizeof(struct sv_block_entry))

#endif

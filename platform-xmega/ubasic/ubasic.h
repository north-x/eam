/*
 * Copyright (c) 2006, Adam Dunkels
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
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ---------------------------------------------------------------------------
 * Source heavily modified by Rene Böllhoff (reneboellhoff@gmx.de); Jun 2010
 *
 *  - Removed DEBUG_PRINTF-Option
 *  - Added Char-Device-Functionality for flexible input
 *  - Added Compiler-Features & Changes for parallel use (Text/Bin)
 *  - Added Extensions for Access to C-Functions/Arrays/Variables
 *  - Added Preprocessor Generated Code with common Config (basic_cfg.h)
 *
 * ---------------------------------------------------------------------------
 */
#ifndef __UBASIC_H__
	#define __UBASIC_H__

		#ifndef BASIC_CONFIG_FILE
			#define BASIC_CONFIG_FILE "basic_cfg.h"
		#endif


		#define BASIC_CONFIG
			#include BASIC_CONFIG_FILE
		#undef BASIC_CONFIG

		#define COMMON_CONFIG
			#include BASIC_CONFIG_FILE
		#undef COMMON_CONFIG
		
		#define TOKENIZER_CONFIG
			#include BASIC_CONFIG_FILE
		#undef TOKENIZER_CONFIG


		void ubasic_init					(void);
		void ubasic_reset					(void);
		void ubasic_run						(void);
		I8	 ubasic_finished			(void);
		I16  ubasic_get_variable	(I8 varnum);
		I16 ubasic_get_variable_bit(I8 varnum, I8 bitpos);
		void ubasic_set_variable	(I8 varnum, I16 value);
		void ubasic_set_variable_bit(I8 varnum, I8 bitpos, I16 value);

		void ubasic_start(void);
		void ubasic_load_scripts(void);
		void ubasic_load_default_scripts(void);
		void ubasic_save_scripts(void);
		void ubasic_supervisor(void);
/*--- Datentyp für For-Schleifen ---*/

typedef struct
{
	I16 line_after_for;		// Zeile/Tokenposition nach dem for-Statement
	I16 to;								// Schleifenzähler bis
	I8 for_variable;			// Schleifenvariable
} T_FOR_STATE;



/*--- Lokale Variablen ---*/
typedef struct
{
	I16					gosub_stack	[MAX_GOSUB_STACK_DEPTH];	// Stack für Gosub/Return
	T_FOR_STATE for_stack		[MAX_FOR_STACK_DEPTH];		// Stack für For-Schleifen
	I16					variables		[MAX_VARNUM];							// Variablen
	U8					gosub_stack_ptr;											// Gosub-Stackpointer
	U8					for_stack_ptr;												// For-Stackpointer
	I8					ended;																// Basic-Prg beendet
	I16					basic_timer;	// aktueller Timerstand (wird durch "wait" gesetzt)

	/*--- Externe Variablen wenn Compiler benutzt wird ---*/
	TOKEN	userkeywordindex;								// Index des erkannten Keywords (-1 = nicht gefunden)
	char 	userkeywordtype;								// Typ des erkannten Keywords 1 : Funktion 2 : Variable 3 : Array)
	int 	userkeyworddata;								// zus. Daten des erk. Keywords (Index für Array)
	void *userkeywordptr;							// Zeiger auf Fkt/Var/Array (je nach Typ)
	
	//I8		runmode;				// Run-Modus (RM_TEXT/RM_COMPILED)
	I16	gototokenpos;		// zuletzt ermittelte Goto/Gosub Tokenposition (nur runmode == RM_COMPILED)
	//unsigned char stringlength;											// aktuelle String-länge
	char tokendata [16];	// Buffer für aktuelles Token
	
	/* --- ein ganz normaler char-Zeiger --- Zeigt auf Programmtext*/
	char *charptr;
	char *charptrbak;
	TOKEN current_token;  // zuletzt erkanntes Token
	char mem[MAX_PROGRAM_LEN];
} ubasic_context_t;

typedef struct ubasic_program {
	uint8_t program[MAX_PROGRAM_LEN];
} ubasic_program_t;

extern ubasic_context_t *ctx;
extern ubasic_context_t ubasic_scripts[UBASIC_SCRIPT_COUNT];
	#endif /* __UBASIC_H__ */

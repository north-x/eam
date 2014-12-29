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


/*--- Allgemeine Header ---*/

#include <stdio.h> /* printf() */
#include <string.h> /* memset() */
#ifdef WIN32
	#include <conio.h>
#endif


/*--- uBasic Header & Platformen ---*/

#include "platform.h"
#include "ubasic.h"
#include "tokenizer.h"
#include "sys/process.h"
#include "sys/etimer.h"
#include <avr/eeprom.h>
#include "eeprom.h"

PROCESS(ubasic_process, "UBasic");
PROCESS(ubasic_tick_process, "UBasic Tick Process");
static struct etimer ub_timer;
/*--- Forward Deklarationen ---*/

I16		expr						(void);		// Ausdruck holen
void	statement				(void);		// Statement bearbeiten

ubasic_context_t *ctx;
ubasic_context_t ubasic_scripts[UBASIC_SCRIPT_COUNT];

ubasic_program_t ubasic_script_store[UBASIC_SCRIPT_COUNT] EEMEM;

uint8_t ubasic_autostart;
uint8_t ubasic_status_run;
uint8_t ubasic_script_status;

void ubasic_start(void)
{
	ubasic_load_scripts();
	process_start(&ubasic_process, NULL);
	process_start(&ubasic_tick_process, NULL);
}

PROCESS_THREAD(ubasic_process, ev, data)
{
	PROCESS_BEGIN();
	
	while (1)
	{
		ubasic_supervisor();
		PROCESS_PAUSE();
	}
	
	PROCESS_END();
}

PROCESS_THREAD(ubasic_tick_process, ev, data)
{
	static uint8_t index;
	PROCESS_BEGIN();
	
	etimer_set(&ub_timer, 1E-3*CLOCK_SECOND);
	
	while (1)
	{
		for (index=0;index<UBASIC_SCRIPT_COUNT;index++)
		{
			if (ubasic_scripts[index].basic_timer)
				ubasic_scripts[index].basic_timer--;
		}
		etimer_reset(&ub_timer);
		PROCESS_YIELD();
	}
	
	PROCESS_END();
}

void ubasic_supervisor(void)
{
	static uint8_t ubasic_script_status_int = 0;
	
	uint8_t index;
	
	for (index=0;index<UBASIC_SCRIPT_COUNT;index++)
	{
		ctx = &ubasic_scripts[index];
		
		if (ubasic_finished())
		{
			ubasic_status_run &= ~(1<<index);
			
			// Enable script if corresponding bit was set
			if (((ubasic_script_status^ubasic_script_status_int)&ubasic_script_status&(1<<index)))
			{
				tokenizer_set_text(ctx->mem, 0);
				ubasic_init();
			}
		}
		else
		{
			ubasic_status_run |= (1<<index);
			
			// Check if program terminated or user requests to stop it
			if (((ubasic_script_status^ubasic_script_status_int)&(~ubasic_script_status)&(1<<index)))
			{
				ctx->ended = 1;
				ubasic_script_status &= ~(1<<index);
			}
			
			if (ctx->basic_timer==0)
				ubasic_run();
		}
	}
	
	ubasic_script_status_int = ubasic_script_status;
}

void ubasic_load_scripts(void)
{
	ubasic_context_t* ub_ctx;
	uint8_t index, temp8;
	
	for (index=0;index<UBASIC_SCRIPT_COUNT;index++)
	{
		ub_ctx = &ubasic_scripts[index];
		
		temp8 = eeprom_read_byte(ubasic_script_store[index].program);
		
		if (temp8!=0xFF)
		{
			eeprom_read_block(ub_ctx->mem, ubasic_script_store[index].program, MAX_PROGRAM_LEN);
			if (eeprom.ubasic_autostart&(1<<index))
			{
				ubasic_script_status |= (1<<index);
				//eeprom.tse_autostart |= (1<<index);
			}
		}
		else
		{
			ubasic_load_default_scripts();
			ubasic_script_status = eeprom.ubasic_autostart;
		}
		
		/*ctx = &ubasic_scripts[index];
		ubasic_set_text(ctx->mem, 0);
		ubasic_init();*/
		ub_ctx->ended = 1;
	}
}

void ubasic_load_default_scripts(void)
{
	ubasic_context_t* ctx;
	uint8_t index;
	
	for (index=0;index<UBASIC_SCRIPT_COUNT;index++)
	{
		ctx = &ubasic_scripts[index];
		ctx->mem[0] = TOKENIZER_ENDOFINPUT;
	}
	
	ctx = &ubasic_scripts[0];
	ctx->mem[0] = 0x08;
	ctx->mem[1] = 0x84;
	ctx->mem[2] = 0x01;
	ctx->mem[3] = 0x00;
	ctx->mem[4] = 0x24;
	ctx->mem[5] = 0x02;
	ctx->mem[6] = 0x00;
	ctx->mem[7] = 0x0e;
	ctx->mem[8] = 0x33;
	ctx->mem[9] = 0x48;
	ctx->mem[10] = 0x84;
	ctx->mem[11] = 0x01;
	ctx->mem[12] = 0x00;
	ctx->mem[13] = 0x15;
	ctx->mem[14] = 0x02;
	ctx->mem[15] = 0x00;
	ctx->mem[16] = 0x0e;
	ctx->mem[17] = 0x09;
	ctx->mem[18] = 0x44;
	ctx->mem[19] = 0x05;
	ctx->mem[20] = 0x24;
	ctx->mem[21] = 0x02;
	ctx->mem[22] = 0x19;
	ctx->mem[23] = 0x48;
	ctx->mem[24] = 0x04;
	ctx->mem[25] = 0x06;
	ctx->mem[26] = 0x23;
	ctx->mem[27] = 0x02;
	ctx->mem[28] = 0x81;
	ctx->mem[29] = 0x0e;
	ctx->mem[30] = 0x17;
	ctx->mem[31] = 0xc4;
	ctx->mem[32] = 0x03;
	ctx->mem[33] = 0x00;
	ctx->mem[34] = 0x24;
	ctx->mem[35] = 0x02;
	ctx->mem[36] = 0x00;
	ctx->mem[37] = 0x48;
	ctx->mem[38] = 0x04;
	ctx->mem[39] = 0x06;
	ctx->mem[40] = 0x15;
	ctx->mem[41] = 0x02;
	ctx->mem[42] = 0x19;
	ctx->mem[43] = 0x0e;
	ctx->mem[44] = 0x25;
	ctx->mem[45] = 0xc4;
	ctx->mem[46] = 0x02;
	ctx->mem[47] = 0x00;
	ctx->mem[48] = 0x24;
	ctx->mem[49] = 0x02;
	ctx->mem[50] = 0x01;
	ctx->mem[51] = 0x48;
	ctx->mem[52] = 0x84;
	ctx->mem[53] = 0x01;
	ctx->mem[54] = 0x00;
	ctx->mem[55] = 0x24;
	ctx->mem[56] = 0x02;
	ctx->mem[57] = 0x00;
	ctx->mem[58] = 0x0e;
	ctx->mem[59] = 0x33;
	ctx->mem[60] = 0x44;
	ctx->mem[61] = 0x05;
	ctx->mem[62] = 0x24;
	ctx->mem[63] = 0x02;
	ctx->mem[64] = 0xe6;
	ctx->mem[65] = 0x48;
	ctx->mem[66] = 0x04;
	ctx->mem[67] = 0x06;
	ctx->mem[68] = 0x22;
	ctx->mem[69] = 0x02;
	ctx->mem[70] = 0x80;
	ctx->mem[71] = 0x0e;
	ctx->mem[72] = 0x41;
	ctx->mem[73] = 0xc4;
	ctx->mem[74] = 0x03;
	ctx->mem[75] = 0x00;
	ctx->mem[76] = 0x24;
	ctx->mem[77] = 0x02;
	ctx->mem[78] = 0x01;
	ctx->mem[79] = 0x48;
	ctx->mem[80] = 0x04;
	ctx->mem[81] = 0x06;
	ctx->mem[82] = 0x15;
	ctx->mem[83] = 0x02;
	ctx->mem[84] = 0xe6;
	ctx->mem[85] = 0x0e;
	ctx->mem[86] = 0x4f;
	ctx->mem[87] = 0xc4;
	ctx->mem[88] = 0x02;
	ctx->mem[89] = 0x00;
	ctx->mem[90] = 0x24;
	ctx->mem[91] = 0x02;
	ctx->mem[92] = 0x01;
	ctx->mem[93] = 0x4e;
	ctx->mem[94] = 0x00;
	ctx->mem[95] = 0x41;
	
	ctx = &ubasic_scripts[1];
	ctx->mem[0] = 0x08;
	ctx->mem[1] = 0x84;
	ctx->mem[2] = 0x01;
	ctx->mem[3] = 0x01;
	ctx->mem[4] = 0x24;
	ctx->mem[5] = 0x02;
	ctx->mem[6] = 0x00;
	ctx->mem[7] = 0x0e;
	ctx->mem[8] = 0x33;
	ctx->mem[9] = 0x48;
	ctx->mem[10] = 0x84;
	ctx->mem[11] = 0x01;
	ctx->mem[12] = 0x01;
	ctx->mem[13] = 0x15;
	ctx->mem[14] = 0x02;
	ctx->mem[15] = 0x00;
	ctx->mem[16] = 0x0e;
	ctx->mem[17] = 0x09;
	ctx->mem[18] = 0x44;
	ctx->mem[19] = 0x08;
	ctx->mem[20] = 0x24;
	ctx->mem[21] = 0x02;
	ctx->mem[22] = 0x19;
	ctx->mem[23] = 0x48;
	ctx->mem[24] = 0x04;
	ctx->mem[25] = 0x09;
	ctx->mem[26] = 0x23;
	ctx->mem[27] = 0x02;
	ctx->mem[28] = 0x81;
	ctx->mem[29] = 0x0e;
	ctx->mem[30] = 0x17;
	ctx->mem[31] = 0xc4;
	ctx->mem[32] = 0x03;
	ctx->mem[33] = 0x01;
	ctx->mem[34] = 0x24;
	ctx->mem[35] = 0x02;
	ctx->mem[36] = 0x00;
	ctx->mem[37] = 0x48;
	ctx->mem[38] = 0x04;
	ctx->mem[39] = 0x09;
	ctx->mem[40] = 0x15;
	ctx->mem[41] = 0x02;
	ctx->mem[42] = 0x19;
	ctx->mem[43] = 0x0e;
	ctx->mem[44] = 0x25;
	ctx->mem[45] = 0xc4;
	ctx->mem[46] = 0x02;
	ctx->mem[47] = 0x01;
	ctx->mem[48] = 0x24;
	ctx->mem[49] = 0x02;
	ctx->mem[50] = 0x01;
	ctx->mem[51] = 0x48;
	ctx->mem[52] = 0x84;
	ctx->mem[53] = 0x01;
	ctx->mem[54] = 0x01;
	ctx->mem[55] = 0x24;
	ctx->mem[56] = 0x02;
	ctx->mem[57] = 0x00;
	ctx->mem[58] = 0x0e;
	ctx->mem[59] = 0x33;
	ctx->mem[60] = 0x44;
	ctx->mem[61] = 0x08;
	ctx->mem[62] = 0x24;
	ctx->mem[63] = 0x02;
	ctx->mem[64] = 0xe6;
	ctx->mem[65] = 0x48;
	ctx->mem[66] = 0x04;
	ctx->mem[67] = 0x09;
	ctx->mem[68] = 0x22;
	ctx->mem[69] = 0x02;
	ctx->mem[70] = 0x80;
	ctx->mem[71] = 0x0e;
	ctx->mem[72] = 0x41;
	ctx->mem[73] = 0xc4;
	ctx->mem[74] = 0x03;
	ctx->mem[75] = 0x01;
	ctx->mem[76] = 0x24;
	ctx->mem[77] = 0x02;
	ctx->mem[78] = 0x01;
	ctx->mem[79] = 0x48;
	ctx->mem[80] = 0x04;
	ctx->mem[81] = 0x09;
	ctx->mem[82] = 0x15;
	ctx->mem[83] = 0x02;
	ctx->mem[84] = 0xe6;
	ctx->mem[85] = 0x0e;
	ctx->mem[86] = 0x4f;
	ctx->mem[87] = 0xc4;
	ctx->mem[88] = 0x02;
	ctx->mem[89] = 0x01;
	ctx->mem[90] = 0x24;
	ctx->mem[91] = 0x02;
	ctx->mem[92] = 0x01;
	ctx->mem[93] = 0x4e;
	ctx->mem[94] = 0x00;
	ctx->mem[95] = 0x41;

}

void ubasic_save_scripts(void)
{
	ubasic_context_t* ctx;
	uint8_t index;
	
	for (index=0;index<UBASIC_SCRIPT_COUNT;index++)
	{
		ctx = &ubasic_scripts[index];
		eeprom_update_block(ctx->mem, ubasic_script_store[index].program, MAX_PROGRAM_LEN);
	}
}

/*--- uBasic Resetten (Variablen, Stacks, Stackpointer initialisieren) ---*/

void ubasic_reset(void)
{
	// Stackpointer resetten
	ctx->for_stack_ptr = 0;
	ctx->gosub_stack_ptr = 0;
  
	// For-Stack Löschen
	memset(ctx->for_stack, 0, sizeof(ctx->for_stack));
	// Variablen Löschen
	memset(ctx->variables, 0, sizeof(ctx->variables));
	// Gosub-Stack Löschen
	memset(ctx->gosub_stack, 0, sizeof(ctx->gosub_stack));
}


/*--- Zuletzt ermitteltes Token prüfen und wenn ok neues Token ermitteln ---*/

void accept(TOKEN token)
{
	// aktuelles Token mit dem zu erwartenden Token vergleichen
  if(token != tokenizer_token()) {
		// bei Fehler -> Fatal Abort
    FATAL_ABORT(FA_UNEXPECTED_TOKEN);
  }
	// wenn ok -> nächstes Token holen
  tokenizer_next();
}


/*--- Hilfsfunktionen für Extensions ---*/

#ifdef BASIC_USES_EXTENSIONS
	
	/*--- Parameter für C-Funktionsaufruf holen ---*/

	I8 get_parameters(T_USER_FUNC_DATAS *params, I16 numparams, I8 next)
	{
		U8 ptr = 0;
		// wenn keine Parameter erwartet werden, nächstes Token holen und tschüss
		if (numparams == 0) { tokenizer_next(); return 0;} 

		// Solange noch parameter zu holen sind
		while (ptr < numparams)
		{
			switch (tokenizer_token())
			{
				// String ablegen
				case TOKENIZER_STRING 	:
					tokenizer_string (((*params) [ptr].text), MAX_USER_KEYWORD_LEN);
					(*params) [ptr].type = 2;
					tokenizer_next ();
					break;
        // Integer-Wert (Zahl/Ausdruck/Variable) ablegen
				case TOKENIZER_NUMBER 	:
				case TOKENIZER_VARIABLE :
				case TOKENIZER_NAME 		:
					(*params) [ptr].value = expr();
					(*params) [ptr].type = 1;
					break;
				default :
        // alles andere -> Fehler
					FATAL_ABORT(FA_ERROR_IN_PARAMETER_LIST);
			}
			// Komma holen (als Separator für Parameter)
			if (++ptr < numparams) { accept (TOKENIZER_COMMA); }
		}
		// Optional nächstes Token schon holen
		if (next) { tokenizer_next(); }
		return 0;
	}


	/*--- Array-Index für ext. C-Array holen ---*/

	I16 get_arrayindex (void)
	{
		I16 i;
		accept (TOKENIZER_LEFTBRACK);		// Eckige Klammer auf
		i = expr();											// Index als Ausdruck holen
		accept (TOKENIZER_RIGHTBRACK);	// Eckige Klammer zu
		return i;
	}
#endif


/*--- Ausdrucks-Funktionen ---*/


/*--- Zahl/Variable/ext. Faktor inkl Klammerausdrücke ---*/

I16 factor(void)
{
  I16								r = 0;		// Result/Temporäre Variable
#ifdef BASIC_USES_EXTENSIONS
  I16								i;				// Index für Array
	T_USER_FUNC_DATAS temp;			// Parameter für ext. Funktionsaufruf
	T_BASIC_USER_FUNC func;			// Funktionszeiger auf ext. Funktion
	TOKEN							idx;			// Index für ext. Aufruf
	U8								type;			// ext. Aufruftyp (1=Fkt 2=Var 3=Arr)
	U16								data;			// zus. Daten für ext. Aufruf
	void							*ptr;			// Zeiger für ext. Aufruf (Fkt/Var/Array)
	U8 *arr_U8;
	I8 *arr_I8;
	U16 *arr_U16;
	I16 *arr_I16;
#endif

  switch(tokenizer_token()) {
		//---------------------------------------------------------------
		// Zahl verarbeiten
		case TOKENIZER_NUMBER:
			r = tokenizer_num();
			accept(TOKENIZER_NUMBER);
			break;
#ifdef BASIC_USES_EXTENSIONS
		//---------------------------------------------------------------
		// Name verarbeiten
		case TOKENIZER_NAME:
			idx = tokenizer_userdata (&type, &data, &ptr);
			if (idx == -1) { FATAL_ABORT (FA_EXTENSION_WORD_NOT_FOUND); }
			switch (type>>4)
			{
				case 1:
				  // C-Funktion aufrufen
					accept (TOKENIZER_NAME);
					get_parameters (&temp, data, 0);
					func = ptr;
					r = func (temp);
					break;
				case 2:
				  // C-Variable holen 
					accept(TOKENIZER_NAME);
					
					switch (type&0xF)
					{
						case 1:
							r = *((uint8_t *) ptr);
							break;
						case 2:
							r = *((int8_t *) ptr);
							break;
						case 3:
							r = *((U16 *) ptr);
							break;
						case 4:
							r = *((I16 *) ptr);
							break;
					}
					
					if (((int8_t)tokenizer_variable_bitpos())>=0)
					{
						r = (r>>tokenizer_variable_bitpos())&1;
					}
					
					break;
				case 3: 
				  // Wert aus C-Array holen
					accept(TOKENIZER_NAME);
					i = get_arrayindex();
					if ((i >= 0) && (i < data))
					{
						switch (type&0xF)
						{
							case 1:
								arr_U8 = (U8 *) ptr;
								r = arr_U8[i];
								break;
							case 2:
								arr_I8 = (I8 *) ptr;
								r = arr_I8[i];
								break;
							case 3:
								arr_U16 = (U16 *) ptr;
								r = arr_U16[i];
								break;
							case 4:
								arr_I16 = (I16 *) ptr;
								r = arr_I16[i];
								break;
						}
					}
					break;
			}
			break;
#endif
		//---------------------------------------------------------------
		//  Klammer-ausdrücke verarbeiten
		case TOKENIZER_LEFTPAREN:
			accept(TOKENIZER_LEFTPAREN);
			r = expr();
			accept(TOKENIZER_RIGHTPAREN);
			break;
		//---------------------------------------------------------------
		case TOKENIZER_ERROR :
			FATAL_ABORT (FA_ERROR_IN_FACTOR);
			break;
		//---------------------------------------------------------------
		// Basic-Variable holen
		default:
			if (tokenizer_variable_bitpos()<0)
				r = ubasic_get_variable(tokenizer_variable_num());
			else
				r = ubasic_get_variable_bit(tokenizer_variable_num(), tokenizer_variable_bitpos());
				
			accept(TOKENIZER_VARIABLE);
			break;
  }
  return r;
}


/*--- Für Terme/Produkte/Vergleiche den Abschnitt BASIC_TOKEN_OPS einblenden ---*/

#define BASIC_TOKEN_OPS

/*--- Term holen ---*/

I16 term(void)
{
  I16 f1, f2;
  TOKEN tk;

	// 1. Faktor holen
  f1 = factor();
	// Operation holen
  tk = tokenizer_token();
	// Solange wie TERM-Operationen ausgeführt werden sollen ...
	while (
		#define TOKEN_CHAR_TERM(token,op,func)	(tk == token) ||
		#define TOKEN_CHAR_EXPR(token,op,func)
		#define TOKEN_CHAR_REL(token,op,func)
			#include BASIC_CONFIG_FILE
		#undef TOKEN_CHAR_TERM
		#undef TOKEN_CHAR_EXPR
		#undef TOKEN_CHAR_REL
		(0)) {
		// nächstes Token
    tokenizer_next();
		// weiteren Faktor holen
    f2 = factor();
		// ausführen
		switch (tk)
		{
		  // Umschaltung Aufruf der operation via Funktion / Textersetzung als Direkt-Anweisung
#ifndef BASIC_USES_FUNC_AS_OPS
			#define TOKEN_CHAR_TERM(token,op,func)	case token : f1 = f1 op f2; break;
#else
			#define TOKEN_CHAR_TERM(token,op,func)	case token : f1 = func (f1, f2); break;
#endif
			#define TOKEN_CHAR_EXPR(token,op,func)
			#define TOKEN_CHAR_REL(token,op,func)
				#include BASIC_CONFIG_FILE
			#undef TOKEN_CHAR_TERM
			#undef TOKEN_CHAR_EXPR
			#undef TOKEN_CHAR_REL
		  case TOKENIZER_ERROR :
				FATAL_ABORT (FA_ERROR_IN_TERM);
				break;
		}

    tk = tokenizer_token();
  }
  return f1;
}


/*--- Ausdruck holen ---*/

I16 expr(void)
{
  I16 t1, t2;
  TOKEN tk;
  
	// 1. Term holen
  t1 = term();
	// Operation holen
  tk = tokenizer_token();
	// Solange wie EXPR-Operationen ausgeführt werden sollen ...
	while (
		#define TOKEN_CHAR_TERM(token,op,func) 
		#define TOKEN_CHAR_EXPR(token,op,func)	(tk == token) ||
		#define TOKEN_CHAR_REL(token,op,func)
			#include BASIC_CONFIG_FILE
		#undef TOKEN_CHAR_TERM
		#undef TOKEN_CHAR_EXPR
		#undef TOKEN_CHAR_REL
		(0)) {
		// nächstes Token
    tokenizer_next();
		// weiteren Faktor holen
    t2 = term();
		// ausführen
		switch (tk)
		{
			#define TOKEN_CHAR_TERM(token,op,func)	
		  // Umschaltung Aufruf der operation via Funktion / Textersetzung als Direkt-Anweisung
#ifndef BASIC_USES_FUNC_AS_OPS
			#define TOKEN_CHAR_EXPR(token,op,func)	case token : t1 = t1 op t2; break;
#else
			#define TOKEN_CHAR_EXPR(token,op,func)	case token : t1 = func (t1, t2); break;
#endif
			#define TOKEN_CHAR_REL(token,op,func)
				#include BASIC_CONFIG_FILE
			#undef TOKEN_CHAR_TERM
			#undef TOKEN_CHAR_EXPR
			#undef TOKEN_CHAR_REL
		  case TOKENIZER_ERROR :
				FATAL_ABORT (FA_ERROR_IN_EXPRESSION);
				break;
		}
    tk = tokenizer_token();
  }
  return t1;
}


/*--- Vergleich ---*/

I16 relation(void)
{
  I16 r1, r2;
  TOKEN tk;
  
	// 1. Operanden holen
  r1 = expr();
	// Operation holen
  tk = tokenizer_token();
	// Solange wie Vergleichs-Operationen ausgeführt werden sollen ...
	while (
		#define TOKEN_CHAR_TERM(token,op,func) 
		#define TOKEN_CHAR_EXPR(token,op,func)
		#define TOKEN_CHAR_REL(token,op,func)		(tk == token) ||
			#include BASIC_CONFIG_FILE
		#undef TOKEN_CHAR_TERM
		#undef TOKEN_CHAR_EXPR
		#undef TOKEN_CHAR_REL
		(0)) {
		// nächstes Token
    tokenizer_next();
		// weiteren Faktor holen
    r2 = expr();
		// ausführen
		switch (tk)
		{
			#define TOKEN_CHAR_TERM(token,op,func)	
			#define TOKEN_CHAR_EXPR(token,op,func)
		  // Umschaltung Aufruf der operation via Funktion / Textersetzung als Direkt-Anweisung
#ifndef BASIC_USES_FUNC_AS_OPS
			#define TOKEN_CHAR_REL(token,op,func)		case token : r1 = r1 op r2; break;
#else
			#define TOKEN_CHAR_REL(token,op,func)		case token : r1 = func (r1, r2); break;
#endif
				#include BASIC_CONFIG_FILE
			#undef TOKEN_CHAR_TERM
			#undef TOKEN_CHAR_EXPR
			#undef TOKEN_CHAR_REL
		}
    tk = tokenizer_token();
  }
  return r1;
}


#undef BASIC_TOKEN_OPS


/*--- Gehe zu Zeile (für Goto/Gosub/For) ---*/

void jump_linenum(I16 linenum)
{
	// Im Compiled Mode -> Einfach Zeiger setzen
	tokenizer_jump_to_pos(linenum);
}


/*--- Goto-Statement ---*/

void goto_statement(void)
{
	// Erwarte Goto
	accept(TOKENIZER_GOTO);
	// Bei Compilerunterstützung : Je nach RunMode Zeilennummer oder Position holen und springen
	jump_linenum(ctx->gototokenpos);
}


/*--- Print-Statement  ---*/

void print_statement(void)
{
	// Temporärer String
	char string[MAX_STRINGLEN];

	// Erwarte Print
	accept(TOKENIZER_PRINT);
	do {
		// Soll ein String gedruckt werden ? 
    if(tokenizer_token() == TOKENIZER_STRING) {
      tokenizer_string(string, sizeof(string));
      BASIC_PRINTF_P (PSTR("%s"), string);
      tokenizer_next();
		// Soll ein Space gedruckt werden ? 
    } else if(tokenizer_token() == TOKENIZER_COMMA) {
      BASIC_PRINTF_P (PSTR(" "));
      tokenizer_next();
    // Platzhalter ? 
#ifdef TOKENIZER_SEMICOLON
    } else if(tokenizer_token() == TOKENIZER_SEMICOLON) {
      tokenizer_next();
#endif
    // Soll der Wert einer Variablen gedruckt werden ? 
    } else if((tokenizer_token() == TOKENIZER_VARIABLE) ||
#ifdef BASIC_USES_EXTENSIONS
    // Soll der Wert einer Variablen (Extensions) gedruckt werden ? 
				(tokenizer_token() == TOKENIZER_NAME)  ||
#endif
	      (tokenizer_token() == TOKENIZER_NUMBER)) 
				 {
      BASIC_PRINTF_P (PSTR("%d"), expr());
    } else {
      break;
    }
		// Solange ausführen bis Ende des Textes oder der Zeile
  } while(tokenizer_token() != TOKENIZER_CR &&
	  tokenizer_token() != TOKENIZER_ENDOFINPUT);
	// Neue Zeile nicht vergessen
  PRINTF_P (PSTR("\n\r"));
	// Nächstes Token Bitte
  tokenizer_next();
}


/*--- If-Statement ---*/

void if_statement(void)
{
  I16 r;
  
	// Erwarte If
  accept(TOKENIZER_IF);
	// Vergleich
  r = relation();
	// Erwarte Then
  if(tokenizer_token()==TOKENIZER_THEN) {
		accept(TOKENIZER_THEN);
  }
  // Wenn vergleich TRUE then Zweig ausführen
  if(r) {
    statement();
  } else {
		// Ansonsten Then überlesen bis Ende der Zeile, Ende des Textes oder eben Else
    do {
      tokenizer_next();
    } while(tokenizer_token() != TOKENIZER_ELSE &&
	    tokenizer_token() != TOKENIZER_CR &&
	    tokenizer_token() != TOKENIZER_ENDOFINPUT);
		// Bei Else -> Else Zweig ausführen
    if(tokenizer_token() == TOKENIZER_ELSE) {
      tokenizer_next();
      statement();
    // Bei Zeilenende -> Nächstes Token
    } else if(tokenizer_token() == TOKENIZER_CR) {
      tokenizer_next();
    }
  }
}


/*--- Verkürztes Let-Statement ---*/

void let_statement(void)
{
  I8 var;
  I8 bitpos;
  
	// Variablennummer holen 
  var = tokenizer_variable_num();
  
  bitpos = tokenizer_variable_bitpos();
  //printf("bitpos: %d", bitpos);
	// Erwarte Variable
  accept(TOKENIZER_VARIABLE);
	// Erwarte =
  accept(TOKENIZER_EQ);
	// Hole Ausdruck und weise den Wert der Variablennummer zu
  if (bitpos<0)
	  ubasic_set_variable(var, expr());
  else
      ubasic_set_variable_bit(var, bitpos, expr());
	// Erwarte Ende der Zeile
  accept(TOKENIZER_CR);
}


/*--- Normales Let-Statement ---*/
#ifdef TOKENIZER_LET
void let_statement_2(void)
{
	// Erwarte Let
	accept (TOKENIZER_LET);
	// Der Rest wie gehabt
	let_statement ();
}
#endif

void wait_statement(void)
{
	// Erwarte Wait
	accept(TOKENIZER_WAIT);
	
	ctx->basic_timer = expr();
	
	accept(TOKENIZER_CR);
	printf("Timer: %d\n", ctx->basic_timer);
}
/*--- Gosub-Statement ---*/

void gosub_statement(void)
{
	CPOS currpos;
	accept(TOKENIZER_GOSUB);
	currpos = tokenizer_get_rel_pos ();
  accept(TOKENIZER_CR);
  if(ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		ctx->gosub_stack[ctx->gosub_stack_ptr] = currpos;
		ctx->gosub_stack_ptr++;
		jump_linenum(ctx->gototokenpos);
  }
}


/*--- Return-Statement ---*/

void return_statement(void)
{
  accept(TOKENIZER_RETURN);
  if(ctx->gosub_stack_ptr > 0) {
    ctx->gosub_stack_ptr--;
		jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr]);
  }
}


/*--- For-Statement ---*/

void for_statement(void)
{
  I8 for_variable;
	I16  to;
  CPOS pos;

  accept(TOKENIZER_FOR);
  for_variable = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_EQ);
  ubasic_set_variable(for_variable, expr());
  accept(TOKENIZER_TO);
  to = expr();
	pos = tokenizer_get_rel_pos();
  accept(TOKENIZER_CR);

  if(ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
	ctx->for_stack[ctx->for_stack_ptr].line_after_for = pos;
    ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
    ctx->for_stack[ctx->for_stack_ptr].to = to;
    ctx->for_stack_ptr++;
  } else {
  }
}


/*--- Next-Statement ---*/

void next_statement(void)
{
  I8 var;
  
  accept(TOKENIZER_NEXT);
  var = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  if(ctx->for_stack_ptr > 0 &&
     var == ctx->for_stack[ctx->for_stack_ptr - 1].for_variable) {
    ubasic_set_variable(var,
			ubasic_get_variable(var) + 1);
    if(ubasic_get_variable(var) <= ctx->for_stack[ctx->for_stack_ptr - 1].to) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for);
    } else {
      ctx->for_stack_ptr--;
      accept(TOKENIZER_CR);
    }
  } else {
    accept(TOKENIZER_CR);
  }
}



/*--- End-Statement ---*/

void end_statement(void)
{
  accept(TOKENIZER_END);
  ctx->ended = 1;
}



/*--- Statement-Verteiler ---*/

void statement(void)
{
  TOKEN token;
#ifdef BASIC_USES_EXTENSIONS
	T_USER_FUNC_DATAS temp;
	T_BASIC_USER_FUNC func;
	TOKEN idx; 
	U8 type;
	I16 i;
	U16 data;
	void *ptr;
	I8 bitpos;
	U8 *arr_U8;
	I8 *arr_I8;
	U16 *arr_U16;
	I16 *arr_I16;
#endif
  
  token = tokenizer_token();

	if (0) { }
	// Wenn Extensions verwendet werden, werden diese zuerst prüfen
#ifdef BASIC_USES_EXTENSIONS
  else	if (token == TOKENIZER_NAME)
	{
		accept (TOKENIZER_NAME);
		// Daten zum TOKENIZER_NAME holen
		idx = tokenizer_userdata (&type, &data, &ptr);
		// Je nach Aufruftyp ...
		switch (type>>4)
		{
			case 1 :
			  // ... Parameter für C-Fkt holen und ausführen
				get_parameters (&temp, data, 1);
				func = ptr;
				func (temp);
				break;
			case 2 :
			  // ... C-Variable zuweisen
				//arr = ptr;
				bitpos = tokenizer_variable_bitpos();
				
				accept (TOKENIZER_EQ);
				if (bitpos<0)
				{
					switch (type&0xF)
					{
						case 1:
							*((uint8_t *)ptr) = (uint8_t) expr();
							break;
						case 2:
							*((int8_t *)ptr) = (int8_t) expr();
							break;
						case 3:
							*((uint16_t *)ptr) = (uint16_t) expr();
							break;
						case 4:
							*((int16_t *)ptr) = (int16_t) expr();
							break;
					}
				}
				else
				{
					switch (type&0xF)
					{
						case 1:
						case 2:
							if (expr())
								*((uint8_t *)ptr) |= (1<<bitpos);
							else
								*((uint8_t *)ptr) &= ~(1<<bitpos);
							break;
						case 3:
						case 4:
							if (expr())
								*((uint16_t *)ptr) |= (1<<bitpos);
							else
								*((uint16_t *)ptr) &= ~(1<<bitpos);
							break;

					}
				}
				//*arr = expr();
				tokenizer_next();
				break;
			case 3 :
			  // ... C-Array-Eintrag zuweisen
				i = get_arrayindex();
				
				accept (TOKENIZER_EQ);
				if ((i >= 0) && (i < data))
				{
					switch (type&0xF)
					{
						case 1:
							arr_U8 = ptr;
							arr_U8[i] = (U8) expr();
							break;
						case 2:
							arr_I8 = ptr;
							arr_I8[i] = (I8) expr();
							break;
						case 3:
							arr_U16 = ptr;
							arr_U16[i] = (U16) expr();
							break;
						case 4:
							arr_I16 = ptr;
							arr_I16[i] = expr();
							break;
					}
				}
				tokenizer_next();
				break;
			default : 
				FATAL_ABORT(FA_UNIMPLEMENTED_FUNCTION);
				break;
		}
	}
#endif
	else if (token == TOKENIZER_VARIABLE)
	{
		let_statement ();
	} 
	else 
	{
		tokenizer_exec_token_func ();
	}
}


/*--- Zeile als Statement ---*/

void line_statement(void)
{
	if (tokenizer_token()==TOKENIZER_NUMBER)
	  accept(TOKENIZER_NUMBER);  
	statement();
  return;
}




/*--- uBasic Initialisieren ---*/

void ubasic_init(void)
{
	// For/Gosub-Stack/Variablen/Modus initialisieren
	ubasic_reset();
	tokenizer_init();
	ctx->ended = 0;
}

/*--- uBasic ausführen (nur 1 Statement !) ---*/

void ubasic_run(void)
{
  if(tokenizer_finished()) {
    return;
  }

  line_statement();
}


/*--- Abfrage ob uBasic-Prg beendet ist (Fehler oder reguläres Ende) ---*/

I8 ubasic_finished(void)
{
  return ctx->ended || tokenizer_finished();
}


/*--- Interface-Funktion für Variable setzen ---*/

void ubasic_set_variable(I8 varnum, I16 value)
{
  // Prüfen ob Variablennummer gültig
  if(varnum > 0 && varnum <= MAX_VARNUM) {
	  // Variablenwert setzen
    ctx->variables[varnum - 1] = value;
  }
}

void ubasic_set_variable_bit(I8 varnum, I8 bitpos, I16 value)
{
	// Prüfen ob Variablennummer gültig
	if(varnum > 0 && varnum <= MAX_VARNUM) {
		// Variablenwert setzen
		if (value)
			ctx->variables[varnum - 1] |= (1<<bitpos);
		else
			ctx->variables[varnum - 1] &= ~(1<<bitpos);
	}
}

/*--- Interface-Funktion für Variable holen ---*/

I16 ubasic_get_variable(I8 varnum)
{
  // Prüfen ob Variablennummer gültig
  if(varnum > 0 && varnum <= MAX_VARNUM) {
	  // Variablenwert holen
    return ctx->variables[varnum - 1];
  }
  return 0;
}

I16 ubasic_get_variable_bit(I8 varnum, I8 bitpos)
{
	// Prüfen ob Variablennummer gültig
	if(varnum > 0 && varnum <= MAX_VARNUM) {
		// Variablenwert holen
		return (ctx->variables[varnum - 1]>>bitpos)&1;
	}
	return 0;
}



/*--- Dummy-Funktion für execbasicfunc ---*/

void dummyfunc (void)
{
}


/*--- Variablen Operatoren (ohne weitere Prüfung) ---*/

#ifdef BASIC_USES_FUNC_AS_OPS

	I16 basic_add (I16 a, I16 b)		{ return a+b; 	}
	I16 basic_sub (I16 a, I16 b) 		{ return a-b; 	}
	I16 basic_mul (I16 a, I16 b)		{ return a*b;		}
	I16 basic_div (I16 a, I16 b)		{ return a/b; 	}
	I16 basic_mod (I16 a, I16 b)		{ return a%b; 	}
	I16 basic_and (I16 a, I16 b)		{ return a&b; 	}
	I16 basic_or  (I16 a, I16 b)		{ return a|b; 	}
	I16 basic_xor (I16 a, I16 b)		{ return a^b; 	}
	I16 basic_rel_eq (I16 a, I16 b)	{ return a==b; 	}
	I16 basic_rel_gt (I16 a, I16 b)	{ return a>b; 	}
	I16 basic_rel_lt (I16 a, I16 b)	{ return a<b; 	}
	I16 basic_rel_le (I16 a, I16 b)	{ return a<=b; 	}
	I16 basic_rel_ge (I16 a, I16 b)	{ return a>=b; 	}
	I16 basic_rel_ne (I16 a, I16 b)	{ return a!=b; 	}
	I16 basic_shr (I16 a, I16 b)		{ return a>>b; 	}
	I16 basic_shl (I16 a, I16 b)		{ return a<<b; 	}

#endif

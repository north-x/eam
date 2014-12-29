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





/*  --- Header --- */

#include "platform.h"

#include "tokenizer.h"
#include "ubasic.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/*  --- Funktionsprototypen --- */

TOKEN singlechar(void);
TOKEN get_token_from_memory(void);
TOKEN get_next_token_from_memory(void);



/*  --- Lokale Typen (alle Modi) --- */

typedef struct 
{
  TOKEN token;
	T_BASIC_FUNCTION func;
} T_KEYWORD_TOKEN;


/*  --- Lokale Typen für Zeichengerät --- */
#ifdef BASIC_USES_CHARDEVICE

	typedef void (* T_SET_TEXT)         (void *pvData, CPOS len);
	typedef char (* T_WORK_CHAR)				(void);
	typedef void (* T_NEXT_CHAR)				(void);
	typedef CPOS (* T_GET_RPOS)					(void);
	typedef void (* T_SET_RPOS)					(CPOS stRel);

	typedef struct
	{
	  T_SET_TEXT  pfSetText;		// Setze Text im Zeichengerät
		T_WORK_CHAR	pfWorkChar;		// Aktuelles Zeichen
		T_NEXT_CHAR	pfNextChar;		// Nächstes Zeichen
		T_GET_RPOS 	pfGetRPos;		// Hole Position
		T_SET_RPOS 	pfSetRPos;		// Setze Position
	} T_CHAR_DEVICE;

#endif


/*  --- Lokale Variablen (alle Modi) --- */
/*
TOKEN current_token = TOKENIZER_ERROR;  // zuletzt erkanntes Token
int 	lastvalue;		// zuletzt erkannter Wert    (current_token = TOKEN_NUMBER)
int 	lastvarnum;		// zuletzt erkannte Variable (current_token = TOKEN_VARIABLE)
int		lastbitpos;		// zuletzt erkannter Bitmanipulator
CPOS 	laststrpos;		// zuletzt erkannter String  (current_token = TOKEN_STRING)
char	insert_cr;		// Flag um CR im folgenden Token zu 
*/
/* --- Lokale Variablen für Zeichengerät (CharDevice) --- */

#ifdef BASIC_USES_CHARDEVICE
	/* --- Funktionszeiger (Variablen) für CharDevice Funktionen  --- */
	T_SET_TEXT 	pfSetText;		// Setzt Text im Zeichengerät
	T_WORK_CHAR	pfGetChar;		// Holt aktuelles Zeichen
	T_NEXT_CHAR	pfNextChar;		// Setzt Zeiger auf das nächste Zeichen
	T_GET_RPOS	pfGetRPos;		// Holt Zeichenzeiger relativ zum Anfang des Codes
	T_SET_RPOS	pfSetRPos;		// Setzt Zeichenzeiger relativ zum Anfang des Codes
#endif


/*  --- Lokale Variablen für Compiler Funktionalität --- */

#ifdef BASIC_USES_COMPILER
	//char				 	runmode = RM_TEXT;								// Setzt den Verarbeitungsmodus (Text oder Binär)
	//int  					gototokenpos = 0;									// zuletzt erkannte Goto/Gosub "Zeile" innerhalb des Streams
	//unsigned char stringlength;											// aktuelle String-länge
	//char 					tokendata [MAX_USER_KEYWORD_LEN];	// Buffer für aktuelles Token
#endif


/*  --- Lokale Variablen für die Extensions --- */

#ifdef BASIC_USES_EXTENSIONS
// userkeyword handling moved to ctx
#endif





/*  --- Lokale Variablen für Text/Symbol-Tokens  --- */
// Die Daten zu den Tokens werden per Präprozessor generiert.
// Die Daten selbst stehen in der basic_cfg.h und werden hier
// "schablonenartig" in die Arrays eingeblendet (bzw erzeugen
// selbst Arrays)

// Sektion BASIC_TOKENS einblenden
#define BASIC_TOKENS

	/*  --- Token-Texte erzeugen --- */
/*
	#define TOKEN_WORD(name,text,func) const char keyword__##name [] FLASHMEM = text;
	#define TOKEN_CHAR(name,char)
	#define TOKEN_DUP(name,char)
		#include BASIC_CONFIG_FILE
	#undef TOKEN_WORD
	#undef TOKEN_CHAR
	#undef TOKEN_DUP
*/
	/*  --- Token-Texte, Token-Wert und dazugehörige Verarbeitungsfunktion (uBasic.c) --- */
	const T_KEYWORD_TOKEN keywords[] FLASHMEM = {
		#define TOKEN_WORD(name,text,func) { name, func, },
		#define TOKEN_CHAR(name,char)
		#define TOKEN_DUP(name,char)
			#include BASIC_CONFIG_FILE
		#undef TOKEN_WORD
		#undef TOKEN_CHAR
		#undef TOKEN_DUP
		{ TOKENIZER_ERROR, NULL, }
	};

#undef BASIC_TOKENS



/*  --- Lokale Variablen für Erweiterungen (Extensions) --- */
// ToDo : Einzelne Einblendungen als Typ zusammenfassen
#ifdef BASIC_USES_EXTENSIONS

	#define BASIC_USER_EXTENSION_INCLUDES
		#include BASIC_CONFIG_FILE
	#undef BASIC_USER_EXTENSION_INCLUDES
	// Sektion BASIC_USER_EXTENSIONS einblenden
	#define BASIC_USER_EXTENSIONS

		int user_dummy_var = 0;

		/*  --- Keyword-Texte für C-Funktionen/Variablen/Arrays erzeugen --- */
	  // Dummy-Variable für Extensions 
/*
		const char uservar_dummy [] FLASHMEM = "dummy";
		#define USER_FUNCTION(name,text,func,param)	const char userfunc__##name [] FLASHMEM  = { text };
		#define USER_VARIABLE(name,text,var,size)				const char uservar__##name  [] FLASHMEM  = { text };
		#define USER_ARRAY(name,text,var,size)			const char userarr__##name  [] FLASHMEM  = { text };	
			#include BASIC_CONFIG_FILE
		#undef USER_FUNCTION
		#undef USER_VARIABLE
		#undef USER_ARRAY
*/
		/*  --- Keyword-Liste für C-Funktionen/Variablen/Arrays erzeugen --- */
/*		const char * const userkeywords [] FLASHMEM  = {
			uservar_dummy, 
			#define USER_FUNCTION(name,text,func,param)	userfunc__##name,
			#define USER_VARIABLE(name,text,var,size)				uservar__##name, 
			#define USER_ARRAY(name,text,var,size)			userarr__##name, 	
				#include BASIC_CONFIG_FILE
			#undef USER_FUNCTION
			#undef USER_VARIABLE
			#undef USER_ARRAY
		};
*/
		/*  --- Zeiger Liste für C-Funktionen/Variablen/Arrays erzeugen --- */
		const void * const userpointer [] FLASHMEM  = {
			[0] = &user_dummy_var,
			#define USER_FUNCTION(name,text,func,param)	[name] = (void *) func,	
			#define USER_VARIABLE(name,text,var,type)	[name] = (void *) &var,
			#define USER_ARRAY(name,text,var,type,size)	[name] = (void *) &var,
				#include BASIC_CONFIG_FILE
			#undef USER_FUNCTION
			#undef USER_VARIABLE
			#undef USER_ARRAY
		};

		/*  --- Typ Liste für C-Funktionen/Variablen/Arrays erzeugen --- */
		const char usertypes [] FLASHMEM  = {
			[0] = 2,
			#define USER_FUNCTION(name,text,func,param)	[name] = 0x10,
			#define USER_VARIABLE(name,text,var,type)	[name] = (0x20+type),
			#define USER_ARRAY(name,text,var,type,size)	[name] = (0x30+type),
				#include BASIC_CONFIG_FILE
			#undef USER_FUNCTION
			#undef USER_VARIABLE
			#undef USER_ARRAY
		};

		/*  --- zus. Daten für C-Funktionen/Variablen/Arrays erzeugen  --- */
		const int userdatas [] FLASHMEM  = {
			[0] = 0,
			#define USER_FUNCTION(name,text,func,param)		[name] = param,
			#define USER_VARIABLE(name,text,var,type)		[name] = type,
			#define USER_ARRAY(name,text,var,type,size)		[name] = size,
				#include BASIC_CONFIG_FILE
			#undef USER_FUNCTION
			#undef USER_VARIABLE
			#undef USER_ARRAY
		};

	#undef BASIC_USER_EXTENSIONS

#endif



/*  --- Fehlertexte für den gesamten Komplex (Tokenizer/Interpreter/Compiler) --- */
#ifdef BASIC_USES_ERROR_TEXTS


	// Text einzeln erzeugen
	const char errortext__unknown_error [] FLASHMEM = { "unknown error" };
	#define BASIC_ERROR(name,text) const char errortext__##name [] FLASHMEM = { text };
		#include BASIC_CONFIG_FILE
	#undef BASIC_ERROR


	// Texte als Liste von char * aufbauen
	const char * const errortext_list [] FLASHMEM = {
		#define BASIC_ERROR(name,text)		errortext__##name,
			#include BASIC_CONFIG_FILE
		#undef BASIC_ERROR
	};

	// Gewünsche Fehlernummer als Text holen (Zeiger auf FLASH-Speicher wenn AVR)
	char *tokenizer_geterror_text (U8 error)
	{
		if (error >= TOKENIZER_NUMBER_ERRORS)
		{
			return (char *) errortext__unknown_error;
		}
		return pgm_read_ptr (&(errortext_list [error]));
	}


#else

	// Wenn Texte nicht übersetzt werden, immer Leer-String zurückgeben

 	const char errortext__none [] FLASHMEM = { "" };

	char *tokenizer_geterror_text (U8 error)
	{
		return errortext__none;
	}

#endif





// Wird das Zeichengerät verwendet so werden die elementaren Zugriffe
// (hole zeichen, nächstes zeichen, hole/setze Position) über Funktionszeiger und Makros gemappt
// Die letztendendes definierten Makros stellen dann die Elementarzugriffe dar.
#ifdef BASIC_USES_CHARDEVICE

	const T_CHAR_DEVICE astCharDevices [] FLASHMEM = {
		#define TOKENIZER_MEDIUM(name,st,cc,nc,gp,sp) { st, cc, nc, gp, sp, }, 
			#include BASIC_CONFIG_FILE
		#undef TOKENIZER_MEDIUM
	};


	/* --- Makros zum Aufruf der Funktion via Funktionszeiger --- */
	#define tokenizer_settext(x,l)	pfSetText(x,l)
	#define tokenizer_getchar()			pfGetChar()
	#define tokenizer_nextchar()		pfNextChar()
	#define tokenizer_startpos()		pfSetRPos((CPOS) 0)
	#define tokenizer_setrelpos(x)	pfSetRPos(x)
	#define tokenizer_getrelpos()		pfGetRPos()

	/*  --- Setzen der Funktionszeiger für Zeichengerät  --- */
	void  tokenizer_chardevice    (U8 device)
	{
	  if (device < TOKENIZER_NUM_CHARDEVICES)
		{
		  pfSetText   = (T_SET_TEXT ) pgm_read_funcptr (&(astCharDevices [device].pfSetText ));
			pfGetChar		= (T_WORK_CHAR) pgm_read_funcptr (&(astCharDevices [device].pfWorkChar));
			pfNextChar	= (T_NEXT_CHAR) pgm_read_funcptr (&(astCharDevices [device].pfNextChar));
			pfSetRPos 	= (T_SET_RPOS ) pgm_read_funcptr (&(astCharDevices [device].pfSetRPos ));
			pfGetRPos		= (T_GET_RPOS ) pgm_read_funcptr (&(astCharDevices [device].pfGetRPos ));
		}
	}

#else



	/* --- Zeichengerät "RAM" (wenn die Zeichengeräte-Funktionalität nicht aktiviert ist --- */

	/* --- Anpassungsmakros für WIN32/AVR (32/16Bit-Zeigerlänge)  --- */
	#ifdef WIN32
		#define PTRTYPE void *
	#endif
	#ifdef AVR
		#define PTRTYPE unsigned int
	#endif

	/* --- Makros zum direkten Zugriff auf die Zeiger --- */
	// Mit diesem Trick werden die Elementaroperationen direkt
	// in Zeigeroperationen auf dem SRAM umgesetzt. Dadurch
	// wird der Geschwindigkeitsvorteil enorm, gegenüber 
	// der über Funktionszeiger abstrahierten Implementierung.
	// Zudem sind die selben Elementarfunktionsmakros definiert
	// wie auch bei einem Zeichengerät, sprich die ganze
	// Zeichen-Quelle ist transparent aufgebaut und der Tokenizer
	// bedient sich nur dieser 6 Makros.
	#define tokenizer_settext(x,l)  ctx->charptr = ctx->charptrbak = (char *) x
	#define tokenizer_getchar()			*(ctx->charptr)
	#define tokenizer_nextchar()		ctx->charptr++
	#define tokenizer_startpos()		ctx->charptr = ctx->charptrbak;
	#define tokenizer_setrelpos(x)	ctx->charptr = &(ctx->charptrbak [(unsigned int) (x)])
	#define tokenizer_getrelpos()		(CPOS) ((PTRTYPE) (ctx->charptr - ctx->charptrbak))


#endif


/*  --- Text Setzen für alle Modi & Zeichengeräte (Makro aufrufen)  --- */
void tokenizer_set_text(void *program, CPOS length)
{
	tokenizer_settext(program, length);
}


/* --- strncmp_P mit Elementarfunktionen  --- */
char tokenizer_strncmp_P(char *string, int len)
{
	CPOS pos = tokenizer_getrelpos();
	char c1, c2;
	for (; len > 0; len --)
	{
		c1 = tokenizer_getchar(); tokenizer_nextchar();
		c2 = pgm_read_byte (string); string ++;
		if (c1 > c2) { tokenizer_setrelpos(pos); return  1; }
		if (c1 < c2) { tokenizer_setrelpos(pos); return -1; }
	}
	return 0;
}

/* --- Ext. Zugriff auf "Setze Zeiger auf Anfang des Codes" --- */
void  tokenizer_start_pos(void)
{
	tokenizer_startpos();
}

/* --- Ext. Zugriff auf "Setze Zeiger relativ zum Anfang des Codes"  --- */
void  tokenizer_set_rel_pos(CPOS stPos)
{
	tokenizer_setrelpos(stPos);
}

/* --- Ext. Zugriff auf "Hole Zeiger relativ zum Anfang des Codes"  --- */
CPOS  tokenizer_get_rel_pos(void)
{
	return tokenizer_getrelpos();
}







/*  --- Funktionen für die Extensions --- */
// Hier wird nach den Bezeichnern der Erweiterungen 
// gesucht und bei Fund die entsprechenden Daten
// (Zeiger, Anzahl Parameter, Index innerhalb des Arrays,
// Typ) bereitgestellt
#ifdef BASIC_USES_EXTENSIONS


	/*  --- Liefert die Daten zum aktuellen Extension Aufruf --- */
	TOKEN tokenizer_userdata(U8 *type, U16 *data, void **ptr)
	{
		*type = ctx->userkeywordtype;
		*data = ctx->userkeyworddata;
		*ptr  = ctx->userkeywordptr;
		return ctx->userkeywordindex;
	}

	/*  --- Liefert den Index des Extension Aufrufs --- */
	TOKEN tokenizer_userdata_idx(void)
	{
		return ctx->userkeywordindex;
	}
#endif


/*  --- Führe Basic-Keyword-Funktion aus --- */
void tokenizer_exec_token_func (void)
{
	unsigned char temp;  // Index für Funktionszegerarray
	T_BASIC_FUNCTION func;
	if ((ctx->current_token >= TOKENIZER_KEYWORDS) && (ctx->current_token < TOKENIZER_KEYCHARS))
	{
		// Index berechnen
	  temp = ctx->current_token - TOKENIZER_KEYWORDS - 1;
		// Funktion holen
	  func = (T_BASIC_FUNCTION) pgm_read_ptr (&(keywords [temp].func));
		// Ausführen
	  func();
	}
}



/*  --- auf Einzelzeichen-Token prüfen --- */
/*
TOKEN singlechar(void)
{
	char x = tokenizer_getchar ();
	#define BASIC_TOKENS
																				if (0) { }
		#define TOKEN_WORD(name,text,func)
		#define TOKEN_CHAR(name,char)				else if (x == char) 	return name;
		#define TOKEN_DUP(name,char)				else if (x == char) 	return name;
			#include BASIC_CONFIG_FILE
		#undef TOKEN_CHAR
		#undef TOKEN_DUP
		#undef TOKEN_WORD
																				else 										return 0;
	#undef BASIC_TOKENS
}
*/

/*  --- Token-Funktionen für Compiler-Funktionalität --- */
	/*  --- Byte aus Stream holen --- */
	unsigned char get_token_byte(void)
	{
		unsigned char temp = tokenizer_getchar();
		tokenizer_nextchar();
		return temp;
	}


	/*  --- Word aus Stream holen --- */
	unsigned short get_token_word(void)
	{
		unsigned short temp = (unsigned char) tokenizer_getchar();
		tokenizer_nextchar();
		temp = temp | ((unsigned short) tokenizer_getchar() << 8);
		tokenizer_nextchar();
		return temp;
	}


	/*  --- String aus Stream holen und nach destbuffer kopieren --- */
	void get_token_string(char *destbuffer)
	{
		unsigned char len = get_token_byte();

		for (; len > 0; len --)
		{
			*destbuffer++ = get_token_byte();
		}
		*destbuffer = 0;
	}


	/*  --- Selbe Funktion wie get_token_from_text nur aus dem Binär-Stream (also vorcompiliert) --- */
	TOKEN get_next_token_from_memory (void)
	{
	// für die Extensions brauchen wir den Index
	#ifdef BASIC_USES_EXTENSIONS
		unsigned char c;  
	#endif
		char tok;
		// Temporäre Variable für Word-Operationen (Zahlen und Zeiger (64kByte !))
		short word;

		// aktuelles Token holen
		tok = tokenizer_getchar();
		
		// Bit 6 mimics a line end token aka CR
		if (tok&(1<<6))
		{
			if (ctx->current_token!=TOKENIZER_CR)
			{
				return TOKENIZER_CR;
			}
		}
		
		tok = get_token_byte();
		ctx->tokendata [0] = tok & 0x3F;
		
		// Variables
		if (ctx->tokendata[0]>=TOKENIZER_NUM_TOKENS)
		{
			ctx->tokendata[2] = -1;
			ctx->tokendata[1] = 64-ctx->tokendata[0];
			ctx->tokendata[0] = TOKENIZER_VARIABLE;
			// Check if there is bitpos included
			if (tok&(1<<7))
			{
				ctx->tokendata[2] = get_token_byte();
			}
		}
		else
		{
			// je nach Token unterschiedliche Daten holen/aufbereiten
			switch (ctx->tokendata [0])
			{
				// Zahl 8/16 Bit holen (als Operand, Zeilennummer oder Zeiger)
				case TOKENIZER_NUMBER		:
					if (tok&(1<<7))
						word = get_token_word ();
					else
						word = get_token_byte();
				
					memcpy (&(ctx->tokendata [1]), &word, 2);
					break;
		  // bei eingeschalteten Extensions spart man sich ja
				// das searchkeywords. Dementsprechend ist nur der Index von Interesse
				// und es werden hier automatisch die entprechenden Extension-Daten
				// bereitgestellt.
		#ifdef BASIC_USES_EXTENSIONS
				case TOKENIZER_NAME			:
					ctx->tokendata [1] = get_token_byte ();
					memcpy (&c, &(ctx->tokendata [1]), 1);
					ctx->userkeywordindex 	= c;
					ctx->userkeyworddata 	= pgm_read_byte (&(userdatas 	[c]));
					ctx->userkeywordtype 	= pgm_read_byte (&(usertypes 	[c]));
					ctx->userkeywordptr  	= pgm_read_ptr  (&(userpointer [c]));
					ctx->tokendata[2] = -1;
					// Check if there is bitpos included
					if (tok&(1<<7))
					{
						ctx->tokendata[2] = get_token_byte();
					}
					break;
		#endif
				// 
				case TOKENIZER_VARIABLE :
					ctx->tokendata [1] = get_token_byte ();
					ctx->tokendata[2] = -1;
					// Check if there is bitpos included
					if (tok&(1<<7))
					{
						ctx->tokendata[2] = ctx->tokendata[1]>>5;
					}
					ctx->tokendata[1] &= 0x1F;
					break;
				case TOKENIZER_STRING		:
					get_token_string (&(ctx->tokendata [1]));
					break;
				// goto und gosub benötigen hier "extra-behandlung" (siehe uBasic.c)
				case TOKENIZER_GOTO  		:
				case TOKENIZER_GOSUB 		:
					if (tok&(1<<7))
						ctx->gototokenpos = get_token_word ();
					else
						ctx->gototokenpos = get_token_byte();
				
					memcpy (&(ctx->tokendata [1]), &(ctx->gototokenpos), 2);
					break;
			}
		}
		return ctx->tokendata [0];
	}

/*  --- Hauptfunktion zur Bestimmung des nächsten Tokens (beide Modi) --- */
TOKEN get_next_token (void)
{
	// vorcompiliert
	return get_next_token_from_memory();
}


/*  ---  Setze Zeiger auf Position und hole aktuelles Token --- */
void tokenizer_jump_to_pos (CPOS jumptodest)
{
	tokenizer_setrelpos ((CPOS) jumptodest);
	tokenizer_next();
}


void tokenizer_init(void)
{
	ctx->current_token = get_next_token();
}


/*  --- holt das aktuelle Token --- */
TOKEN tokenizer_token(void)
{
	return ctx->current_token;
}


/*  --- holt das nächste Token --- */
void tokenizer_next(void)
{
	if(tokenizer_finished())
	{
		return;
	}
	
	ctx->current_token = get_next_token();
	return;
}


/*  --- Sind wir am Ende des Textes/Codes ? --- */
char tokenizer_finished(void)
{
	// Im Compiled Mode ist das Ende mit TOKENIZER_ENDOFINPUT gekennzeichnet
	return ctx->current_token == TOKENIZER_ENDOFINPUT;
}


/*  --- holt eine zuvor erkannte Zahl (TOKEN_NUMBER) --- */
int tokenizer_num(void)
{
	short temp;
	memcpy (&temp, &(ctx->tokendata [1]), 2);
	return (int) temp;
}


/*  --- holt einen zuvor erkannten String (TOKEN_STRING) --- */
void tokenizer_string(char *dest, char len)
{
	if(tokenizer_token() != TOKENIZER_STRING)
	{
		return;
	}
  
	if ((U8) len > (U8) strlen(&(ctx->tokendata [1])))
	{
		len = (I8) strlen(&(ctx->tokendata [1])) + 1;
	}
	
	strncpy(dest, &(ctx->tokendata [1]), len);
}



/*  --- holt eine zuvor erkannte Variable (TOKEN_STRING) ---*/
char tokenizer_variable_num(void)
{
	return ctx->tokendata[1];
}

char tokenizer_variable_bitpos(void)
{
	return ctx->tokendata[2];
}


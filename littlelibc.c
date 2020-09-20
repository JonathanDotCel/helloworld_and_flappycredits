// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "littlelibc.h"
#include <stdarg.h> 

// Based on: https://github.com/grumpycoders/uC-sdk/blob/master/os/src/osdebug.c

// TODO: remove the "New" prefix. That'll get old quick.

// This is part of the standard C lib, right?
#define CHAR_PERCENT 0x25



void NewPrintf( const char * str, ... ){
	
	register int cmd __asm__("$9") = 0x3F;
	__asm__ volatile("" : "=r"(cmd) : "r"(cmd));
	((void(*)(const char*))0xA0)( str );
	

}

void NewStrcpy( char * dst, const char * src ){
	
	while( *src != 0 ){
		*dst++ = *src++;
	}

}

void NewMemcpy( char * dst, const char * src, ulong len ){
	int i = 0;
	for( i = 0; i < len; i++ ){
		*dst++ = *src++;		
	}

}

int NewStrncmp( const char * paramA, const char * paramB, ulong len ){
        
        ulong countyCount = 0;

        while( !(*paramA == 0 && *paramB == 0 ) && countyCount++ < len ){
                if ( *paramA++ != *paramB++ ){
                        return ( *paramA > *paramB ? 1 : -1 );
                }               
        }
        return 0;
}


int NewStrcmp( const char * paramA, const char * paramB ){
        
        ulong countyCount = 0;

        while( !(*paramA == 0 && *paramB == 0 ) && countyCount++ < 1024 ){
                if ( *paramA++ != *paramB++ ){
                        return ( *paramA > *paramB ? 1 : -1 );
                }               
        }
        return 0;
}


static const char charTable[] = "0123456789ABCDEF";


void NewSPrintf( char * out, const char * in, ... ){
	
	va_list list;

	char * writeHead;
	const char * readHead;
	
	// shared args
	unsigned long arg_u = 0;
	long arg_i;

	// temp buffer
	char conversionBuffer[33], * conversionPointer;
	ulong charsGenerated = 0;  // quicker than strlen

	// i =)
	int i = 0;
	
	va_start( list, in );

	writeHead = out;
	readHead = in;

	while( 1 ){

		char argType = 0;
		char readVal = *readHead++;

		if ( readVal == 0 )
			return;

		// next char will determine type
		if ( readVal != CHAR_PERCENT ){
			*writeHead++ = readVal;			
			continue;
		}

		// else read the next char
		argType = *readHead++;
				
		switch( argType ){

			// write an actual percent sign
			case '%':
				*writeHead++ = CHAR_PERCENT;				
			break;
			
			// chaaaar
			case 'c':{

				char c = va_arg(list, ulong);
				*writeHead++ = c;
			}break;

			// string
			case 's':{
				
				char * stringyString = (char*)va_arg(list, ulong*);
				
				while ( *stringyString != 0 ){
					*writeHead++ = *stringyString++;
				}
			}break;

			// Note: i & d drops through to u
			case 'i':
			case 'd':
				
				arg_i = va_arg(list, long);

				if (arg_i < 0) {
					*writeHead++ = '-';
					arg_u = -arg_i;
				} else {
					arg_u = arg_i;
				}
			case 'u':
				
				if ( argType == 'u' )
					arg_u = va_arg( list, unsigned long);
					
				conversionPointer = conversionBuffer + 32;
				*conversionPointer = 0;
				charsGenerated = 0;
				
				do {
					*--conversionPointer = charTable[arg_u % 10];
					arg_u /= 10;	
					charsGenerated++;
				} while (arg_u);
				
				// chars generated = 12 - ( tempWriteHead - writeHead )
				NewMemcpy( writeHead, conversionPointer, charsGenerated );

				writeHead += charsGenerated;
				break;

			case 'p':
				arg_u = va_arg(list, ulong);
				//*writeHead++ = '0';
				//*writeHead++ = 'x';

				for (i = sizeof(arg_u) * 2 - 1; i >= 0; i--) {
					*writeHead++ = charTable[(arg_u >> (i << 2)) & 15];
				}
				break;

			case 'x':
				
				arg_u = va_arg(list, unsigned long);
				charsGenerated = 0;

				for (i = sizeof(arg_u) * 2 - 1; i >= 0; i--) {
					if (!charsGenerated && ((arg_u >> (i << 2)) == 0))
						continue;
					
					*writeHead++ = charTable[(arg_u >> (i << 2)) & 15];
					charsGenerated = 1;
				}

				if (!charsGenerated){
					*writeHead++ = '0';			
				}

				break;


			// %02x for the hex editor
			case '0':
				
				if ( *readHead++ != '2' ) break;
				if ( *readHead++ != 'x' ) break;

				arg_u = va_arg(list, unsigned long);
				
				for (i = sizeof(char) * 2 - 1; i >= 0; i--) {					
					*writeHead++ = charTable[(arg_u >> (i << 2)) & 15];					
				}

				break;
				

		}
		

	}

	*writeHead = 0; // zero out the (potentially) final char.

}
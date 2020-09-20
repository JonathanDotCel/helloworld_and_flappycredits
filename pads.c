// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include <stdlib.h>
#include "littlelibc.h"
#include "pads.h"


// might need this:
// http://problemkaputt.de/psx-spx.htm

// 0x10 bytes into the scratch pad
// anywhere's good, really.
#define PADBUFFER 0x1F800010

#define pPADBUFFER *(volatile short*)(PADBUFFER)
#define PADCONST 0x20000001

ulong padCounter = 0;
char padsInstalled = 0;

ulong padVals = 0;
ulong lastPadVals = 0;


// NOTE: remember the extra 8 bytes of stack

unsigned long GetPadVals(){
	return padVals;
}

#pragma GCC push options
#pragma GCC optimize ("-O0")
void PadStart(){
	
	// Just telling the asm template that sp ($29) and ra ($31)
	// are clobbered won't force it to suddenly use the stack
	// to protect ra, so we have to handle that.
		
	__asm__ volatile (
		
		"addiu $29, $29, -0x04\n\t"		// addiu sp, sp, -$4
		"sw	$31,0($29)\n\t"				// sw ra,0(sp)
				
		"addiu $10,$0,0xB0\n\t"			// li t2, $B0 ; (B0 table)
		"addiu $9,$0,0x13\n\t"			// li t1, $13 ; (PadStart)
		"jalr $10\n\t"					// jalr t2
		"nop\n\t"						// nop :)
		
		"lw	$31,0($29)\n\t"				// lw ra,0(sp)
		"nop\n\t"						
		"addiu $29, $29, 0x04\n\t"		// addiu sp, sp, $4
		"nop\n\t"
		:
		:		
	);

}

// "This is an extremely bizarre and restrictive function - don't use!"
//   -nocash

void PadInit( ulong inConst, ulong inAddr ){
	
	// Same deal as PadStart but needs 4 extra stack slots
	// (and another for ra)

	__asm__ volatile (
		
		"addiu $29, $29, -0x14\n\t"		// addiu sp, sp, -$14
		"sw	$31,0($29)\n\t"				// sw ra,0(sp)
				
		"addu $4,$0,%0\n\t"				// move a0, inConst
		"addu $5,$0,%1\n\t"				// move a1, inAddr
		"addu $6,$0,$0\n\t"				// move a2, 0 (unused)
		"addu $7,$0,$0\n\t"				// move a3, 0 (unused)
		"addiu $10,$0,0xB0\n\t"			// li	t2, $B0 ; (B0 table)
		"addiu $9,$0,0x15\n\t"			// li	t1, $13 ; (PadStart)
		"jalr $10\n\t"					// jalr t2
		"nop\n\t"						// nop
		
		"lw	$31,0($29)\n\t"				// lw ra,0(sp)
		"nop\n\t"
		"addiu $29, $29, 0x14\n\t"		// addiu sp, sp, -$14
		"nop\n\t"

		: 
		: "r" ( inConst ), "r" ( inAddr )		
	);

}

void PadStop(){
	
	padsInstalled = 0;
	
	// See PadStart() and read "Start" as "Stop"

	__asm__ volatile (

		"addiu $29, $29, -0x04\n\t"		// addiu sp, sp, -$4
		"sw	$31,0($29)\n\t"				// sw ra,0(sp)

		"addiu $10,$0,0xB0\n\t"
		"addiu $9,$0,0x14\n\t"
		"jalr $10\n\t"
		"nop\n\t"

		"lw	$31,0($29)\n\t"				// lw ra,0(sp)
		"nop\n\t"						
		"addiu $29, $29, 0x04\n\t"		// addiu sp, sp, $4
		"nop\n\t"
		:
		:
	);

}
#pragma GCC pop options


void InitPads(){
	
	padVals = 0;
	lastPadVals = 0;
	pPADBUFFER = 0xFFFF;

	
	PadStart();	
	PadInit( PADCONST, PADBUFFER );
	
	// Do it after incase there's an int or whatever
	padsInstalled = 1;
	
}


// Should you want to use it as a quick n easy vsync...
// lol but don't
void PadsVsync(){

	if ( !padsInstalled )		
		InitPads();

	// buffer's inverted so 0xFFFFFFFF = nothing pressed
	// we'll set it to "everything pressed" and wait.
	pPADBUFFER = 0;
	padCounter = 0;


	
	while ( pPADBUFFER == 0 ){
		
		padCounter++;
		if ( padCounter > 1000000 ){
			// unset "everything pressed" if we bail
			pPADBUFFER = 0xFFFF;
			padVals = 0;
			return;
		}
	}


}


// Record pad state to determine press/unpress
void MonitorPads(){
	
	lastPadVals = padVals;
	padVals = ~pPADBUFFER;

}

int AnythingPressed(){
	if ( lastPadVals != 0 && padVals == 0 ){
		//clear these so it doesn't send the key to the next menu
		padVals = 0;
		lastPadVals = 0;
		return 1;
	}
	return 0;
}

int Held( ulong inButton ){	
	return ( lastPadVals & inButton ) != 0;
}

// Was any button pressed/released this frame?
int Released( ulong inButton ){

	int returnVal;
	returnVal = !( padVals & inButton ) && ( lastPadVals & inButton );

	if ( returnVal ){
		// clear padVals so the next menu/item/prompt doesn't auto get the new value
		// leave other buttons though, incase we're using shoulder buttons to accelerate
		// menu options, etc
		lastPadVals ^= inButton;		
	}

	return returnVal;

}

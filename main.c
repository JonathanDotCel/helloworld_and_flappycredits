

// 
// GPL BEER LICENSE.
// It's the same as the GPL3 with some additions:
// - if you fork it you must send me a beer and leave the original headers
// - Give a nice clear credit if you use any part of the code elsewhere!
// - only add credits; never remove.
// - change the name a bit if you fork: "Unirom NX", etc. Has to sound cool.
// 
// 
// Standard kinda fare though, no explicit or implied warranty.
// Any issues arising from the use of this software are your own.
// 
// https://github.com/JonathanDotCel
//

#ifndef  MAINC
#define MAINC


// variadic logging
#include <stdarg.h>
#include "main.h"

//
// Includes
//

#include "littlelibc.h"
#include "utility.h"
#include "drawing.h"
#include "pads.h"
#include "ttyredirect.h"
#include "config.h"
#include "flappycredits.h"
#include "gpu.h"

void DoStuff();

int main(){
	
	ResetEntryInt();
	ExitCritical();

	// Clear the text buffer
	InitBuffer();
	
	// Init the pads after the GPU so there are no active
	// ints and we get one frame of visual confirmation.
	
	NewPrintf( "Init GPU...\n" );
	InitGPU();

	NewPrintf( "Init Pads...\n" );
	InitPads();

	InstallTTY();
	NewPrintf( "You can now use NewPrintf() functions!\n" );	

	// Main loop
	while ( 1 ){
		
		MonitorPads();
		
		ClearScreenText();
		
		C64Border();

		
		
		Blah( "\n\n\n\n\n\n\n\n\n                Hello world!     -     Frame %d\n\n\n", GetFrameCount() );
		Blah( "        Dpad  : move block\n" );
		Blah( "        X     : flappy credits\n" );
		Blah( "        O     : reboot\n" );
		Blah( "        R4    : debug\n" );  // there's no r4
		
		DoStuff();

		Draw();
		
	}
	
}


short blockX = SCREEN_WIDTH / 2;
short blockY = SCREEN_HEIGHT / 2;

void DoStuff(){


	if ( Held( PADLup ) ){
		blockY = ( blockY - 1 ) % SCREEN_HEIGHT;
	}
	if ( Held( PADLdown ) ){
		blockY = ( blockY + 1 ) % SCREEN_HEIGHT;
	}
	if ( Held( PADLleft ) ){
		blockX = ( blockX - 1 ) % SCREEN_WIDTH;
	}
	if ( Held( PADLright ) ){
		blockX = ( blockX + 1 ) % SCREEN_WIDTH;
	}

	if ( Released( PADRright ) ){
		// restart via bios
		goto *(ulong*)0xBFC00000;
	}

	if ( Released( PADRdown ) ){
		// restart via bios
		FlappyCredits();
	}

	DrawTile( blockX, blockY, 40, 40, 0xFF8822 );
	
}


#endif // ! MAINC


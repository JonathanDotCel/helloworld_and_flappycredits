// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
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
#include "timloader.h"

//
// Protos
//

void DoStuff();

//
// TIM and Sprite data
//

// Seamonstah
extern ulong lobster_tim;
extern ulong octo_happy_tim;
extern ulong octo_angery_tim;
TIMData lobster;
TIMData happy;
TIMData angery;

#define NUMSPRITES 20
Sprite critterSprites[NUMSPRITES];

// out of 1000
ulong springiness = 830;


int main(){
	
	//ResetEntryInt();
	ExitCritical();

	// Clear the text buffer
	InitBuffer();
	
	// Init the pads after the GPU so there are no active
	// interrupts and we get one frame of visual confirmation.
	
	NewPrintf( "Init GPU...\n" );
	InitGPU();

	NewPrintf( "Init Pads...\n" );
	InitPads();

    // Enable this if you're not inheriting a TTY redirect device from Unirom, n00bROM, etc
	//InstallTTY();
	//NewPrintf( "You can now use NewPrintf() functions!\n" );	

    // Upload the sprites to VRAM
    // we can use the position define in the TIM header
    // or (as here) manually specify

    // Just an example of how you can lay out the CLUT (pallete) data and the actual pixels in VRAM
    // Here the CLUTs are at 512,0 stacked vertically down
    // The actual images are on the next 64px texture page after that, randomly as an example
    // e.g. lobster at 576x16, happy is to the right of lobster, angery is below happy. (L shape)

    // visual learners: https://i.imgur.com/aKbwsL7.png
    // Icons courtesy of icons8.com

    UploadTim( (char*)&lobster_tim, &lobster, SCREEN_WIDTH + TEXPAGE_WIDTH, 1, SCREEN_WIDTH + TEXPAGE_WIDTH, 16 );
    // to the right
    UploadTim( (char*)&octo_happy_tim, &happy, SCREEN_WIDTH + TEXPAGE_WIDTH , 2, lobster.vramX+lobster.vramWidth *2 , lobster.vramY );        
    // below    
    UploadTim( (char*)&octo_angery_tim, &angery, SCREEN_WIDTH + TEXPAGE_WIDTH, 3, happy.vramX, happy.vramY + happy.vramHeight );        
    
    // now make some sprites, and link them to the locations we just set up in VRAM
    
    for( int i = 0; i < NUMSPRITES; i++ ){

        Sprite * s = &critterSprites[i];
        switch (i%3)
        {
            default: s->data = &lobster; break;
            case 1: s->data = &happy; break;
            case 2: s->data = &angery; break;
        }        
        s->xPos = i + (20 * i);
        s->yPos = 30;
        s->height = 40 - i;
        s->width = 40 - i;

    }


	// Main loop
	while ( 1 ){
		
		MonitorPads();
		
		ClearScreenText();
		
		C64Border();
        
		Blah( "\n\n\n\n\n\n\n\n\n                Hello world!     -     Frame %d\n\n\n", GetFrameCount() );
		Blah( "        Dpad  : move block\n" );
		Blah( "        X     : flappy credits\n" );
		Blah( "        O     : reboot\n" );
        Blah( "        L1/R1 : springiness %d of 1000\n", springiness );
		Blah( "        R4    : debug\n" );  // there's no r4
		
		DoStuff();

		Draw();
		
	}
	
}


static short cursorX = SCREEN_WIDTH / 2;
static short cursorY = SCREEN_HEIGHT / 2;


void DoStuff(){

    

	if ( Held( PADLup ) ){
		cursorY = ( cursorY - 2 ) % SCREEN_HEIGHT;
	}
	if ( Held( PADLdown ) ){
		cursorY = ( cursorY + 2 ) % SCREEN_HEIGHT;
	}
	if ( Held( PADLleft ) ){
		cursorX = ( cursorX - 2 ) % SCREEN_WIDTH;
	}
	if ( Held( PADLright ) ){
		cursorX = ( cursorX + 2 ) % SCREEN_WIDTH;
	}

	if ( Released( PADRright ) ){
		// restart via bios
		goto *(ulong*)0xBFC00000;
	}

	if ( Released( PADRdown ) ){
		// restart via bios
		FlappyCredits();
	}
    
    if ( Released( PADL1 ) ){
        springiness -= 5;        
    }

    if ( Released( PADR1 ) ){
        springiness += 5;        
    }

    // Daw the 'cursor'
	DrawTile( cursorX , cursorY, 20, 20, 0xFF8822 );


    // at its most basic...
    //DrawTIMData( &lobster, 20, 20, 28, 28 );
    //DrawSprite( &critterSprites[0] );
    //DrawSprite( &critterSprites[1] );
    //DrawSprite( &critterSprites[2] );
    
    
    
    for( int i = 0; i < NUMSPRITES ; i++ ){
        
        // work out what this sprite is chasing..
        int targetX = (i == 0) ? cursorX : (critterSprites[ i-1 ].xPos + critterSprites[ i-1 ].width /2 - critterSprites[i].width /2 );
        int targetY = (i == 0) ? cursorY : (critterSprites[ i-1 ].yPos + critterSprites[ i-1 ].height /2 - critterSprites[i].width /2 );

        // how far are we from the target?
        int deltaX = targetX - critterSprites[i].xPos;
        int deltaY = targetY - critterSprites[i].yPos;
 
        // e.g. if the gap is 45 pixels, this gives us 450,000
        critterSprites[i].shiftedVeloX += deltaX * 1000;  // e.g. 45 pix becomes 45k pixels
        critterSprites[i].shiftedVeloY += deltaY * 1000;

        #define CLAMP 50000
        // let's not use a clamp function just for the little demonstration
        if ( critterSprites[i].shiftedVeloX > CLAMP ) critterSprites[i].shiftedVeloX = CLAMP;
        if ( critterSprites[i].shiftedVeloX < -CLAMP ) critterSprites[i].shiftedVeloX = -CLAMP;
        if ( critterSprites[i].shiftedVeloY > CLAMP ) critterSprites[i].shiftedVeloY = CLAMP;
        if ( critterSprites[i].shiftedVeloY < -CLAMP ) critterSprites[i].shiftedVeloY = -CLAMP;

        // we can now divide by 10,000 which gives us 4 pixels
        // if we handn't multiplied, it would've rounded 45px / 100 down to 0
        critterSprites[i].xPos += critterSprites[i].shiftedVeloX / (1000 * 22);
        critterSprites[i].yPos += critterSprites[i].shiftedVeloY / (1000 * 22);

        DrawSprite( &critterSprites[i] );

        // dampen the velo
        // let's ignore frameate
        critterSprites[i].shiftedVeloX *= springiness;
        critterSprites[i].shiftedVeloY *= springiness;
        critterSprites[i].shiftedVeloX /= 1000;
        critterSprites[i].shiftedVeloY /= 1000;
        

    }
    

    
    	
}


#endif // ! MAINC


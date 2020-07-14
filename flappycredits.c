
//
// Doesn't account for PAL/NTSC timing differences
// but we're kinda used to that in PAL regions
// feel free to add a multiplier
// 


#include <stdlib.h>
#include "littlelibc.h"
#include "drawing.h"
#include "pads.h"


#define STATE_INTRO 0
#define STATE_PLAYING 1
#define STATE_OUTRO 2
#define STATE_DONE 3

// +Y is down
// ( screen space )

#define PLAYER_WIDTH 20
#define PLAYER_HEIGHT 20

#define PIPE_WIDTH 30

#define PIPE_HEIGHT_AVERAGE 60
#define PIPE_WIDTH_AVERAGE 30
#define PIPE_SPACING_AVERGE 140
#define NUMPIPES 6


// Hah, you think I was going to maintain 2 lists? Copy any changes to readme.md  / flappycredits.c
char *credits[] = { 
	"", "", "",
	"Doofy", "Nocash <3", "Shendo", "Type 79", "Dax", "Jihad / Hitmen", "Silpheed / Hitmen", "SquareSoft74 (no spaces)" ,
	"Foo Chen Hon", "Shadow / PSXDev", "Matthew Read (lol)", "DanHans / GlitterGirls", "Herben", "and asmblur", "JMiller", 
	"Tim S / Firefly", 	"rama (any version)",
	"Angus McFife XIII",
	"Padua", "Blackbag", "Napalm", "Paradox / Paradogs :p", "XPlorer Peeps", "K-Comms Peeps",
	"noisy assholes who recycle...", ".. bottles, one by fucking...", "... one",
	"barog", "L0ser", "cybdyn", "paul", "Peter Lemon", "and krom", "Brian Marshall", "Mistamotiel", "and Mistamontiel...", "tieigo", "orion",
	"Codeman", "Cat", "LordBlitter", "SurfSmurf", "kHn", "Nicolas Noble", "r0r0",
	"Everyone at PSXDev!", 
	"Tetley.co.uk", "And absolutely *not*...", "Lameguy64", "lol"	// lol just fucking about, he's helped loads
																	// And an extra special thanks to SquareSoft74, DanHans, Nicolas Noble and Rama who've been absolute fucking legends with their support and advice!
};

//
// Some sources which may or may not be of interest:
// http://www.psxdev.net/forum/viewtopic.php?f=66&t=3413&hilit=xflash&start=20
// https://github.com/jmiller656/PS1-Experimentation/blob/master/source/ImageDemo/main.c
// http://www.psxdev.net/downloads/Net%20Yaroze%20-%20Library%20Reference.pdf
// https://tcrf.net/Attack_Plarail
// https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/dev%20notes.txt
// https://patpend.net/technical/psx/romstuff.txt
// https://www.retroreversing.com/ps1-psylink
// https://www.retroreversing.com/psyq-sdk-setup
// https://www.retroreversing.com/ps1-psylink
// https://archive.org/stream/ninpcman/ninpcman_djvu.txt
//
//

// Just a rect but I dunno, might want to add stuff later
struct Pipe{

	int left;
	int top;

	int width;
	int height;

} pipes[] = {

	// alloc the memory, but we'll set the values later
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,

};


int cState = 0;
char pendingKey = 0;
char pendingFail = 0;

ulong posX = 0;
ulong posY = 0;

ulong pixelX = 0;
ulong pixelY = 0;

int velY = 0;
ulong randomSeed = 0xD00FD00F;

ulong frameCount = 0;
ulong score = 0;

char textState = 0;
char creditIndex = 0; // give it a few seconds

// Yeha, it's just a rect, but let's make room for expansion
void InitPipe( char whichOne, ulong inX, ulong inY, ulong inWidth, ulong inHeight ){
	
	pipes[ whichOne ].left = inX << 12;
	pipes[ whichOne ].top = inY << 12;
	pipes[ whichOne ].width = inWidth << 12;
	pipes[ whichOne ].height = inHeight << 12;
	
}

void Cleanup(){
	
	cState = STATE_INTRO;

	pendingKey = 0;
	pendingFail = 0;
	posX = 80 << 12;
	posY = ( SCREEN_HEIGHT /2 ) << 12;
	velY = 0;

	frameCount = 0;
	score = 0;
	
	textState = 0;

	InitPipe( 0, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 0 ), 0, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE - 40 );
	InitPipe( 1, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 0 ), SCREEN_HEIGHT - PIPE_HEIGHT_AVERAGE +8, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE - 8 );

	InitPipe( 2, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 1 ), 0, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE - 32 );
	InitPipe( 3, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 1 ), SCREEN_HEIGHT - PIPE_HEIGHT_AVERAGE - 32, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE + 32 );

	InitPipe( 4, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 2 ), 0, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE );
	InitPipe( 5, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 2 ), SCREEN_HEIGHT - PIPE_HEIGHT_AVERAGE, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE );

	InitPipe( 6, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 1 ), 0, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE - 20 );
	InitPipe( 7, ( SCREEN_WIDTH / 2 ) + ( PIPE_SPACING_AVERGE * 1 ), SCREEN_HEIGHT - PIPE_HEIGHT_AVERAGE - 20, PIPE_WIDTH_AVERAGE, PIPE_HEIGHT_AVERAGE + 20 );

}

int RandomRange( int inVal ){	
	return randomSeed % inVal;
}

void RandomisePipe( int i ){
	
	int range = 72;
	int rando = RandomRange( range );

	int aHeight = PIPE_HEIGHT_AVERAGE;

	// e.g. +30, -30
	rando -= range / 2 ;
	
	InitPipe( i, SCREEN_WIDTH, 0, PIPE_WIDTH_AVERAGE, aHeight + rando );
	InitPipe( i+1, SCREEN_WIDTH, SCREEN_HEIGHT - aHeight + rando, PIPE_WIDTH_AVERAGE, aHeight - rando );

}


char PointInsidePipe( int inX, int inY, struct Pipe inPipe ){
	
	// we'll give them that extra pixel...

	return ( inX > inPipe.left && inX < (inPipe.left + inPipe.width) )
		&& ( inY > inPipe.top && inY < (inPipe.top + inPipe.height) );
	
}

// not in screen/pixel space - everything's shifted 12 bits left
char CollideyWidey( int inX, int inY, struct Pipe inPipe ){
	
	// Not the most optimised code, but it's fucking flappy bird
	// and 100% would rather go for readability

	// check if topleft, topright, bottomleft, bottomright are inside the pipe
	int left = inX - (( PLAYER_WIDTH / 2 ) << 12 );
	int right = inX + (( PLAYER_WIDTH / 2 ) << 12);
	int top = inY - (( PLAYER_HEIGHT / 2 ) << 12);
	int bottom = inY + (( PLAYER_HEIGHT / 2 ) << 12);
	

	// I always get the urge to lay these out as if winding clockwise.
	if ( PointInsidePipe( left, top, inPipe ) ) return 1;
	if ( PointInsidePipe( right, top, inPipe ) ) return 1;
	if ( PointInsidePipe( right, bottom, inPipe ) ) return 1;
	if ( PointInsidePipe( left, bottom, inPipe ) ) return 1;

	return 0;

}

#define SCORESTRING "\n\n\n\n\n\n        Score %d\n"

void FlappyCredits(){
	
	int i = 0;
	ulong moveSpeed = 0;

	int numCredits = sizeof( credits ) / 4; // each string is a 32bit pointer to the source

	Cleanup();

	while( 1 ){

		randomSeed += velY >> 12;
		randomSeed += 5;

		ClearScreenText();
		MonitorPads();
		
		DrawBG();
		
		if ( cState == STATE_INTRO ){
			
			
			Blah( "\n\n\n\n\n\n        Hit X to start!\n" );

			if ( pendingKey ){				
				Cleanup();
				cState = STATE_PLAYING;
			}

		}
		
		if ( cState == STATE_PLAYING ){
			
			Blah( SCORESTRING, score );
			

			if ( pendingKey ){
				pendingKey = 0;

				if ( velY > 0 )
					velY = 0;				
				velY -=12000;

				randomSeed += 3;

			}
		
			velY += 1200;
			posY += velY;

			moveSpeed = ( 4 << 12 ) + ( score << 8 );

			//Blah( "MoveSpeed %d\n", moveSpeed >> 12 );

			for( i = 0; i < NUMPIPES; i++ ){

				// multiply first (shift) then divide to get smooth motion
				pipes[i].left -= moveSpeed;

				if ( i % 2 == 0  && pipes[i].left <= 0-pipes[i].width ){
					RandomisePipe( i );
					score += 1;
				}

				// Moved it to after display() so the next frame doesn't kill you before you have a chance
				/*
				if ( CollideyWidey( posX, posY, pipes[i] ) ){
					pendingFail = 2;	
					velY = -22000;
				}
				*/
				

			}


			if ( posY < 0 ){
				pendingFail = 1;
				posY = 0;
			}

			if ( posY > SCREEN_HEIGHT << 12 ){
				pendingFail = 1;
				posY = SCREEN_HEIGHT << 12;
			}

			if ( pendingFail ){
				cState = STATE_OUTRO;
			}


		}


		if ( cState == STATE_OUTRO ){
			
			Blah( SCORESTRING, score );
			
			// bounce it back to the bottom
			if ( pendingFail == 2 ){
				posX -= 600;
				velY += 1200;
				posY += velY;
			}

			if ( posY >= SCREEN_HEIGHT << 12 ){
				posY = SCREEN_HEIGHT << 12;
				pendingFail = 1;
				cState = STATE_DONE;
			}

		}

		if ( cState == STATE_DONE ){
			
			Blah( SCORESTRING, score );
			Blah( "\n\n            O = Retry\n" );

		}

		
		textState = ( frameCount % 100 ) < 90;
		if ( frameCount % 100 == 0 || Released( PADRleft ) )
			creditIndex = ( creditIndex + 1 ) % numCredits;

		
		Blah( "                             Thanks n greets:\n\n" );
		if ( textState != 0 && creditIndex > 0 )
			Blah( "                             %s\n", credits[ creditIndex ] );


		frameCount++;

		pixelX = posX >> 12;
		pixelY = posY >> 12;


		for( i = 0; i < NUMPIPES; i++ ){
			
			BorderTile( pipes[i].left >> 12, pipes[i].top >> 12, pipes[i].width >> 12, pipes[i].height >> 12 );			

			if ( cState == STATE_PLAYING && CollideyWidey( posX, posY, pipes[i] ) ){
				pendingFail = 2;	
				pendingFail = 2;	
				velY = -22000;
			}

		}


		Highlight( pixelX, pixelY, PLAYER_WIDTH, PLAYER_HEIGHT );

		// was designed for a much slower FPS
		// flaps per second
		
		Draw();
		
		
		if ( cState != STATE_OUTRO ){
			
			// Since it doesn't matter how long we press the key
			if ( Released( PADRdown ) ){
				pendingKey = 1;
			}
			
			if ( Released( PADRright ) ){
				if ( cState == STATE_DONE ){
					Cleanup();
				} else {
					return;
				}
			}


		} 


	}


} 

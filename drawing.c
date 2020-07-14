


#include "drawing.h"
#include "littlelibc.h"
#include "pads.h"
#include "main.h"
#include "gpu.h"
#include "utility.h"
#include "ttyredirect.h"
#include "hwregs.h"
#include "config.h"


// Little outline rectangles:
// Either a solid rect or 4 separate lines.
typedef struct Highlighter {

	// target left, right, bottom and top values
	int lTarg;
	int rTarg;
	int botTarg;
	int topTarg;

	// actual left, right bottom and top values
	int lAct;
	int rAct;
	int botAct;
	int topAct;

} highlighter;

highlighter h1;
highlighter h2;


typedef struct {
	ulong	tag;
	uchar	r0, g0, b0, code;
	short	x0, 	y0;
	short	w,	h;
} NTILE;


ulong frameCounter = 0;

// Tell the GPU to finish up drawing, draw the font, vsync, etc.
void Draw(){

	frameCounter++;

	EndDrawing();

}

ulong GetFrameCount(){
	return frameCounter;
}


// Wrapper for GPU drawTile
// -prevents flickering from weird positions
// -prevents spillover into the next buffer
// -converts RGB to BRG
void BorderTileColor( int inX, int inY, int inWidth, int inHeight, ulong inColor ){
	
	// if it goes off the left, make it thinner
	int outX = inX;
	int outWidth = inWidth;

	// same with the bottom?	
	int outY = inY;
	int outHeight = inHeight;

	ulong outColor = inColor;

	ulong r = 0;
	ulong b = 0;

	if ( inX < 0 ){
		outWidth += inX; //subtract that much
		outX = 0;
	}

	
	if ( inY < 0 ){
		outHeight += inY; // subtract that much
		outY = 0;
	}
	

	if ( inX + inWidth >= SCREEN_WIDTH ){
		outWidth = ( SCREEN_WIDTH - inX);
	}

	if ( outY + outHeight >= SCREEN_HEIGHT ){
		//outHeight -= ( SCREEN_HEIGHT - outY);
		outHeight = ( SCREEN_HEIGHT - outY );
	}

	if ( outWidth <= 0 )
		return;
	
	// swap some RGB
	r = ( inColor & 0xFF0000 ) >> 16;
	b = ( inColor & 0x0000FF ) << 16;

	outColor = ( inColor & 0x00FF00 ) | r | b;

	DrawTile( outX, outY, outWidth, outHeight, outColor );

}

void BorderTileRGB( int inX, int inY, int inWidth, int inHeight, char r, char g, char b){
	
	BorderTileColor( 
		inX, inY, 
		inWidth, inHeight,
		r | g << 8 | b << 16
	);

}


void BorderTile( int inX, int inY, int inWidth, int inHeight ){
	
	BorderTileColor( inX, inY, inWidth, inHeight, LIGHTBLUE );	
	BorderTileColor( inX, inY, inWidth, inHeight, LIGHTGREEN );		
	
}


// Assumes a fixed time so we can hard code
// some of the factors. (E.g. time delta)
int Lerp( int from, int to ){

	int delta;
	int snap = 0;

	if ( from == to ) return from;


	delta = to - from;
	snap = ( delta < 0 || delta > 0 );
	delta = delta / 3;

	// no floats, so snap within a pixel
	if ( snap && delta == 0 ){
		return to;
	}

	return from + delta;

}

void HighlightOffset( int inX, int inY, int inWidth, int inHeight, char colorOffset ){

	NTILE t;
	// int r = 0xA1, g = 0x9D, b = 0xFF;  // Border color, too light
	unsigned int r = 0x51;
	unsigned int g = 0x5F;
	unsigned int b = 0xFF; // Not true to C64, but looks good

	int w = 0;
	int h = 0;

	// clamp it at 0xFF so we don't go over
	r = ( r + colorOffset < 0xFF ) ? r + colorOffset : r;
	g = ( g + colorOffset < 0xFF ) ? g + colorOffset : g;
	b = ( b + colorOffset < 0xFF ) ? b + colorOffset : b;

	h1.lTarg = inX - ( inWidth / 2 );
	h1.rTarg = inX + ( inWidth / 2 );
	h1.botTarg = inY + ( inHeight / 2 );
	h1.topTarg = inY - ( inHeight / 2 );

	h1.lAct = Lerp( h1.lAct, h1.lTarg );
	h1.rAct = Lerp( h1.rAct, h1.rTarg );
	h1.botAct = Lerp( h1.botAct, h1.botTarg );
	h1.topAct = Lerp( h1.topAct, h1.topTarg );

	w = h1.rAct - h1.lAct;
	h = h1.botAct - h1.topAct;

	BorderTileColor( h1.lAct, h1.topAct, w, h, r << 16 | g << 8 | b );

}



void Highlight( int inX, int inY, int inWidth, int inHeight ){

	HighlightOffset( inX, inY, inWidth, inHeight, 0 );

}

// Basically "start a new draw call"
void DrawBG(){
	
	StartDrawing();

	BorderTileColor(
		0, 0, 
		SCREEN_WIDTH, 
		SCREEN_HEIGHT * 2,
		BACKGREEN
	);
	
}

// Calls DrawBG() and thus can be used to start a draw
void C64Border(){
	
	int thicc = 48;
	int tall = 32;

	DrawBG();
	
	// yes they overlap	
	BorderTile( 0, 0, thicc, SCREEN_HEIGHT );
	BorderTile( SCREEN_WIDTH - thicc, 0, thicc, SCREEN_HEIGHT );

	BorderTile( 0, 0, SCREEN_WIDTH, tall );
	BorderTile( thicc, SCREEN_HEIGHT - tall, SCREEN_WIDTH, tall );
	

}


#pragma region logger
//
// Logger
//

//
// Logs a formatted message into a buffer, with optional line update ('\x08' prefix)
// ( vs FontPrint which seems to avoid the heap and transfer prims straight to VRAM/GPU )
//
// Pre-scan the input string and work out how many %params were passed in.
// Allows repeat buffer use and repeating/clearing individual lines
//
// notes: chars, shorts, etc are 'promoted' to 32bit so they fit one stack "slot"
// notes: strings are just a 32bit pointer
// notes: not tested with floats
// notes: trying to create a function prototype causes a nonworking binary 
//        - not sure it's worth the trouble debugging, so it's near the top of the file.
//


#define TEXT_COLS 64  // -12 inside border
#define TEXT_ROWS 32  // -8 inside border

#define LOG_LENGTH (TEXT_ROWS * TEXT_COLS)
#define LINE_LENGTH 100   // max per single log entry

#define MAX_ARGS 10                                       // See note on missing vprintf

// Some pointer abuse here 'cause I've been toying with putting the buffer on the stack...
char headerLogBuffer[ LOG_LENGTH ];
ulong logBase = 0;                                       // 
ulong logPos = 0;                                        // 
ulong lastLogPos = 0;                                    // the start of the last line, for \r style line updates
int lastLineRepeatable = 0;                               // equivalent to \r


// Quickly adds a single char
// - does not check line wrapping
// - does not check buffer overflow
// - does not honor repeated line setups
// Indended for e.g. speeding up the hex editor rendering within known dimenstions
void BlahChar( char inChar ){
	
	*(uchar*)logPos = inChar;
	lastLogPos = logPos;	
	logPos += 1;

}

// Same deal
void BlahNewline(){

	*(uchar*)logPos = 10;
	lastLogPos = logPos;	
	logPos += 1;

}
						
// what, you think we were going to call it Log() after an intro like that?
void Blah( char* pMSG, ... ){

	va_list vaList;
	int offset = 0;
	int var = 0;
	int argsCounted = 0;
	int originalLineLength = 0;
	int generatedLineLength = 0;
	int thisLineRepeatable = 0;
	
	// A little extra wiggle room (e.g. 64 + CR + LF + \0 + mistakes )
	// let's put 'em on the stack
	//char tempBuffer[ 100 ];
	ulong params[ MAX_ARGS ];

	// Can point this at the tempbuffer to expand out % args...
	// or if no args, just use it to read from the incoming message
	char * lineBuffer;


	// scan the input to work out how many args we passed in
	while( pMSG[ offset ] != 0 && argsCounted < MAX_ARGS && offset < 200 ){

		if ( pMSG[ offset ] == '%' ){			
			argsCounted++;
		}

		offset++;

	}
	originalLineLength = offset;
	

	if ( argsCounted > 0 ){
		
		char tempBuffer[100];

		// pass the first arg before the ... param so the macro knows
		// where to look in the stack
		va_start( vaList, pMSG );

		for( offset = 0; offset < argsCounted; offset++ ){

			var = va_arg( vaList, ulong );
			params[ offset ] = var;

		}

		// done with this
		va_end( vaList );


		// Clear the line buffer.
		for( offset = 0; offset < LINE_LENGTH; offset++ ){
			tempBuffer[ offset ] = 0;
		}

			if ( argsCounted < 4 ){
				
				NewSPrintf( tempBuffer, pMSG, params[0], params[1], params[2] );
				
			} else {
				// If you're passing fewer than 10 params, it's not a problem as the formatter will just ignore them
				// If you're passing more, then they'll be clipped. No problems.
				NewSPrintf( tempBuffer, pMSG, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8], params[9] );
			}
		
		
		// Get the generated length -1 (e.g. excluding the null terminator)
		lineBuffer = tempBuffer;
		while( lineBuffer[++generatedLineLength] != 0 ){};

	} else {
		
		// point the lineBuffer at this instead
		lineBuffer = pMSG;
		generatedLineLength = originalLineLength;

	}


	if ( lineBuffer[0] == '\x08' )
		thisLineRepeatable = 1;
		

	// Go back a line
	if ( thisLineRepeatable && lastLineRepeatable ){
		logPos = lastLogPos;
	}


	// we at risk of going over the buffer length? reset
	// TODO: remove up to first newline or null terminator
	if ( ( logPos + generatedLineLength ) >= ( logBase + LOG_LENGTH ) ){
		ClearScreenText();
	}
	

	// Can't sprintf the formatted string into the buffer 'cause it'll add the null terminator
	// just copy it in manually
	for( offset = 0; offset < generatedLineLength; offset++ ){		
		*(uchar*)(offset+logPos) = lineBuffer[ offset ];
	}

	// record the where this line started... and where it ended
	lastLogPos = logPos;	
	logPos += generatedLineLength;

	lastLineRepeatable = thisLineRepeatable;

}


// Returns the address of the log buffer text
// TODO: this should be *sent* to the GPU to avoid circular references
ulong GetLogBuffer(){
	logBase = (ulong)&headerLogBuffer;
	return logBase;
}

// TODO: see above
ulong GetLogBufferEnd(){
	return logPos;
}




// E.g. running sans GUI?
char bufferInitialised = 0;			


void InitBuffer(){

	ClearScreenText();
	bufferInitialised = 1;

}

// No such issues prototyping this one...
void ClearScreenText(){

	int i = 0;

	GetLogBuffer();
	logPos = logBase;
	lastLogPos = logBase;	
	
	// A few null terminators will do.
	// 12 seemed good.
	for ( i = 0; i < 12; i++ ){		
		*(uchar*)(logBase+i) = 0;
	}	

}



#pragma endregion logger




// Shortcut for "draw the border, then the text, then vsync it"
void DBorder(){
	
	if ( !bufferInitialised )
		return;

	C64Border();
	Draw();
	
}


// Holds a message till you hit X
void HoldMessage(){

	MonitorPads();

	DBorder();

	while ( 1 ){

		MonitorPads();

		if ( Released( PADRdown ) ){
			return;
		}
		
	}

}

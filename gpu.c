// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// some reading
// http://problemkaputt.de/psx-spx.htm#gpuioportsdmachannelscommandsvram
// http://problemkaputt.de/psx-spx.htm#gpudisplaycontrolcommandsgp1
// https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/
// https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/fs1x8n4/?utm_medium=android_app&utm_source=share
// 


#include "gpu.h"
#include "littlelibc.h"
#include "hwregs.h"
#include "drawing.h"
#include "utility.h"
#include "timloader.h"
#include "drawing.h" // the sprite type

// Example: also possible via a little assembly fragment
// inc_font.tim via exe_wrapper.asm
extern ulong xfont;

#pragma region hardware registers and addresses

//
// Hardware Registers
//

#define MODE_NTSC 0
#define MODE_PAL 1 

#define SCREEN_WIDTH  512 // screen width
#define SCREEN_HEIGHT 240 // screen height

#define DMA_CTRL 0xBF8010F0
#define pDMA_CTRL *(volatile ulong*)DMA_CTRL

#define GP0 0xBF801810
#define pGP0 *(volatile ulong*)GP0

#define GP1 0xBF801814
#define pGP1 *(volatile ulong*)GP1

	// GP1 Status

	// bit 10, drawing is allowed?
	#define GP1_DRAWING_ALLOWED 0x400
	// bit 26 - ready for another command?
	#define GP1_READYFORCOMMAND 0x4000000	
	// bit 28 - ready to recieve DMA block
	#define GP1_READYFORDMABLOCK 0x10000000

	// GP0 Commands
	#define GP0_CLEAR_CACHE			0x01000000
	#define GP0_COPY_RECT_VRAM2VRAM	0x80000000
	#define GP0_COPY_RECT_CPU2VRAM	0xA0000000
	#define GP0_COPY_RECT_VRAM2CPU	0xC0000000

	// GP1 Commands

	#define GP1_RESET_GPU		0x00000000
	#define GP1_RESET_BUFFER	0x01000000
	#define GP1_ACK_GPU_INTR	0x02000000
	#define GP1_DISPLAY_ENABLE	0x03000000
	#define GP1_DMA_DIR_REQ		0x04000000
		#define GP1_DR_OFF		0x0
		#define GP1_DR_FIFO		0x01
		#define GP1_DR_CPUTOGP0	0x02
		#define GP1_DR_GPUTOCPU	0x03

	#define GP1_TEXPAGE 0xE1000000
		#define PAGE_8_BIT 0x80
		#define PAGE_4_BIT 0x00
		#define PAGE_DITHER 0x200
		#define PAGE_ALLOW_DISPLAY_AREA_DRAWING 0x400

// DMA Control Reg
#define DPCR 0xBF8010F0
#define pDPCR *(volatile ulong*)DPCR

// DMA Interrupt Reg
#define DICR 0xBF8010F4
#define pDICR *(volatile ulong*)DICR

#define D2_MADR 0xBF8010A0
#define pD2_MADR *(volatile ulong*)D2_MADR

#define D2_BCR	0xBF8010A4
#define pD2_BCR *(volatile ulong*)D2_BCR

// facilitates mem to vram transfer/cancelation/monitoring, etc
#define D2_CHCR 0xBF8010A8
#define pD2_CHCR *(volatile ulong*)D2_CHCR
	
	// 0x01000401
	// "link mode , mem->GPU, DMA enable"
	#define D2_TO_RAM 0
	#define D2_FROM_RAM 1

	// bits 10, 11
	#define SYNCMODE_IMMEDIATE	0x0	
	#define SYNCMODE_DMA		0x200
	#define SYNCMODE_LINKEDLIST	0x400

	// bit 24
	#define D2_START_BUSY	0x1000000

#define INT_VBLANK 0x01

// For testing edge drawing tolerances
#define testOffsetY 20
#define testOffsetX 2

#pragma endregion hardware registers and addresses


//
// Protos
//
void Flip();
void WaitGPU();
void VSync();
void DrawFontBuffer();
void EnableDisplay( char doEnable );
void UploadFont();


//
// Vars
//
#define DOUBLE_BUFFERED 1
ulong drawBufferY = 0;
ulong displayBufferY = 0;


// A pre-draw hook for testing timing, etc.
// E.g. try to get it done between StartDrawing and EndDrawing's VSync
void StartDrawing(){
	
	// There's nothing here now

}


// Generally just let drawing.c handle this
void EndDrawing(){

	DrawFontBuffer();	
	
	VSync();
	
	Flip();
	
}


// Flips the display area to the top or bottom of VRAM 
// so we can be display frame 0 while drawing frame 1
// Without it you'll get a little page tearing / flickering.
// The 2nd page sits at y=SCREEN_HEIGHT (240) in VRAM - right below the first.

void Flip(){
	
	#if DOUBLE_BUFFERED
		// Drawing to buffer 0? Display buffer 1
		// Drawing to buffer 1? Display buffer 0

		if ( drawBufferY == 0 ){
			drawBufferY = SCREEN_HEIGHT;
			displayBufferY = 0;
		} else {
			drawBufferY = 0;
			displayBufferY = SCREEN_HEIGHT;
		}
	#else
		drawBufferY = 0;
		displayBufferY = 0;
	#endif
		
	// Flip this as soon after vsync as possible
	// Start of Display Area in RAM  e.g. 0x0503C000 is (240p << 10)
	WaitGPU();	
	pGP1 = 0x05000000 | ( displayBufferY << 10 );		// double buffer?

	// Set drawing area top, left
	WaitGPU();		
	pGP0 = 0xE3000000 | ( drawBufferY << 10 ) | 0;

	// Set the drawing area bottom, right
	WaitGPU();
	pGP0 = 0xE4000000 | (( drawBufferY + SCREEN_HEIGHT) << 10) | SCREEN_WIDTH;	
	
	// Automatically offsets all poly/tile/primitives.
	// Could zero it and add 240 to everything, for example
	pGP0 = 0xE5000000 | (drawBufferY << 11 );
	WaitGPU();
	

}


// VBlank bit it still set even with ints disabled.
// set, so e.g. EnterCritical and wait loop+poll it.
// The BIOS does this, so it's not as jank as it sounds.

void VSync(){

	// Were we already in a crit section?
	uchar wasInCritical = EnterCritical();
	ushort oldMask = pIMASK;
	ushort blankCounter = 0;

	// Enable vblanks via the mask
	pIMASK |= INT_VBLANK;

	// Wait for the vblank int flag to happen
	while( !(pISTAT & INT_VBLANK) ){
		blankCounter++;
	}

	// Should probably acknowledge this int
	// if the pads weren't about to do that.
	pISTAT ^= INT_VBLANK;

	// Restore the old mask state
	pIMASK = oldMask;

	// Don't exit critical sec if we were already in one...
	if ( !wasInCritical ) ExitCritical();
	
}



#pragma GCC push options
#pragma GCC optimize ("-O0")
void InitGPU(){

	char vidMode = MODE_NTSC;
	
	// enable ints n stuff
	ExitCritical();
	
	if ( IsPAL() ){
		vidMode = MODE_PAL;
	}

	// reset
	pGP1 = 0;
	
	EnableDisplay( 0 );
	
	// DMA off
	WaitGPU();	
	pGP1 = 0x04000000;
	
	if ( vidMode == MODE_PAL ){
		
		WaitGPU();		
		pGP1 = 0x0800000A;
		
		// H Display Range
		WaitGPU();		
		pGP1 = 0x06C7B27B;

		// V Display Range
		WaitGPU();		
		pGP1 = 0x07046C2B;

	} else {

		WaitGPU();		
		pGP1 = 0x08000002;

		// H Display Range
		WaitGPU();		
		pGP1 = 0x06C67267;

		// V Display Range
		WaitGPU();		
		//pGP1 = 0x07040010;	// spotted it using 0x07040C13 when switching pal-ntsc
		pGP1 = 0x07040C13;
		

	}
    

    //SetPageDepth( 8, 1 );
	WaitGPU();
	
	Flip();
	
	EnableDisplay( 1 );
	NewPrintf("Display enabled.\n");

	UploadFont();
	
	InitBuffer();

	NewPrintf("Log Buffer ready\n");

	
}
#pragma GCC pop options

//
// Various GPU routines used in Greentro, Herben's Import Player, etc.
//

// WaitGPU - waits until GPU ready to recieve commands
void WaitGPU(){
    
	int waitCounter = 0;	
	int nullVal = 0;

	while ( 
		( pGP1 & GP1_READYFORCOMMAND ) == 0			// bit 28 in GP1
		//|| ( pD2_CHCR & D2_START_BUSY ) != 0		// bit 24 in D2-CHCR
	){
		waitCounter++;
		__asm__ volatile( "" );
	}

}


// TODO: Add the DMA block check?
void WaitIdle(){

	int waitCounter = 0;

	while ( ( pGP1 & GP1_READYFORCOMMAND ) == 0 ){
		waitCounter++;
	}

}

// Wait for DMA
void WaitDMA(){

	int waitCounter = 0;

	while ( ( pGP1 & GP1_READYFORCOMMAND ) == 0 ){
		waitCounter = 0;
	}

}

// Wait for DMA & GPU idle
void WaitDone(){

	WaitDMA();
	WaitIdle();

	return;

}

void EnableDisplay( char doEnable ){
	
	// disable the display
	WaitGPU();	
	__asm__ volatile ("");
	pGP1 = 0x03000000 | (!doEnable);
	
}


// Send a linked list to the GPU

void SendList( ulong listAddr ){
	
	ulong DPCRValue;

	WaitGPU();
	
	pDPCR |= 0x800;	// bit 11, GPU
		
	pGP1 = GP1_DMA_DIR_REQ | GP1_DR_CPUTOGP0;
	
	pD2_MADR = listAddr;

	pD2_BCR = 0;

	pD2_CHCR = D2_FROM_RAM | SYNCMODE_LINKEDLIST | D2_START_BUSY;
	
}

// Uploads some stuff (Texture, CLUT, etc) to VRAM without using
// DMA. Not the fastest, but it's plenty fast for Unirom.

void SendToVRAM( ulong memAddr, short xPos, short yPos, short width, short height ){
	
    NewPrintf( "Uploading to %d,%d %d,%d\n", xPos, yPos, width, height );

	int i = 0;
	int numDWords = ( width * height );
    
	WaitIdle();

	// DMA Off
	pGP1 = GP1_DMA_DIR_REQ;

	pGP1 = GP1_RESET_BUFFER;

	pGP0 = GP0_COPY_RECT_CPU2VRAM;
	pGP0 = (( yPos << 16 ) | ( xPos & 0xFFFF  ));
	pGP0 = (( height << 16 ) | ( width / 2 ));

	for( i = memAddr; i < memAddr + numDWords; i+=4 ){
		
		pGP0 = *(ulong*)i;

	}
	
}



// 8 pages off the screen
// 2 pixels from the top
#define FNTLoadX 512
#define FNTLoadY 16

// 8 pages off the screen
// 0 pixels from the top
#define CLUTLoadX 512
#define CLUTLoadY 0

// For the font preview
// (e.g. check that your entire font is mapped properly)
#define UVX 0
#define UVY 1


void UploadFont(){
	
    /*    
    // E.g.
	// @ 640x0	
	SendToVRAM( ((ulong)&xfont + imageOffset), FNTLoadX, FNTLoadY, 128, 48 );
	// @ 640 x 256
	SendToVRAM( (ulong)&xfont + CLUTOffset, CLUTLoadX, CLUTLoadY, 512, 1 );
	*/

    TIMData nullTimData;
    UploadTim( (char*)&xfont, &nullTimData, CLUTLoadX, CLUTLoadY, FNTLoadX, FNTLoadY );

}

void SetPageDepth( int inPage, char in8Bit ){
    pGP0 = GP1_TEXPAGE | (PAGE_8_BIT* in8Bit) | PAGE_DITHER | PAGE_ALLOW_DISPLAY_AREA_DRAWING | inPage;
}


#pragma GCC push options
#pragma GCC optimize("-O0")

// Quick n dirty sprite draw, using the info parsed from a TIM header
// Note: this is likely to be a ton slower than using a linked list / OT!

void DrawSprite( Sprite * inSprite ){

    //NewPrintf( "X y w h %d, %d, %d, %d\n", inSprite->xPos, inSprite->yPos, inSprite->width, inSprite->height );
    DrawTIMData( inSprite->data, inSprite->xPos, inSprite->yPos, inSprite->width, inSprite->height );
    
}

void DrawTIMData( TIMData * inData, ulong inX, ulong inY, ulong inWidth, ulong inHeight ){   
    
    // x,y - Typical Order: TL, BL, TR, BR    
    // vertex data
    short L = inX;
    ulong T = inY;
    short R = L + inWidth;
    ulong B = T + inHeight;
    
    // Same thing for each letter's UV
    // The UV is relative to the texture page (unlike textured rectangle, the texpage is specified below)
    uchar uvLeft  = inData->pixU;
    ushort uvTop = inData->pixV;
    uchar uvRight    = uvLeft + (inData->vramWidth * 2);
    ushort uvBottom = uvTop + (inData->vramHeight * 1);
    
    // Now we have our four points
    // TODO: we should allow some sort of rotation here
    // based around a centre point
    ushort uvTopLeft    = (uvTop << 8 ) | uvLeft;
    ushort uvBottomLeft = (uvBottom << 8 ) | uvLeft;
    ushort uvTopRight   = (uvTop << 8 ) | uvRight;
    ushort uvBottomRight = (uvBottom << 8 ) | uvRight;

    // May be helpful to visualise this as splitting the VRAM
    // 16-pix long rows, 64 of them per line        
    ulong clutPos = ( inData->clutY << 6 ) | ( inData->clutX / 16 );        
        
    WaitGPU();
    
    // functionally similar to SetPageDepth
    ulong pageConfig = ( PAGE_8_BIT | inData->texPage) << 16;

    pGP0 = 0;
    pGP0 = 0x2C808080;                      // Command + Color    

    pGP0 = (T << 16) | (L);                 // vert T,L
    pGP0 = (clutPos << 16) | uvTopLeft;

    pGP0 = (B << 16) | (L);                 // vert B,L
    
    pGP0 = pageConfig | uvBottomLeft;

    pGP0 = (T << 16) | (R);                 // vert B,L
    pGP0 = uvTopRight;

    pGP0 = (B << 16) | (R);                 // vert B,L
    pGP0 = uvBottomRight;
    
}

#pragma GCC pop options


// Where's the font cursor?
static ulong cursorX = 0;
static ulong cursorY = 0;

// TileSize / TileSpacing - For padding or stretching
#define TS 8

// Font Size - actual size of the TIM
#define FS 8

#define maxRows 26
#define maxCols 63

void PrintChar( char inChar ){
	
	int id = inChar - 32;
	short col = (id % 16);
	short row = (id / 16);

	// x,y - Typical Order: TL, BL, TR, BR
	short L = ( (cursorX) * TS ); // the offset from earlier	
	ulong T = (  ((cursorY) * TS ) ) << 16;
	
	// Same thing for each letter's UV
	// 8 being twice the offset we used for fntloadx
	char fL = (col * FS);
	short fT = ( (row * FS) + FNTLoadY ) << 8;
		
	WaitGPU();
				
	pGP0 = 0;
	pGP0 = 0x75808080;
	pGP0 = ( T ) | ( L );	// vert Y,X
	pGP0 = 0x00200000 | fT | fL;  // clut @ x= 512, then texpos
	
	
	/*
	// Same thing with an unlinked linked list
	sprt_8.u = 8 * col;
	sprt_8.v = 8 * row;

	sprt_8.x = 8 + ( cursorX * 8 );
	sprt_8.y = 8 + ( cursorY * 8 );

	SendList( (ulong)&sprt_8 );
	*/

}


// Uses BRG color format, so there's a wrapper
// for this in drawing.c
void DrawTile( short inX, short inY, short inWidth, short inHeight, ulong inColor ){
	
	//pGP0 = 0;
	pGP0 = 0x60000000 | ( inColor & 0x00FFFFFF);	
	pGP0 = (inY << 16 ) | inX;	
	pGP0 = (inHeight << 16 ) | inWidth;
	
	WaitGPU();
	
}




void DrawFontBuffer(){
	
    SetPageDepth( 8, 1 );

	ulong logBuffer = GetLogBuffer();
	char readChar = *(char*)logBuffer;
	ulong iStart = GetLogBuffer();
	ulong iMax = GetLogBufferEnd();
	ulong i;
    
    // Font's hardcoded (in this demo) to Page 8
    // e.g. 512px, immediately offscreen on the right
    SetPageDepth( 8, 1 );
	
	cursorX = 0;
	cursorY = 0;
	
	for ( i = iStart ; i < iMax; i++ ){
		
		
		readChar = *(char*)i;
		if ( readChar == 0 )
			break;

		// separate incase we get a newline *on* the boundary
		if ( cursorX > maxCols ){
			cursorX = 0;
			cursorY++;			
		}

		// CR/LF - newline
		if ( readChar == 10 || readChar == 13 ){			
			cursorX = 0;
			cursorY++;
		}  else	if ( readChar == 32 ){
			cursorX++;
		} else if ( cursorY < maxRows ){					
			if ( readChar >= 10 )
				PrintChar( readChar );
			cursorX++;
		}

	}
	
	
	
}


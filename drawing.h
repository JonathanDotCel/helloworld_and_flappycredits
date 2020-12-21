// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#ifndef DRAWING_H
#define DRAWING_H

#include "timloader.h"

//
// Display / GPU vars
//

// we could set this lower to do the C64 intro properly
// but a lot of modern hardware is iffy about res switches
// e.g. my elgato fails about 1 in 20 times and that's not acceptable
#define SCREEN_WIDTH  512 // screen width
#define SCREEN_HEIGHT 240 // screen height
#define PACKETMAX 18

#define LIGHTGREEN 0x0000745A
//#define BACKGREEN 0x004CA7CB
#define BACKGREEN 0x00222222
#define LIGHTBLUE 0x007070DE
//#define LIGHTBLUE 0x00867ADE
//#define BACKBLUE 0x003232FA
#define BACKBLUE 0x00224477

// For screen layout
#define CHARWIDTH 8
#define CHARHEIGHT 8
#define HALFCHAR 4

typedef struct Sprite{

    // Size + Position we intend to draw to on screen
    // in the future we'll add rotation, etc
    unsigned long xPos;
    unsigned long yPos;
    unsigned long width;
    unsigned long height;
    
    // The TIM data in VRAM
    // e.g. position, palette location etc
    TIMData * data;

    //
    // Purely for example
    //
    int shiftedVeloX;
    int shiftedVeloY;

} Sprite;

// Vsycn, draw, timing, etc
void Draw();

void InitBuffer();

// Character logging
void BlahNewline();
void BlahChar( char inChar );
void Blah( char* pMSG, ... );
void ClearScreenText();
unsigned long GetLogBuffer();
unsigned long GetLogBufferEnd();


// Various drwawing primitives
void DrawBG();
void DBorder();
void C64Border();
void HoldMessage();

void Highlight( int inX, int inY, int inWidth, int inHeight );
void Outline( int inX, int inY, int inWidth, int inHeight );
void BorderTile( int inX, int inY, int inWidth, int inHeight );
void Pulse( int inX, int inY, int inWidth, int inHeight );

void RasterBorder();
unsigned long GetFrameCount();


#endif
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef GPU_H
#define GPU_H


void InitGPU();
void DrawFontBuffer();
void ClearPulseCounter();

// let drawing.c handle this
void StartDrawing();
void EndDrawing();

// Draws most of the tile primitives
void DrawTile( short inX, short inY, short inWidth, short inHeight, unsigned long inColor );

// Test function
void PrintChar( char inChar );


#endif
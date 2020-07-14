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
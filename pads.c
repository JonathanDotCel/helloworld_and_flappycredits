// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include "pads.h"

#include <stdlib.h>

#include "drawing.h"
#include "littlelibc.h"
#include "hwregs.h"
//#include "sio.h"            // JOY_MODE/SIO_MODE consts
#include "utility.h"

static char padsInstalled = 0;
static ushort padVals = 0x0000;
static ushort lastPadVals = 0x0000;
static ulong padReads = 0;


    // References: 
    // https://github.com/Lameguy64/n00brom/blob/master/rom/pad.inc
    // https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/sio0/driver.c
    // http://problemkaputt.de/psx-spx.htm
    // SCPH7001 bios
    // Trusty logic analyser <3


    #define JOY_TXRX    0x1F801040
    #define pJOY_TXRX   *(volatile uchar*)JOY_TXRX
    #define JOY_STAT	0x1F801044
    #define pJOY_STAT   *(volatile ushort*)JOY_STAT
    #define JOY_MODE	0x1F801048
    #define pJOY_MODE   *(volatile ushort*)JOY_MODE
    #define JOY_CTRL	0x1F80104A
    #define pJOY_CTRL    *(volatile ushort*)JOY_CTRL
    #define JOY_BAUD	0x1F80104E
    #define pJOY_BAUD   *(volatile ushort*)JOY_BAUD
    

    #define JOYCTRL_TXENABLE    0x01       // bit 0
    #define JOYCTRL_SELECTJN    0x02       // bit 1, select pad in bit 13/
    #define JOYCTRL_ACK         0x10
    #define JOYCTRL_RESET       0x40
    #define JOYCTRL_ACKENABLE   0x1000    // bit 12
    #define JOYCTRL_PAD2        0x2000    // set for 2nd pad

    #define JOYSTAT_TXREADY_FLAG_1 0x01
    #define JOYSTAT_RXFIFO_NOT_EMPTY 0x02
    #define JOYSTAT_TX_FINISHED 0x04    
    #define JOYSTAT_IRQ 0x200

    #define PAD_READ_ERROR_MASK 0x100   

// heh, you really don't want to mess with the timing here
// it's been timed to within a microsecond or so to maximise compatibility
#pragma GCC push options
#pragma GCC optimize("-O0")

void PadDelay(int inLen) {    
    int i = 0;
    for (i = 0; i < inLen; i++) {
        __asm__ volatile("" : "=r"(i) : "r"(i));
    }
}

ulong Swap( uchar inCommand, ulong inDelay );

static void PadInit_Internal(){

    pJOY_CTRL = JOYCTRL_RESET;              // (0x40)
    pJOY_BAUD = 0x88;                       // 250khz, default value
    pJOY_MODE = MR_CHLEN_8 | MR_BR_1;       // 8 bit, 1x Mul (0xD)
    
    PadDelay( 10 );

    pJOY_CTRL = JOYCTRL_SELECTJN;       // 0x02

    PadDelay( 10 );

    pJOY_CTRL = JOYCTRL_SELECTJN | JOYCTRL_PAD2;  // 0x2002

    PadDelay( 10 );

    pJOY_CTRL = 0x0;

    PadDelay( 1500 );
    
    /*
    int timeout = 80;
    while( ( pJOY_STAT & JOYSTAT_RXFIFO_NOT_EMPTY ) != 0 ){
        if ( timeout-- <= 0 ) break;
    }
    */


}




void PadClearInt(){

    pISTAT &= (~0x80);

}

ulong PadWaitInt(){

    ulong timeout = 50;
    while( (pISTAT & 0x80) == 0 ){
        if ( timeout-- <= 0 ){            
            return 0;
        }        
    }
    
    return 1;

}

// Sends a byte to the pad and gets one back in exchange
ulong Swap( uchar inCommand, ulong inDelay ){
    
    pJOY_TXRX = inCommand;      // fifo = 0

    PadDelay( inDelay );    

    pJOY_CTRL |= JOYCTRL_ACK;   // Ctrl | 0x10;

    PadClearInt();              // for the NEXT Swap() call

    int timeout = 80;
    while( ( pJOY_STAT & JOYSTAT_RXFIFO_NOT_EMPTY ) == 0 ){        
        if ( timeout-- <= 0 ) break;
    }
    
    return pJOY_TXRX;
    
}


ulong ReadPad( int whichPad, ulong inBuffer ){
    
    ulong bytesRead = 0;    
    uchar lastByte = 0;
    uchar success = 0;
    uchar bytesToRead = 0;

    // store the old pad mask and enable pad ints
    ulong oldMask = pIMASK;
    
    char wasCritical = EnterCritical();
    
    pIMASK |= 0x80;

    ulong padSelector = (whichPad * 0x2000);

    // Select the correct pad    
    pJOY_CTRL = padSelector | JOYCTRL_SELECTJN;  // whichPad | 2

    // About 38 microseconds from SEL going low.
    // Kernel varies ~1 bit either side
    PadDelay( 0x11 );

    // Kernel likes to do this separately
    ulong selectEnable = JOYCTRL_TXENABLE | JOYCTRL_SELECTJN | JOYCTRL_ACKENABLE;
    pJOY_CTRL = padSelector | selectEnable;     // padSelector | 0x1003
    
    int timeout = 80;
    while( ( pJOY_STAT & JOYSTAT_TXREADY_FLAG_1 ) == 0 ){   // STAT & 1 == 0
        if ( timeout-- <= 0 ) break;
    }

    // From 7k bios, no wait for int beforehand
    {
        lastByte = Swap( 0x01, 0x14 );
        if ( lastByte == 0xFFFFFFFF ) goto fail;
        //*(volatile uchar*)(inBuffer + bytesRead++) = lastByte;
        
    }
    
    // 28 microseconds from last ack
    // (77->106 from SEL going low)
    PadDelay( 0x12 );
    if ( !PadWaitInt() ) goto fail;

    // expected result 0x41 for digi, 0x73 for anal
    {
        lastByte = Swap( 0x42, 0x19 );
        if ( lastByte == 0xFFFFFFFF ) goto fail;
        *(volatile uchar*)(inBuffer + bytesRead++) = lastByte;
        bytesToRead = ((lastByte & 0xF) << 1) +2; // halfwords to bytes
        
    }


    // Let's limit this to prevent buffer overflows
    // e.g. unsynced brook adapter
    bytesToRead = (bytesToRead & 0xF);

    // No artificial delay here, there's naturally about
    // 7ms delay either side of the last ACK
    // 7 to get here from Swap() and 7 till the next send
    if ( !PadWaitInt() ) goto fail;

    // 0x00 for pads, 0x01 for multitap
    {
        lastByte = Swap( 0x00, 0x14 );
        if ( lastByte == 0xFFFFFFFF ) goto fail;
        *(volatile uchar*)(inBuffer + bytesRead++) = lastByte;
        
    }
    
    // Aiming again for about 7microseconds either side of the last ack.    
    // PadDelay( 0x02 );                     // fix for 1080H pads (early)

    // This byte is only supposed to be 0 when the pad's entered config mode
    // but the brook adapter sets it anyway when toggling
    if ( lastByte == 0x5A || lastByte == 0 ){

        while( (bytesRead < bytesToRead) ){
            
            if ( !PadWaitInt() ) goto fail;     // throws off the timing a bit.

            lastByte = Swap( 0x00, 0x10 );
            if ( lastByte == 0xFFFFFFFF ) break;
            *(volatile uchar*)(inBuffer + bytesRead++) = lastByte;
            
            //if ( !PadWaitInt() ) break;
            
        }
        
        //lastByte = Swap( 0x00, 0x10 );        
        //*(volatile uchar*)(inBuffer + bytesRead++) = lastByte;

    } else {
        goto fail;
    }

    success = 1;
    fail:
    
    // if pad ints weren't enabled, turn them back off
    if ( (oldMask & 0x80) == 0 )
    pIMASK &= ~0x80;

    // returns SEL hi
    pJOY_CTRL = 0;

    /*
    // not necessary, but 175ms after SEL goes high
    // if there's no 2nd pad, the BIOS does this little
    // hiccup for 7 microseconds
    Delay( 126 );    
    pJOY_CTRL = padSelector | JOYCTRL_SELECTJN;  // whichPad | 2
    Delay( 7 );
    pJOY_CTRL = 0;
    */
    

    if ( success ){        
        //NewPrintf( "Pad_%x_ %x   %x   BR=%x\n", whichPad, *(ulong*)inBuffer, *(ulong*)(inBuffer+4), bytesRead );
    } else {
        //NewPrintf( "FAIL_%x_ %x   %x   BR=%x\n", whichPad, *(ulong*)inBuffer, *(ulong*)(inBuffer+4), bytesRead );
        for( int i = 0; i < 0x10; i++ ){
            *(uchar*)(inBuffer + i ) = 0xFF;
        }
    }
    
    padReads++;

    if ( !wasCritical ) ExitCritical();
    
    return bytesRead;
    

}


#pragma GCC pop options



// NOTE: remember the extra 8 bytes of stack

unsigned long GetPadVals() { return padVals; }


void InitPads() {
    
    padVals = 0;
    lastPadVals = 0;    
    padsInstalled = 1;
    
    PadInit_Internal();    

}


char allowKeyRepeats = 1;
unsigned long framesHeld = 0;
unsigned long repeatDelay = 45;

// Record pad state to determine press/unpress
void MonitorPads() {
    
    //pPADBUFFER = 0xFFFF;
    *(unsigned long*)PADBUFFER = 0xFFFFFFFF;

    ReadPad( 0, PADBUFFER );
    ReadPad( 1, PAD2BUFFER );
    
    if ( padReads < 20 ){
        // give it a few frames to sync up, filter
        // voltage issues, etc.
        // 10 per pad
        padVals = 0x0000;
        lastPadVals = 0x0000;
    } else {
        
        ushort tempPrevValue = padVals;
        lastPadVals = padVals;

        // read the 2xLSB for each pad
        ushort inv1 =  ~(*(ushort*)(PADBUFFER+2));
        ushort inv2 =  ~(*(ushort*)(PAD2BUFFER+2));

        padVals = inv1 | inv2;

        // and reverse byte order
        padVals = ( padVals >> 8 ) | ( (padVals & 0xFF) << 8 );

        if ( allowKeyRepeats ){
            
            // key repeat timer
            ushort directional = ( PADLleft | PADLright | PADLup | PADLdown );
            uchar isDirectional = (padVals & directional ) != 0;

            // been holding the same key for a while?
            // release it to allow
            if ( tempPrevValue == padVals && isDirectional ){
                framesHeld++;
                if ( framesHeld % 18 == 0 ){
                    // release just the arrow keys, so it doesn't trigger, x, triangle, etc
                    // e.g. remove just the 'directional' bits
                    padVals &= (~directional);
                }
            } else {
                framesHeld = 0;
            }

        }


    }
    
}

int AnythingPressed() {

    if (lastPadVals != 0 && padVals == 0) {
        // clear these so it doesn't send the key to the next menu
        padVals = 0;
        lastPadVals = 0;
        return 1;
    }
    return 0;

}

int Held(ulong inButton) { return (lastPadVals & inButton) != 0; }

// Was any button pressed/released this frame?
int Released(ulong inButton) {
    
    int returnVal;
    returnVal = !(padVals & inButton) && (lastPadVals & inButton);

    if (returnVal) {
        // clear padVals so the next menu/item/prompt doesn't auto get the new value
        // leave other buttons though, incase we're using shoulder buttons to accelerate
        // menu options, etc
        lastPadVals ^= inButton;        
    }

    return returnVal;
}

void PadStop(){

}

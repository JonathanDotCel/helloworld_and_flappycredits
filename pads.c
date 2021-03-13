// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// References: 
// https://github.com/Lameguy64/n00brom/blob/master/rom/pad.inc
// https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/sio0/driver.c
// http://problemkaputt.de/psx-spx.htm
// SCPH7001 bios
// Trusty logic analyser <3
// Big love to Dan, Schnappy, Skitchin, Arthur, Nicolas for help and info.
// 

//
// Basic use
// ReadPad( whichever, buffer*, 0x42, 0x00 );
//
// Or 
// PadStartComms();
// Send things
// PadEndComms();
// 


#include "pads.h"
#include <stdlib.h>
#include "drawing.h"
#include "littlelibc.h"
#include "hwregs.h"
#include "utility.h"

//
// Defines
//

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

//
// Vars
//

// For Swap
static uchar lastByte = 0;
static uchar success = 0;
static uchar bytesRead = 0;
static uchar bytesToRead = 0;
static uchar swapFailed = 0;  // pad didn't return anything

// General
static ushort padVals = 0x0000;
static ushort lastPadVals = 0x0000;
static ulong padReads = 0;

//
// Protos
//

static ulong Swap( uchar inCommand, ulong inDelay );

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

// Sets the pad SIOs to the default
// values you'd see in the kernel/shell
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
    
}

// Clear the pads' interrupt bit
void PadClearInt(){

    pISTAT &= (~0x80);

}

// Wait for the Pads' interrupt flag
// This is passive and doesn't require any kind of driver
// on the kernel end of things
ulong PadWaitInt(){

    ulong timeout = 50;
    while( (pISTAT & 0x80) == 0 ){
        if ( timeout-- <= 0 ){            
            return 0;
        }        
    }
    
    return 1;

}

// Similar to PadWaitInt, but on the pads' HW regs
ulong PadWaitAck( ulong inTimeout ){

    ulong waitabix = 0;    
    while ( (pJOY_STAT & 0x200) == 0 ){
        // do a thing?
        if ( waitabix++ >= inTimeout ){
            return 0;
        }
    }

    return 1;

}

// Sends a byte to the pad and gets one back in exchange
static ulong Swap( uchar inCommand, ulong inDelay ){
    
    swapFailed = 0;

    pJOY_TXRX = inCommand;      // fifo = 0

    PadDelay( inDelay );    

    pJOY_CTRL |= JOYCTRL_ACK;   // Ctrl | 0x10;

    PadClearInt();              // for the NEXT Swap() call
    
    int timeout = 80;
    while( ( pJOY_STAT & JOYSTAT_RXFIFO_NOT_EMPTY ) == 0 ){        
        if ( timeout-- <= 0 ){
            swapFailed = 1;
            NewPrintf( "Swap Failed\n" );
            return 0xFFFFFFFF;
        }
    }

    return pJOY_TXRX;
    
}


static ulong oldMask;
static char wasCritical;
uchar isFirstByte = 0;

ulong PadStartComms( ulong whichPad ){

    // store the old pad mask and enable pad ints
    oldMask = pIMASK;

    wasCritical = EnterCritical();

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
    
    lastByte = 0;    
    isFirstByte = 1;

}

void PadEndComms(){

   
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
    

    if ( !wasCritical ) ExitCritical();


}


// Rough timings from the bios/shell
//
// Sel to first clock pulse...
// 35us
// 10us to ack, 23us to next clock
// 12us to ack, 5us to next clock
// 8us to ack, 5us to next clock
// 7us to ack, 6us to next clock
//

// Pads tested:
// 1080A (digital)
// 1080H (digital)
// 1080M (digital)

// 1200A (ds)
// 1200M (ds)
// 1200H (ds)

// 10010A (ds2)
// 10010H (ds2)

// 110H (psone ds)

// NegCon, GCon45, Maus, etc

// Pads can be read relatively easily with a simple
// PadStartComms -> PadSendChunk (with constant timings) -> PadEndComms
// but since compatibility is fiddly across different models
// and timing's tight, here's a decent recreation of
// the bios/PSYQ's pad implementation.

// pad ID 0 indexed
// buffer is 0x10 length max
// requestByte is 0x42 (get inputs) by default)
// requestState is normally 0, but 1 for "set led", "set config mode", etc
ulong ReadPad( int whichPad, uchar * outBytes, char requestByte, char requestState ){
    
    success = 0;
    bytesRead = 0;
    bytesToRead = 0;
    
    uchar * originalBuffer = outBytes;

    PadStartComms( whichPad );
    
    // From 7k bios, no wait for int beforehand
    // Send: 0x01 Recv: 0xFF
    // this one isn't included in the byte count   
    {
        lastByte = Swap( 0x01, 0x00 );
        
        if ( lastByte == 0xFFFFFFFF ) goto fail;        
        //*outBytes++ = lastByte;  //<-- this one doesn't contrib to byte count

        // usually ~10us till Ack pulse happens                
        // (already happened by this point)

        // 24us from ack to next Swap
        PadDelay( 0x10 );
        if ( PadWaitAck( 0x50 ) ){
            //bytesRead++;
        } else {
            goto fail;
        }

    } 
    
    // Send: 0x42
    // Recv: 0x41 digi, 0x73 anal   
    {
        lastByte = Swap( requestByte, 0x00 ); //also works

        if ( lastByte == 0xFFFFFFFF ) goto fail;
        *outBytes++ = lastByte;

        // usually ~12us till Ack pulse happens                
        // (already happened by this point)

        // 12us to next swap
        PadDelay( 0x0A );
        if ( PadWaitAck( 0x50 ) ){
            bytesRead++;
        } else {
            goto fail;
        }

        //bytesToRead = length - 3;
        bytesToRead = ((lastByte & 0xF) << 1) +2; // halfwords to bytes
        bytesToRead = (bytesToRead & 0xF);

    }
    
    // Send: 0x00 for pads, 0x01 for multitap
    // Recv: 0x5A (or e.g. 0xFF, 0x00)
    {
        lastByte = Swap( 0x00, 0x14 );
        
        if ( lastByte == 0xFFFFFFFF ) goto fail;
        *outBytes++ = lastByte;
        
        // usually ~12us till Ack pulse happens                
        // (already happened by this point)

        // 7-8 us to next swap
        PadDelay( 0x09 );
        if ( PadWaitAck( 0x50 ) ){
            bytesRead++;
        } else {
            goto fail;
        }

    }   

    // 0x5A = normal
    // 0x00 = config mode or brook adapter 'stuck'
    if ( lastByte != 0x5A && lastByte != 0x00 ){
        goto fail;
    }

    // 4 bytes includes: 0x42, 0x5A, 0xVAL1, 0xVAL2
    if ( bytesToRead < 4 ) goto fail;
    
    // We've already got 2
    while( bytesRead < bytesToRead ){
                
        lastByte = Swap( requestState, 0x00 );

        if ( lastByte == 0xFFFFFFFF ) goto fail;
        *outBytes++ = lastByte;

        PadDelay( 0x09 );

        // Don't ack the last one        
        if ( bytesRead == (bytesToRead -1) ){
            break;
        }

        // Ack these ones
        if ( PadWaitAck( 0x50 ) ){
            bytesRead++;
        } else {
            goto fail;
        }
          
        
    }

    success = 1;
    fail:
    
    PadEndComms();

    if ( success ){        
        //NewPrintf( "Pad_%x_ %x   %x   BR=%x\n", whichPad, *(ulong*)originalBuffer, *(ulong*)(originalBuffer+4), bytesRead );
    } else {
        //NewPrintf( "FAIL_%x_ %x   %x   BR=%x\n", whichPad, *(ulong*)originalBuffer, *(ulong*)(originalBuffer+4), bytesRead );
        for( int i = 0; i < 0x10; i++ ){
            *originalBuffer++ = 0xFF;
        }
    }
    
    padReads++;
    return bytesRead;
    

} // PadRead

#pragma GCC pop options



// NOTE: remember the extra 8 bytes of stack

unsigned long GetPadVals() { return padVals; }


void InitPads() {
    
    padVals = 0;
    lastPadVals = 0;    
        
    PadInit_Internal();    

}


char allowKeyRepeats = 1;
unsigned long framesHeld = 0;
unsigned long repeatDelay = 45;

// Record pad state to determine press/unpress
void MonitorPads() {
    
    *(unsigned long*)PAD1BUFFER = 0xFFFFFFFF;

    ReadPad( 0, pPAD1BUFFER, 0x42, 0x00 );
    ReadPad( 1, pPAD2BUFFER, 0x42, 0x00 );
    
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
        ushort inv1 =  ~(*(ushort*)(PAD1BUFFER+2));
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

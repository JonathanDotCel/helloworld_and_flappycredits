// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// References:
//
// https://github.com/grumpycoders/pcsx-redux
// https://problemkaputt.de/psx-spx.htm
// https://github.com/Lameguy64/n00brom
// 

#include "ttyredirect.h"
#include "littlelibc.h"
#include "utility.h"

#include "drawing.h"
#include "sio.h"

// File control block structure
struct SIOFCB{

    ulong FCB_STATUS;
    ulong FCB_DISKID;
    ulong FCB_TADDR;
    ulong FCB_TLEN;
    ulong FCB_FPOS;
    ulong FCB_DFLAGS;
    ulong FCB_ERROR;
    ulong FCB_DCB;
    ulong FCB_FSIZE;
    ulong FCB_LBN;
    ulong FCB_FCBID;

};


#define MODE_READ 1
#define MODE_WRITE 2

void __attribute__((section(".kttytext"))) SendCharTTY( char inChar ){

    unsigned short waitFlag = SR_TXU | SR_TXRDY;
    ulong bailout = 0;
    
    while( ( pSIOSTATUS & waitFlag) == 0 ){
        if ( bailout++ > 20000 )
            break;
    }

    pSIODATA = inChar;

}


static inline int __attribute__((section(".kttytext"))) __attribute__((always_inline)) SomethingInBufferTTY() { return ((pSIOSTATUS & SR_RXRDY)); }

static inline void __attribute((section(".kttytext"))) __attribute__((always_inline)) AckTTY() { pSIOCONTROL |= CR_ERRRST; }

static uchar __attribute__((section(".kttytext"))) ReadCharTTY() {

    volatile ulong optWait = 0;

    // while data in buffer
    while ( !SomethingInBufferTTY() ) {
        optWait++;
    }

    // Has to be this order
    char rVal = pSIODATA;

    // Ack
    AckTTY();

    return rVal;

}

void __attribute__((section(".kttytext"))) SendStringTTY( char * inString, int inLength ){

    for ( int i = 0; i < inLength; i++){
        SendCharTTY( *inString++ );
    }

}

void __attribute__((section(".kttytext"))) KTTYAction(struct SIOFCB * fcb, ulong inMode){

    ulong i = 0;
    ulong bailout = 0;

    ulong transferLength = fcb->FCB_TLEN;
    char * readAddr = fcb->FCB_TADDR;

    if (inMode == MODE_WRITE){
    
        SendStringTTY( readAddr, transferLength );

    } else {

        for( int i = 0; i < transferLength; i++ ){
            *readAddr++ = ReadCharTTY();
        }

    }
    
}



static void __attribute__((section(".kttytext"))) KTTYNull() {}

static ulong __attribute__((section(".kttytext"))) KTTYReturn0() { return 0; }

static struct ttyDevice {

    const char * deviceName;
    ulong flags;
    ulong blockSize;
    const char * deviceDesc;
    
    void * init;
    void * open;
    void * action;
    void * close;
    void * ioctl;
    void * read;
    void * write;
    void * erase;
    void * undelete;
    void * firstFile;
    void * nextFile;
    void * format;
    void * chdir;
    void * rename;
    void * deinit;
    void * check;
} defaultTTYDevice = {
    "tty",
    3,
    1,
    "SIO_TTY",

    &KTTYNull,      // init
    &KTTYReturn0,   // open
    &KTTYAction,    // action
    &KTTYReturn0,   // close

    &KTTYReturn0,   // ioctl
    &KTTYNull,      // read
    &KTTYNull,      // write
    &KTTYNull,      // erase

    &KTTYNull,      // undelete
    &KTTYNull,      // firstfile
    &KTTYNull,      // nextfile
    &KTTYNull,      // format

    &KTTYNull,      // chdir
    &KTTYNull,      // rename
    &KTTYNull,      // deinit/remove
    &KTTYNull,      // check
};

static char ttyInstalled = 0;

ulong IsTTYInstalled() { return ttyInstalled; }

void RemoveTTY(){

    ulong wasCritical = EnterCritical();
    
    // Close stdin
    CloseFile( 0 );

    // Close stdout
    CloseFile( 1 );
    
    // Remove the dummy TTY device    
    RemoveDevice( defaultTTYDevice.deviceName );
    
    if ( !wasCritical ) ExitCritical();

}

void InstallTTY(){

    if ( ttyInstalled ) return;

    ulong wasCritical = EnterCritical();
    
    RemoveTTY();
    
    NewMemcpy( (char*)&__ktty_dest_start, (char*)&__ktty_src, (ulong)&__ktty_length );
    
    AddDevice( (void*)&defaultTTYDevice );

    // Open stdin and stdout again
    char fd1 = OpenFile( defaultTTYDevice.deviceName, 2 );
    char fd2 = OpenFile( defaultTTYDevice.deviceName, 1 );
   
    if ( !wasCritical ) ExitCritical();

    //Blah( "Files: %x,%x\n", fd1, fd2 );
    //HoldMessage();

    ttyInstalled = 1;
    
}

void TTYViewMemoryAllocation(){
    ClearTextBuffer();

    Blah( "\n\n\n\n\n\n" );
    Blah( "__ktty_src %x\n", (ulong)&__ktty_src );    
    Blah( "__ktty_length %x\n", (ulong)&__ktty_length );
    Blah( "__ktty_dest_start %x\n", (ulong)&__ktty_dest_start );
    Blah( "__ktty_dest_end %x\n", (ulong)&__ktty_dest_end );

    HoldMessage();
}



// For use with TTYRedirect when using stdin / readchar
// They won't work till this TTY driver is installed.
#pragma section non-relocated utility functions


char BIOS_ReadChar(){
    register int cmd asm("t1") = 0x3B;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((char(*)())0xA0)();
}

void BIOS_WriteChar( char inChar ){
    register int cmd asm("t1") = 0x3C;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((char(*)(char))0xA0)( inChar );
}

ulong BIOS_ReadString( char * dest ){
    register int cmd asm("t1") = 0x3D;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((ulong(*)(char*))0xA0)( dest );
}

void BIOS_WriteString( const char * src ){
    register int cmd asm("t1") = 0x3E;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((ulong(*)(char*))0xA0)( src );
}

char Raw_ReadChar(){

    //use the in-kernel address
    // also uses the in-kernel address
    // gcc will attempt to make this a regular function 
    // call and hit R_MIPS_26 without volatile
    volatile ulong addr = &ReadCharTTY;
    return ((char(*)())addr)();

}

void Raw_WriteChar( char c ){

    // also uses the in-kernel address
    // gcc will attempt to make this a regular function 
    // call and hit R_MIPS_26 without volatile
    volatile ulong addr = &SendCharTTY;    
    ((void(*)(char))addr)( c );

}

char __attribute__((always_inline)) STDIN_BytesWawiting(){
    return (pSIOSTATUS & SR_RXRDY) != 0;
}

#pragma endsection non-relocated utility functions
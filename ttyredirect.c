// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include "ttyredirect.h"
#include "littlelibc.h"
#include "utility.h"

#include "drawing.h"

#define SIO_DATA 0x1F801050
#define SIO_STAT 0x1F801054
#define SIO_MODE 0x1F801058
#define SIO_CTRL 0x1F80105E


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


#pragma GCC push options
#pragma GCC optimize ("-O0")

#define MODE_READ 1
#define MODE_WRITE 2


void __attribute__((section(".kttytext"))) KTTYAction( struct SIOFCB * fcb, ulong inMode ){

    ulong i = 0;
    ulong bailout = 0;
    
    if( inMode != MODE_WRITE )
        return;
    
    ulong transferLength = fcb->FCB_TLEN;
    ulong readAddr = fcb->FCB_TADDR;

    for( i = readAddr; i < readAddr + transferLength; i++ ){

        bailout = 0;
        
        // SR_TXU | SR_TXRDY
        while( (*(uchar*)SIO_STAT & 0x05) == 0 ){
            if ( bailout++ > 8000 )
                break;
        }

        *(uchar*)SIO_DATA = *(uchar*)readAddr;

    }
    

}

void __attribute__((section(".kttytext"))) KTTYNull(){
    
}

ulong __attribute__((section(".kttytext"))) KTTYReturn0(){
    return 0;    
}


#pragma GCC pop options



char deviceName[] = "tty";
char deviceDesc[] = "SIO TTY";


struct ttyDevice{

    char * deviceName;
    ulong flags;
    ulong blockSize;
    char * deviceDesc;
    
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

    (char*)deviceName,
    3,
    0,
    (char*)deviceDesc,

    &KTTYNull,      // init
    &KTTYReturn0,      // open
    &KTTYAction,      // action
    &KTTYReturn0,      // close

    &KTTYReturn0,      // ioctl
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

char ttyInstalled = 0;

ulong IsTTYInstalled(){
    return ttyInstalled;
}



void RemoveTTY(){

    char* namePointer = (char*)&deviceName;

    if ( !ttyInstalled )
        return;

    // Close stdin
    CloseFile( 0 );
    
    // Close stdout
    CloseFile( 1 );
    
    // Remove the dummy TTY device
    RemoveDevice( (char*)&namePointer );

}

void InstallTTY(){

    if ( ttyInstalled )
        return;

    ulong wasCritical = EnterCritical();
    
    RemoveTTY();
    
    NewMemcpy( (char*)&__ktty_dest_start, (char*)&__ktty_src, (ulong)&__ktty_length );

    // need a wee bit more stack for these ones...
    __asm__ volatile( "addiu $sp, $sp, -0x08\n\t" );

    AddDevice( (void*)&defaultTTYDevice );

    // Open stdin and stdout again
    OpenFile( deviceName, 2 );
    OpenFile( deviceName, 1 );
    
    __asm__ volatile( "addiu $sp, $sp, 0x08\n\t" );

    if ( !wasCritical )
        ExitCritical();

    ttyInstalled = 1;
    
}


void TTYViewMemoryAllocation(){

    ClearScreenText();

    Blah( "\n\n\n\n\n\n" );
    Blah( "__ktty_src %x\n", (ulong)&__ktty_src );    
    Blah( "__ktty_length %x\n", (ulong)&__ktty_length );
    Blah( "__ktty_dest_start %x\n", (ulong)&__ktty_dest_start );
    Blah( "__ktty_dest_end %x\n", (ulong)&__ktty_dest_end );

    HoldMessage();

}
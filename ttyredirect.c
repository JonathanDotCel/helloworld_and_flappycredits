

#ifndef TTYC_H
#define TTYC_H

#include "ttyredirect.h"
#include "littlelibc.h"
#include "utility.h"

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
        while( *(uchar*)SIO_STAT & 5 == 0 ){
            if ( bailout++ > 80 )
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

#endif

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


void InstallTTY(){

    ulong wasCritical = EnterCritical();

    // Some of these functions need a little extra stack.
    __asm__ volatile( "addiu $sp, $sp, -0x08\n\t" );
    
    // Close stdin
    CloseFile( 0 );
    
    // Close stdout
    CloseFile( 1 );
    
    // Remove the dummy TTY device
    RemoveDevice( deviceName );
    
    NewMemcpy( (char*)&__ktty_dest_start, (char*)&__ktty_src, (ulong)&__ktty_length );

    AddDevice( (void*)&defaultTTYDevice );

    // Open stdin and stdout again
    OpenFile( deviceName, 2 );
    OpenFile( deviceName, 1 );
    
    __asm__ volatile( "addiu $sp, $sp, 0x08\n\t" );

    if ( !wasCritical )
        ExitCritical();

    
}
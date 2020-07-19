

#ifndef TTYREDIRECT_H
#define TTYREDIRECT_H
    
    void InstallTTY();

    const extern unsigned long __ktty_src;
    const extern unsigned long __ktty_dest_start;
    const extern unsigned long __ktty_length;
    
#endif




#pragma once
    
    void InstallTTY();
    void RemoveTTY();
    unsigned long IsTTYInstalled();
    void TTYViewMemoryAllocation();
    
    const extern unsigned long __ktty_src;
    const extern unsigned long __ktty_dest_start;
    const extern unsigned long __ktty_dest_end;
    const extern unsigned long __ktty_length;
    



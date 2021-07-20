// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
    
    void InstallTTY();
    void RemoveTTY();
    unsigned long IsTTYInstalled();
    void TTYViewMemoryAllocation();
    
    // Convenience functions for using TTY through the BIOS
    // Note: they won't work untill InstallTTY is called

    char BIOS_ReadChar();
    void BIOS_WriteChar( char inChar );

    unsigned long BIOS_ReadString( char * dest );
    void BIOS_WriteString( const char * src );

    // Same thing but bypasses the bios
    // Note: also won't work unless InstallTTY is called

    char Raw_ReadChar();
    void Raw_WriteChar( char c );

    // Is there anything waiting in the SIO buffer?
    // E.g. for Raw_ReadChar() or BIOS_ReadChar()
    
    char STDIN_BytesWawiting();

    // For use with TTYViewMemoryAllocation
    
    const extern unsigned long __ktty_src;
    const extern unsigned long __ktty_dest_start;
    const extern unsigned long __ktty_dest_end;
    const extern unsigned long __ktty_length;


// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
    
    void InstallTTY();
    void RemoveTTY();
    unsigned long IsTTYInstalled();
    void TTYViewMemoryAllocation();
    
    const extern unsigned long __ktty_src;
    const extern unsigned long __ktty_dest_start;
    const extern unsigned long __ktty_dest_end;
    const extern unsigned long __ktty_length;
    



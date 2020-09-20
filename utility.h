// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
// Various small utility functions
// 
// e.g. is it a standalone .exe, from a ROM, is caetla installed, etc.
//

#ifndef UTILITY_H
#define UTILITY_H

#include "littlelibc.h"
#include "pads.h"

int InCriticalSection();
int EnterCritical();
int ExitCritical();


void ResetGraph();
void InitHeap (unsigned long * a, unsigned long b);
int StopCallback(void);

unsigned long ResetEntryInt();

void Delay( int inLen );

int IsJAP();
int IsPAL();
int IsROM();

int HasCaetla();
//void FastBoot();

void ROM_Reboot();

void UnloadMe();

void RebootToShell();

void rom_load_caetla_normal();
void rom_load_caetla_boot();
void rom_load_caetla_comms();

// Some kernel functions for the TTY redirect

void AddDevice( void * deviceInfo );
void RemoveDevice( char * deviceName );
void PrintDevices();
void CloseFile( ulong fileHandle );
ulong OpenFile( char * fileName, ulong accessMode );


#endif // !UTILITY_H

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

#endif // !UTILITY_H
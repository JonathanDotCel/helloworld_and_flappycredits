// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef PADS_H
#define PADS_H

// This is Sony's naming convention.
#ifndef _LIBETC_H_

#define PADLup     (1<<12)
#define PADLdown   (1<<14)
#define PADLleft   (1<<15)
#define PADLright  (1<<13)
#define PADRup     (1<< 4)
#define PADRdown   (1<< 6)
#define PADRleft   (1<< 7)
#define PADRright  (1<< 5)
#define PADi       (1<< 9)
#define PADj       (1<<10)
#define PADk       (1<< 8)
#define PADl       (1<< 3)
#define PADm       (1<< 1)
#define PADn       (1<< 2)
#define PADo       (1<< 0)
#define PADh       (1<<11)
#define PADL1      PADn
#define PADL2      PADo
#define PADR1      PADl
#define PADR2      PADm
#define PADstart   PADh
#define PADselect  PADk

#endif


// Mode bits; normally you'd find these in the SIO code
#define MR_SB_00 0x0
#define MR_SB_01 0x40
#define MR_SB_10 0x80
#define MR_SB_11 0xC0
#define MR_P_EVEN 0x20
#define MR_PEN 0x10
#define MR_CHLEN_5 0x0
#define MR_CHLEN_6 0x4
#define MR_CHLEN_7 0x8
#define MR_CHLEN_8 0xC
#define MR_BR_1 0x1
#define MR_BR_16 0x2
#define MR_BR_64 0x3


void InitPads();
void PadStop();
int AnythingPressed();
int Released( unsigned long inButton );
int Held( unsigned long inButton );
void MonitorPads();
unsigned long GetPadVals();



#endif // !PADS_H
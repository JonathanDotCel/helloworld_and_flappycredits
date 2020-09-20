// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef HWREGS_H
#define HWREGS_H

#define ISTAT 0xBF801070
#define	pISTAT *(volatile ulong*)ISTAT

#define IMASK 0xBF801074
#define pIMASK *(volatile ulong*)IMASK

// 0x10 bytes into the scratch pad
// anywhere's good, really, but it's 0x10 long
#define PADBUFFER 0x1F800010
#define PAD2BUFFER 0x1F800020

#endif
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef HWREGS_H
#define HWREGS_H

#define ISTAT 0xBF801070
#define	pISTAT *(volatile ulong*)ISTAT

#define IMASK 0xBF801074
#define pIMASK *(volatile ulong*)IMASK


#endif
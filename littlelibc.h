// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef LITTLELIBC_H
#define LITTLELIBC_H

#include <stdarg.h> 

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

// These will obviously need refactored at some point

void NewPrintf( const char * str, ... );
void NewStrcpy( char * dst, const char * src );
void NewMemcpy( char * dst, const char * src, ulong len );
void NewSPrintf( char * out, const char * in, ... );
int NewStrncmp( const char * paramA, const char * paramB, ulong len );
int NewStrcmp( const char * paramA, const char * paramB );

#endif //! ITTLELIBC_H
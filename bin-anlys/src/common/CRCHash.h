// *****************************************************************************
// CRCHash Routines (Version 1.0)
// Copyright © 2004-2005 Mira Software, Inc.
// Author: Andrey Klimoff
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// o Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// o Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// o The names of the authors may not be used to endorse or promote
//   products derived from this software without specific prior written
//   permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *****************************************************************************

// Revision: September 15, 2006

/*******************************************************************************
   Cyclic Redundancy Check (CRC)

   A CRC is a numerical value related to a block of data. This value provides
information that allows an application to determine if the block of data has
been modified. A CRC is similar to a Checksum, but is considered a stronger
error-checking mechanism.

   CRC is based on binary polynomial calculation.

   Standard CRC-8 generator polynomial:
       Name               : CRC-8 Standard
       Standards          : -
       References         : -
       Initializing value : FF
       Finalizing value   : FF
       Polynomial value   : 31 (Mirror value = 8C)
       Polynom            : x^8 + x^5 + x^4 + 1

   Standard CRC-16 generator polynomial:
       Name               : CRC-16 Standard
       Standards          : ITU X.25/T.30
       References         : LHA
       Initializing value : 0000
       Finalizing value   : 0000
       Polynomial value   : 8005 (Mirror value = A001)
       Polynom            : x^16 + x^15 + x^2 + 1

   CRC-16 CCITT generator polynomial:
       Name               : CRC-16 CCITT
       Standards          : CRC-CCITT
       References         : ITU X.25/T.30, ADCCP, SDLC/HDLC
       Initializing value : FFFF
       Finalizing value   : 0000
       Polynomial value   : 1021 (Mirror value = 8408)
       Polynom            : x^16 + x^12 + x^5 + 1

   CRC-16 XModem generator polynomial:
       Name               : CRC-16 XModem
       Standards          : CRC-XModem
       References         : -
       Initializing value : 0000
       Finalizing value   : 0000
       Polynomial value   : 8408 (Mirror value = 1021)
       Polynom            : x^16 + x^12 + x^5 + 1

   Standard CRC-32 generator polynomial:
       Name               : CRC-32 Standard
       Standards          : ISO 3309, ITU-T V.42, ANSI X3.66, FIPS PUB 71
       References         : ZIP, RAR, Ethernet, AUTODIN II, FDDI
       Initializing value : FFFFFFFF
       Finalizing value   : FFFFFFFF
       Polynomial value   : 04C11DB7 (Mirror value = EDB88320)
       Polynom            : x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 +
                            x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1

   Standard CRC-64 generator polynomial:
       Name               : CRC-64 Standard
       Standards          : ISO 3309
       References         : -
       Initializing value : FFFFFFFFFFFFFFFF
       Finalizing value   : FFFFFFFFFFFFFFFF
       Polynomial value   : 000000000000001B (Mirror value = D800000000000000)
       Polynom            : x^64 + x^4 + x^3 + x + 1

   CRC-64 ECMA182 generator polynomial:
       Name               : CRC-64 ECMA-182
       Standards          : ECMA 182
       References         : -
       Initializing value : FFFFFFFFFFFFFFFF
       Finalizing value   : FFFFFFFFFFFFFFFF
       Polynomial value   : 42F0E1EBA9EA3693 (Mirror value = C96C5795D7870F42)
       Polynom            : x^64 + x^62 + x^57 + x^55 + x^54 + x^53 + x^52 + 
                            x^47 + x^46 + x^45 + x^40 + x^39 + x^38 + x^37 + 
                            x^35 + x^33 + x^32 + x^31 + x^29 + x^27 + x^24 + 
                            x^23 + x^22 + x^21 + x^19 + x^17 + x^13 + x^12 + 
                            x^10 + x^9  + x^7  + x^4  + x    + 1

// ******************************************************************************
// Demo:

#include <iostream.h>
#include <conio.h>

#include "CRCHash.h"

int main(int argc, char* argv[])
    {
    char str[] = "Hello world!";

    cout << "String used for CRC calculations:" << endl << endl << '\t';
    cout << str << endl << endl;

    int len = sizeof(str) - 1;

    cout << "Calculation results:" << endl << endl << hex << uppercase;

    cout << '\t' << "CRC-8 : " << (int) CRC08Hash::Evaluate(str, len) << endl;
    cout << '\t' << "CRC-16: " << CRC16Hash::Evaluate(str, len) << endl;
    cout << '\t' << "CRC-32: " << CRC32Hash::Evaluate(str, len) << endl << endl;

    // Another calculation...

    char buffer[] = "Welcome to CRC!";

    len = sizeof(buffer) - 1;

    CRC32Hash X;

    for (int i = 0; i < len; i++) X.Update(buffer[i]);

    cout << "CRC-32 for the string \"" << 
        buffer << "\" is " << X.Evaluate() << endl;

    getch();
    }

*******************************************************************************/

#ifndef CRCHash_H
#define CRCHash_H

namespace MIAMIU  /* MIAMI Utils */
{

//******************************************************************************
// Template class 'CRCHash' implements mirror-algorithm of CRC calculation.

template <typename T, const T POLYNOM, const T INITIAL, const T FINAL>
class CRCHash
    {
    private:
        T CRC;
        struct CRCTable { T Data[256]; CRCTable(void); };
        static const CRCTable Table;
    public:
        CRCHash(void) : CRC(INITIAL) {;};
        CRCHash(const CRCHash & iCRCHash) : CRC(iCRCHash.CRC) {;};
        void Reset(void) { CRC = INITIAL; };
        void Update(const void *Buffer, size_t Length);
        void UpdateForFile(FILE *fd);
        inline void Update(unsigned char Value);
        T Evaluate(void) const { return CRC ^ FINAL; };
        static T Evaluate(const void *Buffer, size_t Length);
    };

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    const typename CRCHash<T, POLYNOM, INITIAL, FINAL>::CRCTable
        CRCHash<T, POLYNOM, INITIAL, FINAL>::Table;

//******************************************************************************
// Warning: Microsoft Visual C++ Compiler cannot compile methods of inner
// nested classes inside a template defined outside of template body! For inner
// nested classes inside a template, you must define functions inside the class.
// Such functions automatically become inline functions. This error is
// generated for code allowed by the standard of C++ language, however, not
// yet supported by Microsoft Visual C++. So, if you get an error, just move
// the method into a nested class of a template.

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    CRCHash<T, POLYNOM, INITIAL, FINAL>::
        CRCTable::CRCTable(void)
            {
            for (int i = 0, t = 0; i < 256; t = 8, i++)
                {
                Data[i] = i;
                while (t--) Data[i] = Data[i] >> 1 ^ (Data[i] & 1 ? POLYNOM :0);
                }
            }

//******************************************************************************

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    void CRCHash<T, POLYNOM, INITIAL, FINAL>::
        Update(unsigned char Value)
    {
            CRC = (CRC >> 8) ^ Table.Data[(Value ^ CRC) & 0xFFU];
    }

//******************************************************************************

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    void CRCHash<T, POLYNOM, INITIAL, FINAL>::
        Update(const void *Buffer, size_t Length)
    {
           register const unsigned char * Block =
               static_cast<const unsigned char *>(Buffer);
           while (Length--) Update(*Block++);
    }

//******************************************************************************

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    void CRCHash<T, POLYNOM, INITIAL, FINAL>::
        UpdateForFile(FILE *fd)
    {
        unsigned char Buffer[4096];
        int l;
        while ( (l=fread(Buffer, sizeof(unsigned char), 4096, fd) ) > 0)
        {
            register const unsigned char * Block =
                static_cast<const unsigned char *>(Buffer);
            while (l--) Update(*Block++);
        }
    }

//******************************************************************************

template <typename T, T POLYNOM, T INITIAL, T FINAL>
    T CRCHash<T, POLYNOM, INITIAL, FINAL>::
        Evaluate(const void *Buffer, size_t Length)
    {
            CRCHash Instance;
            Instance.Update(Buffer, Length);
            return Instance.Evaluate();
    }

//******************************************************************************

typedef uint8_t   CRC08;
typedef uint16_t  CRC16;
typedef uint32_t  CRC32;
typedef uint64_t  CRC64;

class CRC08Hash : public CRCHash<CRC08, 0x8CU, 0xFFU, 0xFFU> {};
class CRC16Hash : public CRCHash<CRC16, 0xA001U, 0x0000U, 0x0000U> {};
class CRC32Hash : public CRCHash<CRC32, 0xEDB88320UL, 0xFFFFFFFFUL, 0xFFFFFFFFUL> {};
class CRC64Hash : public CRCHash<CRC64, 0xC96C5795D7870F42ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL> {};

//******************************************************************************

// typedef unsigned __int64 CRC64;
// class CRC64Hash : public CRCHash<CRC64, 0x000000000000001BUI64, 0xFFFFFFFFFFFFFFFFUI64, 0xFFFFFFFFFFFFFFFFUI64> {};

//******************************************************************************

} /* namespace MIAMIU */

#endif

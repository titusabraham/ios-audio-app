//
//  VocDataTypeDef.h
//  Vocoder-iOS
//
//  Created by Abraham, Titus on 9/12/13.
//  Copyright (c) 2013 Abraham, Titus. All rights reserved.
//

#ifndef __Vocoder_iOS__VocDataTypeDef__
#define __Vocoder_iOS__VocDataTypeDef__


#include "stddef.h"


// -------------------------------------------------------------------------
// Standard Types
// -------------------------------------------------------------------------

// The following definitions are the same accross platforms.  This first
// group are the sanctioned types.

#ifndef _BOOLEAN_DEFINED
typedef unsigned char boolean;     // Boolean value type
#define _BOOLEAN_DEFINED
#endif

#ifndef _UINT32_DEFINED
typedef unsigned long int uint32;      // Unsigned 32 bit value
#define _UINT32_DEFINED
#endif

#ifndef _UINT16_DEFINED
typedef unsigned short uint16;      // Unsigned 16 bit value
#define _UINT16_DEFINED
#endif

#ifndef _UINT8_DEFINED
typedef unsigned char uint8;       // Unsigned 8  bit value
#define _UINT8_DEFINED
#endif

#ifndef _INT32_DEFINED
typedef signed long int int32;       // Signed 32 bit value
#define _INT32_DEFINED
#endif

#ifndef _INT16_DEFINED
typedef signed short int16;       // Signed 16 bit value
#define _INT16_DEFINED
#endif

#ifndef _INT8_DEFINED
typedef signed char int8;        // Signed 8  bit value
#define _INT8_DEFINED
#endif

#ifndef _WORD_DEFINED
typedef unsigned short word; /* Unsigned 16 bit value type. */
#define _WORD_DEFINED
#endif

#ifndef _DWORD_DEFINED
typedef unsigned long dword; /* Unsigned 32 bit value type. */
#define _DWORD_DEFINED
#endif

#ifndef _BYTE_DEFINED
typedef unsigned char byte; /* Unsigned 8  bit value type. */
#define  _BYTE_DEFINED
#endif

/*---------------------------------------------------------------------------
 *  Char Type
 *---------------------------------------------------------------------------*/
typedef uint8 ychar;

#ifndef WCHAR
typedef wchar_t WCHAR;
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif


#ifndef TRUE
#define TRUE   1   // Boolean true value
#endif

#ifndef FALSE
#define FALSE  0   // Boolean false value
#endif

#ifndef ON
#define ON   1    // On value
#endif

#ifndef OFF
#define OFF  0    // Off value
#endif

#ifdef NULL
#undef NULL //This is to ensure we dont have any random NULL implementation
#endif

#ifndef NULL
#define NULL 0
#endif


#endif /* defined(__Vocoder_iOS__VocDataTypeDef__) */

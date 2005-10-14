/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */
#ifndef COMMON_SCUMMSYS_H
#define COMMON_SCUMMSYS_H

#if !defined(_STDAFX_H) && !defined(__PLAYSTATION2__)
#error Included scummsys.h without including stdafx.h first!
#endif

#include <stdlib.h>
#include <stdio.h>

// Use config.h, generated by configure
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif

// make sure we really are compiling for WIN32
#ifndef WIN32
#undef _MSC_VER
#endif

#if defined(_MSC_VER)

	#define scumm_stricmp stricmp
	#define scumm_strnicmp _strnicmp
	#define snprintf _snprintf

	#if defined(CHECK_HEAP)
	#undef CHECK_HEAP
	#define CHECK_HEAP checkHeap();
	#endif

	#define SCUMM_LITTLE_ENDIAN

	#define FORCEINLINE __forceinline
	#define NORETURN _declspec(noreturn)
	#define PLUGIN_EXPORT __declspec(dllexport)

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed long int32;
	
	#define START_PACK_STRUCTS pack(push, 1)
	#define END_PACK_STRUCTS	 pack(pop)

#if defined(_WIN32_WCE) && _WIN32_WCE < 300

	#define CDECL __cdecl
	#define SMALL_SCREEN_DEVICE

#endif

#elif defined(__MINGW32__)

	#define scumm_stricmp stricmp
	#define scumm_strnicmp strnicmp
	#define SCUMM_LITTLE_ENDIAN

	#ifndef _HEAPOK
	#define _HEAPOK	(-2)
	#endif

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS	 pack(pop)

	#define PLUGIN_EXPORT __declspec(dllexport)

#elif defined(UNIX)

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp

	#ifndef CONFIG_H
		#ifdef X11_BACKEND
	
		// You need to set this manually if necessary
	//	#define SCUMM_LITTLE_ENDIAN
		
		#else
		/* need this for the SDL_BYTEORDER define */
		#include <SDL_byteorder.h>
	
		#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		#define SCUMM_LITTLE_ENDIAN
		#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
		#define SCUMM_BIG_ENDIAN
		#else
		#error Neither SDL_BIG_ENDIAN nor SDL_LIL_ENDIAN is set.
		#endif
		#endif
	#endif

	// You need to set this manually if necessary
//	#define SCUMM_NEED_ALIGNMENT

	#ifndef HAVE_CONFIG_H
	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;
	#endif

	#if defined(__DECCXX) // Assume alpha architecture
	#define INVERSE_MKID
	#define SCUMM_NEED_ALIGNMENT
	#endif

	#if defined(__GNUC__)
	#else
	#define START_PACK_STRUCTS pack (1)
	#define END_PACK_STRUCTS	 pack ()
	#endif

#elif defined(__PALMOS_TRAPS__)	|| defined (__PALMOS_ARMLET__) // PALMOS
	#include "palmversion.h"
	#include "globals.h"
	#include "extend.h"
	
	#define STRINGBUFLEN 256

	#define scumm_stricmp stricmp
	#define scumm_strnicmp strnicmp

#ifdef PALMOS_68K
	#define SCUMM_BIG_ENDIAN
#else
	#define SCUMM_LITTLE_ENDIAN
#endif
	#define SCUMM_NEED_ALIGNMENT

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed long int32;
	
	#define START_PACK_STRUCTS pack (1)
	#define END_PACK_STRUCTS   pack ()

#if !defined(COMPILE_ZODIAC) && !defined(PALMOS_ARM)
	#define NEWGUI_256
#endif

#elif defined(__MORPHOS__)
	#define scumm_stricmp stricmp
	#define scumm_strnicmp strnicmp

	#define SCUMM_BIG_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed long int32;

	#if defined(__GNUC__)
	#else
		#define START_PACK_STRUCTS pack (1)
		#define END_PACK_STRUCTS	 pack ()
	#endif
	#define main morphos_main

#elif defined(__DC__)

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp
	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed long int32;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS	 pack(pop)

#elif defined __GP32__ //ph0x
	#define SCUMM_NEED_ALIGNMENT
	#define SCUMM_LITTLE_ENDIAN 

	#define scumm_stricmp stricmp
	#define scumm_strnicmp strnicmp

	#define _HEAPOK 0

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned long uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed long int32;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS	 pack(pop)
#elif defined __PLAYSTATION2__
	#define SCUMM_NEED_ALIGNMENT
	#define SCUMM_LITTLE_ENDIAN 

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;

	typedef unsigned long uint64;
	typedef signed long int64;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS	 pack(pop)

	#include "backends/ps2/fileio.h"

	#define fopen(a, b)			ps2_fopen(a, b)
	#define fclose(a)			ps2_fclose(a)
	#define fflush(a)			ps2_fflush(a)
	#define fseek(a, b, c)		ps2_fseek(a, b, c)
	#define ftell(a)			ps2_ftell(a)
	#define feof(a)				ps2_feof(a)
	#define fread(a, b, c, d)	ps2_fread(a, b, c, d)
	#define fwrite(a, b, c, d)	ps2_fwrite(a, b, c, d)
	#define fgetc(a)			ps2_fgetc(a)
	#define fgets(a, b, c)		ps2_fgets(a, b, c)
	#define fputc(a, b)			ps2_fputc(a, b)
	#define fputs(a, b)			ps2_fputs(a, b)
	#define fprintf				ps2_fprintf
	#define fsize(a)			ps2_fsize(a)

	extern void ps2_disableHandleCaching(void);
#elif defined (__PSP__)
	#define	SCUMM_NEED_ALIGNMENT
	#define	SCUMM_LITTLE_ENDIAN

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;

	typedef unsigned long uint64;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS         pack(pop)

#elif defined (__amigaos4__)
	#include <exec/types.h>

	#define	scumm_stricmp strcasecmp
	#define	scumm_strnicmp strncasecmp

	#define	SCUMM_BIG_ENDIAN

	// You need to set this manually if necessary
	#define	SCUMM_NEED_ALIGNMENT

	#ifndef	HAVE_CONFIG_H
	typedef	unsigned char byte;
	typedef	unsigned int uint;
	#endif
#elif defined __SYMBIAN32__ // AnotherGuest / Sprawl / SumthinWicked

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp

	#define CDECL	
	#define SCUMM_NEED_ALIGNMENT
	#define SCUMM_LITTLE_ENDIAN	
	#define CHECK_HEAP
	#define SMALL_SCREEN_DEVICE

	#define FORCEINLINE inline
	#define _HEAPOK 0
	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short int  uint16;
	typedef unsigned long int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int int16;
	typedef signed long int int32;
	
	#define START_PACK_STRUCTS pack (push,1)
	#define END_PACK_STRUCTS   pack(pop)
#else
	#error No system type defined
#endif


//
// GCC specific stuff
//
#if defined(__GNUC__)
	#define GCC_PACK __attribute__((packed))
	#define NORETURN __attribute__((__noreturn__)) 
#else
	#define GCC_PACK
#endif


//
// Fallbacks / default values for various special macros
//
#ifndef START_PACK_STRUCTS
#define	START_PACK_STRUCTS
#define	END_PACK_STRUCTS
#endif

#ifndef FORCEINLINE
#define FORCEINLINE inline
#endif

#ifndef CHECK_HEAP
#define CHECK_HEAP
#endif

#ifndef CDECL
#define	CDECL
#endif

#ifndef PLUGIN_EXPORT
#define PLUGIN_EXPORT
#endif

#ifndef NORETURN
#define	NORETURN
#endif

#ifndef STRINGBUFLEN
#define STRINGBUFLEN 1024
#endif


//
// Overlay color type (FIXME: shouldn't be declared here)
//
#if defined(NEWGUI_256)
	// 256 color only on PalmOS
	typedef byte OverlayColor;
#else
	// 15/16 bit color mode everywhere else...
	typedef int16 OverlayColor;
#endif


#include "common/endian.h"

#endif

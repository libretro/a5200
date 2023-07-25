/*
 * atari.c - main high-level routines
 *
 * Copyright (c) 1995-1998 David Firth
 * Copyright (c) 1998-2006 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# elif defined(HAVE_TIME_H)
#  include <time.h>
# endif
#endif
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "antic.h"
#include "atari.h"
#include "cartridge.h"
#include "cpu.h"
#include "devices.h"
#include "gtia.h"

#include "input.h"

#include "memory.h"
#include "pia.h"
#include "pokeysnd.h"
#include "rtime8.h"
#include "sio.h"
#include "util.h"
#include "statesav.h"
#if defined(SOUND)
#include "pokeysnd.h"
#endif

unsigned int joy_5200_trig[4]  = {0};
unsigned int joy_5200_stick[4] = {0};
unsigned int joy_5200_pot[8]   = {0};

int start_binloading = FALSE;

int hold_start = 0;
int press_space = 0;
int tv_mode = TV_NTSC;

/* Now we check address of every escape code, to make sure that the patch
   has been set by the emulator and is not a CIM in Atari program.
   Also switch() for escape codes has been changed to array of pointers
   to functions. This allows adding port-specific patches (e.g. modem device)
   using Atari800_AddEsc, Device_UpdateHATABSEntry etc. without modifying
   atari.c/devices.c. Unfortunately it can't be done for patches in Atari OS,
   because the OS in XL/XE can be disabled.
*/
static UWORD esc_address[256];
static EscFunctionType esc_function[256];

static void Atari800_ClearAllEsc(void)
{
	int i;
	for (i = 0; i < 256; i++)
		esc_function[i] = NULL;
}

void Atari800_AddEsc(UWORD address, UBYTE esc_code, EscFunctionType function) {
	esc_address[esc_code] = address;
	esc_function[esc_code] = function;
	dPutByte(address, 0xf2);			/* ESC */
	dPutByte(address + 1, esc_code);	/* ESC CODE */
}

void Atari800_AddEscRts(UWORD address, UBYTE esc_code, EscFunctionType function) {
	esc_address[esc_code] = address;
	esc_function[esc_code] = function;
	dPutByte(address, 0xf2);			/* ESC */
	dPutByte(address + 1, esc_code);	/* ESC CODE */
	dPutByte(address + 2, 0x60);		/* RTS */
}

void Atari800_RemoveEsc(UBYTE esc_code)
{
	esc_function[esc_code] = NULL;
}

void Atari800_RunEsc(UBYTE esc_code)
{
	if (esc_address[esc_code] == regPC - 2 && esc_function[esc_code] != NULL) {
		esc_function[esc_code]();
		return;
	}
	Atari800_Exit();
}

void Warmstart(void)
{
	PIA_Reset();
	ANTIC_Reset();
	/* CPU_Reset() must be after PIA_Reset(),
	   because Reset routine vector must be read from OS ROM */
	CPU_Reset();
	/* note: POKEY and GTIA have no Reset pin */
}

void Coldstart(void)
{
	PIA_Reset();
	ANTIC_Reset();
	/* CPU_Reset() must be after PIA_Reset(),
	   because Reset routine vector must be read from OS ROM */
	CPU_Reset();
	/* note: POKEY and GTIA have no Reset pin */
	/* reset cartridge to power-up state */
	CART_Start();

	/* set Atari OS Coldstart flag */
	dPutByte(0x244, 1);
	/* handle Option key (disable BASIC in XL/XE)
	   and Start key (boot from cassette) */
	consol_index = 2;
	consol_table[2] = 0x0f;
	consol_table[2] &= ~CONSOL_OPTION; /* Always disable BASIC - not available on A5200 */
	if (hold_start) {
		/* hold Start during reboot */
		consol_table[2] &= ~CONSOL_START;
	}
	consol_table[1] = consol_table[2];
}

/* Reinitializes after machine_type or ram_size change.
   You should call Coldstart() after it. */
static void Atari800_InitialiseMachine(void)
{
	Atari800_ClearAllEsc();
	MEMORY_InitialiseMachine();
	Device_UpdatePatches();
}

static int Atari800_DetectFileType(const uint8_t *data, size_t size) {
	UBYTE header[4];
	if (data == NULL || size < 4)
		return AFILE_ERROR;
	memcpy(header, data, 4);
	switch (header[0]) {
	case 0:
		if (header[1] == 0 && (header[2] != 0 || header[3] != 0) /* && size < 37 * 1024 */) {
			return AFILE_BAS;
		}
		break;
	case 0x1f:
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if ((header[1] >= '0' && header[1] <= '9') || header[1] == ' ') {
			return AFILE_LST;
		}
		break;
	case 'A':
		if (header[1] == 'T' && header[2] == 'A' && header[3] == 'R') {
			return AFILE_STATE;
		}
		break;
	case 'C':
		if (header[1] == 'A' && header[2] == 'R' && header[3] == 'T') {
			return AFILE_CART;
		}
		break;
	case 'F':
		if (header[1] == 'U' && header[2] == 'J' && header[3] == 'I') {
			return AFILE_CAS;
		}
		break;
	case 0x96:
		if (header[1] == 0x02) {
			return AFILE_ATR;
		}
		break;
	case 0xf9:
	case 0xfa:
		return AFILE_DCM;
	case 0xff:
		if (header[1] == 0xff && (header[2] != 0xff || header[3] != 0xff)) {
			return AFILE_XEX;
		}
		break;
	default:
		break;
	}
	/* 40K or a-power-of-two between 4K and CART_MAX_SIZE */
	if (size >= 4 * 1024 && size <= CART_MAX_SIZE
	 && ((size & (size - 1)) == 0 || size == 40 * 1024))
		return AFILE_ROM;
	/* BOOT_TAPE is a raw file containing a program booted from a tape */
	if ((header[1] << 7) == size)
		return AFILE_BOOT_TAPE;
	if ((size & 0x7f) == 0)
		return AFILE_XFD;
	return AFILE_ERROR;
}

int Atari800_OpenFile(const uint8_t *data, size_t size)
{
        int type;
	// Remove cart if exist
	CART_Remove();

	type = Atari800_DetectFileType(data, size);

        if (type == AFILE_CART || type == AFILE_ROM) 
	{
		if (CART_Insert(data, size) == 0)
		{
			Coldstart();
			return type;
		}
	}
	return AFILE_ERROR;
}

void Atari800_Initialise(void)
{
	Device_Initialise();
	RTIME8_Initialise();
	SIO_Initialise ();

	INPUT_Initialise();

	// Platform Specific Initialisation 
	Atari_Initialise();

	// Initialise Custom Chips
	ANTIC_Initialise();
	GTIA_Initialise();
	PIA_Initialise();
	POKEY_Initialise();

	Atari800_InitialiseMachine();
}

UNALIGNED_STAT_DEF(atari_screen_write_long_stat)
UNALIGNED_STAT_DEF(pm_scanline_read_long_stat)
UNALIGNED_STAT_DEF(memory_read_word_stat)
UNALIGNED_STAT_DEF(memory_write_word_stat)
UNALIGNED_STAT_DEF(memory_read_aligned_word_stat)
UNALIGNED_STAT_DEF(memory_write_aligned_word_stat)

int Atari800_Exit(void) {
	SIO_Exit(); /* umount disks, so temporary files are deleted */
	return 0;
}

UBYTE Atari800_GetByte(UWORD addr) {
	UBYTE byte;

	switch (addr & 0xff00) {
	case 0x4f00:
	case 0x8f00:
		CART_BountyBob1(addr);
		return 0;
	case 0x5f00:
	case 0x9f00:
		CART_BountyBob2(addr);
		return 0;
	case 0xbf00:
		byte = CART_5200SuperCartGetByte(addr);
		return byte;
		break;
	case 0xd000:				/* GTIA */
	case 0xc000:				/* GTIA - 5200 */
		return GTIA_GetByte(addr);
	case 0xd200:				/* POKEY */
	case 0xe800:				/* POKEY - 5200 */
	case 0xeb00:				/* POKEY - 5200 */
		return POKEY_GetByte(addr);
	case 0xd300:				/* PIA */
		return PIA_GetByte(addr);
	case 0xd400:				/* ANTIC */
		return ANTIC_GetByte(addr);
	case 0xd500:				/* bank-switching cartridges, RTIME-8 */
		return CART_GetByte(addr);
	default:
		break;
	}

	return 0xff;
}

void Atari800_PutByte(UWORD addr, UBYTE byte) {
	switch (addr & 0xff00) {
    case 0x4f00:
    case 0x8f00:
      CART_BountyBob1(addr);
      break;
    case 0x5f00:
    case 0x9f00:
      CART_BountyBob2(addr);
      break;
	case 0xbf00:
	  CART_5200SuperCartPutByte(addr, byte);
	  break;
	case 0xd000:				/* GTIA */
    case 0xc000:				/* GTIA - 5200 */
      GTIA_PutByte(addr, byte);
      break;
    case 0xd200:				/* POKEY */
    case 0xe800:				/* POKEY - 5200 AAA added other pokey space */
    case 0xeb00:				/* POKEY - 5200 */
      POKEY_PutByte(addr, byte);
      break;
    case 0xd300:				/* PIA */
      PIA_PutByte(addr, byte);
      break;
    case 0xd400:				/* ANTIC */
      ANTIC_PutByte(addr, byte);
      break;
    case 0xd500:				/* bank-switching cartridges, RTIME-8 */
      CART_PutByte(addr, byte);
      break;
    default:
      break;
	}
}

void Atari800_Frame(void)
{
	INPUT_Frame();
	GTIA_Frame();
	ANTIC_Frame();
	POKEY_Frame();
}

void MainStateSave(void) {
	UBYTE temp;
	int default_tv_mode;
	int os = 0;
	int default_system = 6;
	int pil_on = FALSE;

	if (tv_mode == TV_PAL) {
		temp = 0;
		default_tv_mode = 1;
	}
	else {
		temp = 1;
		default_tv_mode = 2;
	}
	SaveUBYTE(&temp, 1);

	temp = 4;
	SaveUBYTE(&temp, 1);

	SaveINT(&os, 1);
	SaveINT(&pil_on, 1);
	SaveINT(&default_tv_mode, 1);
	SaveINT(&default_system, 1);
}

void MainStateRead(void) {
	/* these are all for compatibility with previous versions */
	UBYTE temp;
	int default_tv_mode;
	int os;
	int default_system;
	int pil_on;

	ReadUBYTE(&temp, 1);
	tv_mode = (temp == 0) ? TV_PAL : TV_NTSC;

	ReadUBYTE(&temp, 1);
	ReadINT(&os, 1);
	ReadINT(&pil_on, 1);
	ReadINT(&default_tv_mode, 1);
	ReadINT(&default_system, 1);
}

/* -------------------------------------------------------------------------- */
/* CONFIG & INITIALISATION                                                    */
/* -------------------------------------------------------------------------- */
void Atari_Initialise(void)
{
   unsigned i;

#ifdef SOUND
   /* Initialise sound routines */
#ifdef STEREO_SOUND
  Pokey_sound_init(FREQ_17_EXACT, SOUND_SAMPLE_RATE, 2, 0);
#else
  Pokey_sound_init(FREQ_17_EXACT, SOUND_SAMPLE_RATE, 1, 0);
#endif
#endif

   for (i = 0; i < 4; i++)
   {
      joy_5200_trig[i]  = 1;
      joy_5200_stick[i] = STICK_CENTRE;
      atari_analog[i]   = 0;
   }

   for (i = 0; i < 8; i++)
      joy_5200_pot[i] = JOY_5200_CENTER;

   key_consol = CONSOL_NONE;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_PORT(unsigned int num)
{
   /* num is port index
    * > port 0: controller 0, 1
    * > port 1: controller 2, 3 */
   switch (num)
   {
      case 0:
         return (joy_5200_stick[1] << 4) | joy_5200_stick[0];
      case 1:
         return (joy_5200_stick[3] << 4) | joy_5200_stick[2];
      default:
         break;
   }

   return (STICK_CENTRE << 4) | STICK_CENTRE;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_TRIG(unsigned int num)
{
   /* num is controller index */
   if (num < 4)
      return joy_5200_trig[num];

   return 1;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_POT(unsigned int num)
{
   /* num is analog axis index
    * > 0, 1 - controller 1: x, y
    * > 2, 3 - controller 2: x, y
    * > 4, 5 - controller 3: x, y
    * > 6, 7 - controller 4: x, y */
   if (num < 8)
      return joy_5200_pot[num];

   return JOY_5200_CENTER;
}

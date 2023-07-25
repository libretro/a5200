/*
 * pokey.c - POKEY sound chip emulation
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2005 Atari800 development team (see DOC/CREDITS)
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
#include <time.h>

#include "atari.h"
#include "cpu.h"
#include "pia.h"
#include "pokey.h"
#include "gtia.h"
#include "sio.h"
#include "input.h"
#include "statesav.h"
#ifdef SOUND
#include "pokeysnd.h"
#endif
#include "antic.h"

#define _STIMER 0x09
#define _SKRES 0x0a
#define _POTGO 0x0b
#define _SEROUT 0x0d
#define _IRQEN 0x0e
#define _SKCTLS 0x0f

#define _POT0 0x00
#define _POT1 0x01
#define _POT2 0x02
#define _POT3 0x03
#define _POT4 0x04
#define _POT5 0x05
#define _POT6 0x06
#define _POT7 0x07
#define _ALLPOT 0x08
#define _KBCODE 0x09
#define _RANDOM 0x0a
#define _SERIN 0x0d
#define _IRQST 0x0e
#define _SKSTAT 0x0f


UBYTE KBCODE;
UBYTE SERIN;
UBYTE IRQST;
UBYTE IRQEN;
UBYTE SKSTAT;
UBYTE SKCTLS;
int DELAYED_SERIN_IRQ;
int DELAYED_SEROUT_IRQ;
int DELAYED_XMTDONE_IRQ;

/* structures to hold the 9 pokey control bytes */
UBYTE AUDF[4 * MAXPOKEYS];	/* AUDFx (D200, D202, D204, D206) */
UBYTE AUDC[4 * MAXPOKEYS];	/* AUDCx (D201, D203, D205, D207) */
UBYTE AUDCTL[MAXPOKEYS];	/* AUDCTL (D208) */
int DivNIRQ[4], DivNMax[4];
int Base_mult[MAXPOKEYS];		/* selects either 64Khz or 15Khz clock mult */

UBYTE POT_input[8] = {228, 228, 228, 228, 228, 228, 228, 228};
UBYTE PCPOT_input[8] = {112, 112, 112, 112, 112, 112,112, 112};
UBYTE POT_all;
UBYTE pot_scanline;

UBYTE poly9_lookup[511];
UBYTE poly17_lookup[16385];
static uint32_t random_scanline_counter;

UBYTE POKEY_GetByte(UWORD addr)
{
	UBYTE byte = 0xff;

#ifdef STEREO_SOUND
	if (addr & 0x0010 && stereo_enabled)
		return 0;
#endif
	addr &= 0x0f;
	if (addr < 8)
   {
      byte = POT_input[addr];
      if (byte <= pot_scanline)
         return byte;
      return pot_scanline;
   }
	switch (addr)
   {
      case _ALLPOT:
         {
            unsigned int i;
            for (i = 0; i < 8; i++)
               if (POT_input[i] <= pot_scanline)
                  byte &= ~(1 << i);		// reset bit if pot value known 
         }
         return byte;
      case _KBCODE:
         if ( SKCTLS & 0x01 )
            return 0xff;
         return KBCODE | ((random_scanline_counter & 0x1)<<5);
      case _RANDOM:
         if ((SKCTLS & 0x03) != 0) {
            int i = random_scanline_counter + XPOS;
            if (AUDCTL[0] & POLY9)
               return poly9_lookup[i % POLY9_SIZE];
            {
               const UBYTE *ptr;
               i %= POLY17_SIZE;
               ptr = poly17_lookup + (i >> 3);
               i &= 7;
               byte = (UBYTE) ((ptr[0] >> i) + (ptr[1] << (8 - i)));
            }
         }
         break;
      case _SERIN:
         return SERIN;
      case _IRQST:
         return IRQST;
      case _SKSTAT:
         return SKSTAT + (1 << 4);
   }

	return byte;
}

/*****************************************************************************/
/* Module:  Update_Counter()                                                 */
/* Purpose: To process the latest control values stored in the AUDF, AUDC,   */
/*          and AUDCTL registers.  It pre-calculates as much information as  */
/*          possible for better performance.  This routine has been added    */
/*          here again as I need the precise frequency for the pokey timers  */
/*          again. The pokey emulation is therefore somewhat sub-optimal     */
/*          since the actual pokey emulation should grab the frequency values */
/*          directly from here instead of calculating them again.            */
/*                                                                           */
/* Author:  Ron Fries,Thomas Richter                                         */
/* Date:    March 27, 1998                                                   */
/*                                                                           */
/* Inputs:  chan_mask: Channel mask, one bit per channel.                    */
/*          The channels that need to be updated                             */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

static void Update_Counter(int chan_mask)
{
/************************************************************/
/* As defined in the manual, the exact Div_n_cnt values are */
/* different depending on the frequency and resolution:     */
/*    64 kHz or 15 kHz - AUDF + 1                           */
/*    1 MHz, 8-bit -     AUDF + 4                           */
/*    1 MHz, 16-bit -    AUDF[CHAN1]+256*AUDF[CHAN2] + 7    */
/************************************************************/

	/* only reset the channels that have changed */

	if (chan_mask & (1 << CHAN1)) {
		/* process channel 1 frequency */
		if (AUDCTL[0] & CH1_179)
			DivNMax[CHAN1] = AUDF[CHAN1] + 4;
		else
			DivNMax[CHAN1] = (AUDF[CHAN1] + 1) * Base_mult[0];
		if (DivNMax[CHAN1] < LINE_C)
			DivNMax[CHAN1] = LINE_C;
	}

	if (chan_mask & (1 << CHAN2)) {
		/* process channel 2 frequency */
		if (AUDCTL[0] & CH1_CH2) {
			if (AUDCTL[0] & CH1_179)
				DivNMax[CHAN2] = AUDF[CHAN2] * 256 + AUDF[CHAN1] + 7;
			else
				DivNMax[CHAN2] = (AUDF[CHAN2] * 256 + AUDF[CHAN1] + 1) * Base_mult[0];
		}
		else
			DivNMax[CHAN2] = (AUDF[CHAN2] + 1) * Base_mult[0];
		if (DivNMax[CHAN2] < LINE_C)
			DivNMax[CHAN2] = LINE_C;
	}

	if (chan_mask & (1 << CHAN4)) {
		/* process channel 4 frequency */
		if (AUDCTL[0] & CH3_CH4) {
			if (AUDCTL[0] & CH3_179)
				DivNMax[CHAN4] = AUDF[CHAN4] * 256 + AUDF[CHAN3] + 7;
			else
				DivNMax[CHAN4] = (AUDF[CHAN4] * 256 + AUDF[CHAN3] + 1) * Base_mult[0];
		}
		else
			DivNMax[CHAN4] = (AUDF[CHAN4] + 1) * Base_mult[0];
		if (DivNMax[CHAN4] < LINE_C)
			DivNMax[CHAN4] = LINE_C;
	}
}


static int POKEY_siocheck(void)
{
	return (AUDF[CHAN3] == 0x28 || AUDF[CHAN3] == 0x10
	        || AUDF[CHAN3] == 0x08 || AUDF[CHAN3] == 0x0a)
		&& AUDF[CHAN4] == 0x00 && (AUDCTL[0] & 0x28) == 0x28;
}

#ifndef SOUND_GAIN /* sound gain can be pre-defined in the configure/Makefile */
#define SOUND_GAIN 4
#endif

#ifndef SOUND
#define Update_pokey_sound(addr, val, chip, gain)
#endif

void POKEY_PutByte(UWORD addr, UBYTE byte)
{
#ifdef STEREO_SOUND
	addr &= stereo_enabled ? 0x1f : 0x0f;
#else
	addr &= 0x0f;
#endif
	switch (addr) {
	case OFFSET_AUDC1:
		AUDC[CHAN1] = byte;
		POKEYSND_Update(OFFSET_AUDC1, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDC2:
		AUDC[CHAN2] = byte;
		POKEYSND_Update(OFFSET_AUDC2, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDC3:
		AUDC[CHAN3] = byte;
		POKEYSND_Update(OFFSET_AUDC3, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDC4:
		AUDC[CHAN4] = byte;
		POKEYSND_Update(OFFSET_AUDC4, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDCTL:
		AUDCTL[0] = byte;

		/* determine the base multiplier for the 'div by n' calculations */
		if (byte & CLOCK_15)
			Base_mult[0] = DIV_15;
		else
			Base_mult[0] = DIV_64;

		Update_Counter((1 << CHAN1) | (1 << CHAN2) | (1 << CHAN3) | (1 << CHAN4));
		POKEYSND_Update(OFFSET_AUDCTL, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDF1:
		AUDF[CHAN1] = byte;
		Update_Counter((AUDCTL[0] & CH1_CH2) ? ((1 << CHAN2) | (1 << CHAN1)) : (1 << CHAN1));
		POKEYSND_Update(OFFSET_AUDF1, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDF2:
		AUDF[CHAN2] = byte;
		Update_Counter(1 << CHAN2);
		POKEYSND_Update(OFFSET_AUDF2, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDF3:
		AUDF[CHAN3] = byte;
		Update_Counter((AUDCTL[0] & CH3_CH4) ? ((1 << CHAN4) | (1 << CHAN3)) : (1 << CHAN3));
		POKEYSND_Update(OFFSET_AUDF3, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_AUDF4:
		AUDF[CHAN4] = byte;
		Update_Counter(1 << CHAN4);
		POKEYSND_Update(OFFSET_AUDF4, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_IRQEN:
		IRQEN = byte;
		IRQST |= ~byte & 0xf7;	/* Reset disabled IRQs except XMTDONE */
		if ((~IRQST & IRQEN) == 0)
			IRQ = 0;
		break;
	case OFFSET_SKRES:
		SKSTAT |= 0xe0;
		break;
	case OFFSET_POTGO:
    //POT_all = 0xFF;
		if (!(SKCTLS & 4)) {
			pot_scanline = 0;	/* slow pot mode */
    }  
    //else
    //  pot_scanline = 226;//228;  
      //INPUT_potgo(&POT_all);
		break;
	case OFFSET_SEROUT:
		if ((SKCTLS & 0x70) == 0x20 && POKEY_siocheck())
			SIO_PutByte(byte);
		DELAYED_SEROUT_IRQ = SEROUT_INTERVAL;
		IRQST |= 0x08;
		DELAYED_XMTDONE_IRQ = XMTDONE_INTERVAL;
		break;
	case OFFSET_STIMER:
		DivNIRQ[CHAN1] = DivNMax[CHAN1];
		DivNIRQ[CHAN2] = DivNMax[CHAN2];
		DivNIRQ[CHAN4] = DivNMax[CHAN4];
		POKEYSND_Update(_STIMER, byte, 0, SOUND_GAIN);
		break;
	case OFFSET_SKCTL:
		SKCTLS = byte;
    POKEYSND_Update(OFFSET_SKCTL, byte, 0, SOUND_GAIN);
		if (byte & 4)
			pot_scanline = 228;	/* fast pot mode - return results immediately */
		break;
#ifdef STEREO_SOUND
	case OFFSET_AUDC1 + OFFSET_POKEY2:
		AUDC[CHAN1 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDC1, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDC2 + OFFSET_POKEY2:
		AUDC[CHAN2 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDC2, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDC3 + OFFSET_POKEY2:
		AUDC[CHAN3 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDC3, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDC4 + OFFSET_POKEY2:
		AUDC[CHAN4 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDC4, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDCTL + OFFSET_POKEY2:
		AUDCTL[1] = byte;
		/* determine the base multiplier for the 'div by n' calculations */
		if (byte & CLOCK_15)
			Base_mult[1] = DIV_15;
		else
			Base_mult[1] = DIV_64;

		POKEYSND_Update(OFFSET_AUDCTL, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDF1 + OFFSET_POKEY2:
		AUDF[CHAN1 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDF1, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDF2 + OFFSET_POKEY2:
		AUDF[CHAN2 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDF2, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDF3 + OFFSET_POKEY2:
		AUDF[CHAN3 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDF3, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_AUDF4 + OFFSET_POKEY2:
		AUDF[CHAN4 + CHIP2] = byte;
		POKEYSND_Update(OFFSET_AUDF4, byte, 1, SOUND_GAIN);
		break;
	case OFFSET_STIMER + OFFSET_POKEY2:
		POKEYSND_Update(OFFSET_STIMER, byte, 1, SOUND_GAIN);
		break;
#endif
	}
}

void POKEY_Initialise(void)
{
	int i;
	uint32_t reg;

	/* Initialise Serial Port Interrupts */
	DELAYED_SERIN_IRQ = 0;
	DELAYED_SEROUT_IRQ = 0;
	DELAYED_XMTDONE_IRQ = 0;

	KBCODE = 0xff;
	SERIN = 0x00;	/* or 0xff ? */
	IRQST = 0xff;
	IRQEN = 0x00;
	SKSTAT = 0xef;
	SKCTLS = 0x00;

	for (i = 0; i < (MAXPOKEYS * 4); i++) {
		AUDC[i] = 0;
		AUDF[i] = 0;
	}

	for (i = 0; i < MAXPOKEYS; i++) {
		AUDCTL[i] = 0;
		Base_mult[i] = DIV_64;
	}

	for (i = 0; i < 4; i++)
		DivNIRQ[i] = DivNMax[i] = 0;

	pot_scanline = 0;

	/* initialise poly9_lookup */
	reg = 0x1ff;
	for (i = 0; i < 511; i++) {
		reg = ((((reg >> 5) ^ reg) & 1) << 8) + (reg >> 1);
		poly9_lookup[i] = (UBYTE) reg;
	}
	/* initialise poly17_lookup */
	reg = 0x1ffff;
	for (i = 0; i < 16385; i++) {
		reg = ((((reg >> 5) ^ reg) & 0xff) << 9) + (reg >> 8);
		poly17_lookup[i] = (UBYTE) (reg >> 1);
	}

	random_scanline_counter = time(NULL) % POLY17_SIZE;
}

void POKEY_Frame(void)
{
	random_scanline_counter %= (AUDCTL[0] & POLY9) ? POLY9_SIZE : POLY17_SIZE;
}

/***************************************************************************
 ** Generate POKEY Timer IRQs if required                                 **
 ** called on a per-scanline basis, not very precise, but good enough     **
 ** for most applications                                                 **
 ***************************************************************************/

void POKEY_Scanline(void)
{
	if (pot_scanline < 228)
		pot_scanline++;
  
  POT_input[0] = PCPOT_input[0]; POT_input[1] = PCPOT_input[1]; POT_input[2] = PCPOT_input[2]; POT_input[3] = PCPOT_input[3];
  
	random_scanline_counter += LINE_C;

	if (DELAYED_SEROUT_IRQ > 0) {
		if (--DELAYED_SEROUT_IRQ == 0) {
			if (IRQEN & 0x10) {
				IRQST &= 0xef;
				GenerateIRQ();
			}
		}
	}

	if (DELAYED_XMTDONE_IRQ > 0)
		if (--DELAYED_XMTDONE_IRQ == 0) {
			IRQST &= 0xf7;
			if (IRQEN & 0x08)
				GenerateIRQ();
		}

	if ((DivNIRQ[CHAN1] -= LINE_C) < 0 ) {
		DivNIRQ[CHAN1] += DivNMax[CHAN1];
		if (IRQEN & 0x01) {
			IRQST &= 0xfe;
			GenerateIRQ();
		}
	}

	if ((DivNIRQ[CHAN2] -= LINE_C) < 0 ) {
		DivNIRQ[CHAN2] += DivNMax[CHAN2];
		if (IRQEN & 0x02) {
			IRQST &= 0xfd;
			GenerateIRQ();
		}
	}

	if ((DivNIRQ[CHAN4] -= LINE_C) < 0 ) {
		DivNIRQ[CHAN4] += DivNMax[CHAN4];
		if (IRQEN & 0x04) {
			IRQST &= 0xfb;
			GenerateIRQ();
		}
	}
}

void POKEYStateSave(void)
{
	int SHIFT_KEY = 0;
	int KEYPRESSED = 0;
	UWORD random_scanline_counter_lo;
	UWORD random_scanline_counter_hi;

	SaveUBYTE(&KBCODE, 1);
	SaveUBYTE(&IRQST, 1);
	SaveUBYTE(&IRQEN, 1);
	SaveUBYTE(&SKCTLS, 1);

	SaveINT(&SHIFT_KEY, 1);
	SaveINT(&KEYPRESSED, 1);
	SaveINT(&DELAYED_SERIN_IRQ, 1);
	SaveINT(&DELAYED_SEROUT_IRQ, 1);
	SaveINT(&DELAYED_XMTDONE_IRQ, 1);

	SaveUBYTE(&AUDF[0], 4);
	SaveUBYTE(&AUDC[0], 4);
	SaveUBYTE(&AUDCTL[0], 1);

	SaveINT(&DivNIRQ[0], 4);
	SaveINT(&DivNMax[0], 4);
	SaveINT(&Base_mult[0], 1);

	/* random_scanline_counter (current value of
	 * the random number generator) must also be
	 * saved/restored for consistent operation
	 * (when using runahead, netplay. etc.) */
	random_scanline_counter_lo = random_scanline_counter         & 0xFFFF;
	random_scanline_counter_hi = (random_scanline_counter >> 16) & 0xFFFF;

	SaveUWORD(&random_scanline_counter_lo, 1);
	SaveUWORD(&random_scanline_counter_hi, 1);
}

void POKEYStateRead(void)
{
	int i;
	int SHIFT_KEY;
	int KEYPRESSED;
	UWORD random_scanline_counter_lo = 0;
	UWORD random_scanline_counter_hi = 0;

	ReadUBYTE(&KBCODE, 1);
	ReadUBYTE(&IRQST, 1);
	ReadUBYTE(&IRQEN, 1);
	ReadUBYTE(&SKCTLS, 1);

	ReadINT(&SHIFT_KEY, 1);
	ReadINT(&KEYPRESSED, 1);
	ReadINT(&DELAYED_SERIN_IRQ, 1);
	ReadINT(&DELAYED_SEROUT_IRQ, 1);
	ReadINT(&DELAYED_XMTDONE_IRQ, 1);

	ReadUBYTE(&AUDF[0], 4);
	ReadUBYTE(&AUDC[0], 4);
	ReadUBYTE(&AUDCTL[0], 1);
	for (i = 0; i < 4; i++) {
		POKEY_PutByte((UWORD) (OFFSET_AUDF1 + i * 2), AUDF[i]);
		POKEY_PutByte((UWORD) (OFFSET_AUDC1 + i * 2), AUDC[i]);
	}
	POKEY_PutByte(AUDCTL, AUDCTL[0]);

	ReadINT(&DivNIRQ[0], 4);
	ReadINT(&DivNMax[0], 4);
	ReadINT(&Base_mult[0], 1);

	/* random_scanline_counter (current value of
	 * the random number generator) must also be
	 * saved/restored for consistent operation
	 * (when using runahead, netplay. etc.) */
	ReadUWORD(&random_scanline_counter_lo, 1);
	ReadUWORD(&random_scanline_counter_hi, 1);

	random_scanline_counter = (random_scanline_counter_hi << 16) |
			random_scanline_counter_lo;
}

/*
 * LameNES - Nintendo Entertainment System (NES) emulator
 *
 * Copyright (c) 2005, Joey Loman, <joey@lamenes.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../3DO/core.h"

#include <filestreamfunctions.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lame6502/lame6502.h"

#include "lamenes.h"
#include "memory.h"
#include "romloader.h"

/* pointers to the nes headers */
unsigned char header[15];

unsigned char PRG; /* rom mem */
unsigned char CHR; /* vrom mem */

unsigned char MAPPER;

int RCB;
int OS_MIRROR = 0;
int FS_MIRROR;
int TRAINER;
int SRAM;
int MIRRORING;

char title[128];

int analyze_header(char *romfn)
{
	Stream *romfp;

	romfp = OpenDiskStream(romfn, 0);
	if (romfp == NULL)
	{
		return(1);
	}

	romlen = SeekDiskStream(romfp, 0, SEEK_END);

	SeekDiskStream(romfp, 0, SEEK_SET);

	ReadDiskStream(romfp, (char *)&header[0], 15);

	CloseDiskStream(romfp);


	/* ines rom header must be: "NES\n" (HEX: 4E 45 53 1A), else exit */
	if ((header[0] != 'N') || (header[1] != 'E') || (header[2] != 'S') || (header[3] != 0x1A))
	{
		free(header);
		return(1);
	}

	PRG = header[4];

	CHR = header[5];

	/* detect mapper lower bits */
	MAPPER = (header[6] >> 4);
	MAPPER |= (header[7] & 0xf0);

	/* detect mirroring, sram status,
	 * trainer status and four screen mirroring status.
	 */
	RCB = (header[6] - ((header[6] >> 4) << 4));

	switch (RCB) {
	case 0x00:
		/* horizontal mirroring only */
		MIRRORING = 0;
		SRAM = 0;
		TRAINER = 0;
		FS_MIRROR = 0;
		break;

	case 0x01:
		/* vertical mirroring only */
		MIRRORING = 1;
		SRAM = 0;
		TRAINER = 0;
		FS_MIRROR = 0;
		break;

	case 0x02:
		/* horizontal mirroring and sram enabled */
		MIRRORING = 0;
		SRAM = 1;
		TRAINER = 0;
		FS_MIRROR = 0;
		break;

	case 0x03:
		/* vertical mirroring and sram enabled */
		MIRRORING = 1;
		SRAM = 1;
		TRAINER = 0;
		FS_MIRROR = 0;
		break;

	case 0x04:
		/* horizontal mirroring and trainer on */
		MIRRORING = 0;
		SRAM = 0;
		TRAINER = 1;
		FS_MIRROR = 0;
		break;

	case 0x05:
		/* vertical mirroring and trainer on */
		MIRRORING = 1;
		SRAM = 0;
		TRAINER = 1;
		FS_MIRROR = 0;
		break;

	case 0x06:
		/* horizontal mirroring, sram enabled and trainer on */
		MIRRORING = 0;
		SRAM = 1;
		TRAINER = 1;
		FS_MIRROR = 0;
		break;

	case 0x07:
		/* vertical mirroring, sram enabled and trainer on */
		MIRRORING = 1;
		SRAM = 1;
		TRAINER = 1;
		FS_MIRROR = 0;
		break;

	case 0x08:
		/* horizontal mirroring and four screen vram on */
		MIRRORING = 0;
		SRAM = 0;
		TRAINER = 0;
		FS_MIRROR = 1;
		break;

	case 0x09:
		/* vertical mirroring and four screen vram on */
		MIRRORING = 1;
		SRAM = 0;
		TRAINER = 0;
		FS_MIRROR = 1;
		break;

	case 0x0A:
		/* horizontal mirroring, sram enabled and four screen vram on */
		MIRRORING = 0;
		SRAM = 1;
		TRAINER = 0;
		FS_MIRROR = 1;
		break;

	case 0x0B:
		/* vertical mirroring, sram enabled and four screen vram on */
		MIRRORING = 1;
		SRAM = 1;
		TRAINER = 0;
		FS_MIRROR = 1;
		break;

	case 0x0C:
		/* horizontal mirroring, trainer on and four screen vram on */
		MIRRORING = 0;
		SRAM = 0;
		TRAINER = 1;
		FS_MIRROR = 1;
		break;

	case 0x0D:
		/* vertical mirroring, trainer on and four screen vram on */
		MIRRORING = 1;
		SRAM = 0;
		TRAINER = 1;
		FS_MIRROR = 1;
		break;

	case 0x0E:
		/* horizontal mirroring, sram enabled, trainer on and four screen vram on */
		MIRRORING = 0;
		SRAM = 1;
		TRAINER = 1;
		FS_MIRROR = 1;
		break;

	case 0x0F:
		/* vertical mirroring, sram enabled, trainer on and four screen vram on */
		MIRRORING = 1;
		SRAM = 1;
		TRAINER = 1;
		FS_MIRROR = 1;
		break;

	default:

		break;
	}

	free(header);

	return(0);
}

int load_rom(char *romfn)
{
	Stream *romfp;

	romfp = OpenDiskStream(romfn, 0);
	if (romfp == NULL)
	{
		return(1);
	}

	ReadDiskStream(romfp, (char *)&romcache[0x0000], romlen);

	CloseDiskStream(romfp);


	/* load prg data in memory */
	if (PRG == 0x01)
	{
		/* map 16kb in mirror mode */
		memcpy(memory + 0x8000, romcache + 16, 16384);
		memcpy(memory + 0xC000, romcache + 16, 16384);
	}
	else
	{
		/* map 2x 16kb the first one into 8000 and the last one into c000 */
		memcpy(memory + 0x8000, romcache + 16, 16384);
		memcpy(memory + 0xC000, romcache + 16 + ((PRG - 1) * 16384), 16384);
	}

	/* load chr data in ppu memory */
	if (CHR != 0x00)
	{
		memcpy(ppu_memory, romcache + 16 + (PRG * 16384), 8192);

		/* fetch title from last 128 bytes */
		memcpy(title, romcache + 16 + (PRG * 16384) + 8192, 128);
	}

	return(0);
}

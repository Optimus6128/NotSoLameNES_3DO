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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lame6502/lame6502.h"
#include "lame6502/debugger.h"
#include "lame6502/disas.h"

#include "macros.h"
#include "lamenes.h"
#include "romloader.h"
#include "ppu.h"
#include "palette.h"
#include "nes_input.h"
#include "memory.h"

// included mappers
#include "mappers/mmc1.h"	// 1
#include "mappers/unrom.h"	// 2
#include "mappers/cnrom.h"	// 3
#include "mappers/mmc3.h"	// 4

#include "3DO/core.h"
#include "3DO/input.h"
#include "3DO/system_graphics.h"
#include "3DO/tools.h"

char *romsDirectory = "Roms";
uint32 fileCount;
char fileStr[16][20];

char romfn[256];

// cache the rom in memory to access the data quickly
unsigned char *romcache;

unsigned int start_int;
unsigned int vblank_int;
unsigned int vblank_cycle_timeout;
unsigned int scanline_refresh;

int systemType;

bool pause_emulation = false;

unsigned short NES_screen_width;
unsigned short NES_screen_height;

CCB *screenCel;
CCB *screenRowCel[32];
int scrollRowX[32];
uint16 palette3DO[256];

long romlen;

int frameskipNum = 0;
bool skipRendering = false;
bool skipCPU = false;


static void initNESscreenCELs()
{
	int y;
	const uint32 width = NES_screen_width + 8;
	const uint32 height = NES_screen_height + 8;

	screenCel = CreateCel(width, height, 16, CREATECEL_UNCODED, NULL);
	screenCel->ccb_Flags |= (CCB_LAST | CCB_BGND);
	screenCel->ccb_XPos = 32 << 16;

	for (y=0; y<32; ++y) {
		screenRowCel[y] = CreateCel(width, 8, 16, CREATECEL_UNCODED, (uint16*)screenCel->ccb_SourcePtr + y * width * 8);
		screenRowCel[y]->ccb_XPos = screenCel->ccb_XPos;
		screenRowCel[y]->ccb_YPos = (y * 8) << 16;
		screenRowCel[y]->ccb_Flags = screenCel->ccb_Flags & ~CCB_LAST;
		if (y!=0) LinkCel(screenRowCel[y-1], screenRowCel[y]);
	}
	screenRowCel[31]->ccb_Flags |= CCB_LAST;
}

static void initNESpal3DO()
{
	int i;
	for (i=0; i<256; ++i) {
		palette3DO[i] = MAKE_NES_TO_3DO_PAL(palette[i & 63].r, palette[i & 63].g, palette[i & 63].b);
	}
}

static void updateSmoothScrollingRow(int charLine)
{
	const uint32 scrollX = scrollRowX[charLine] & 7;
	CCB *rowCel = screenRowCel[charLine];

	rowCel->ccb_PRE0 = (rowCel->ccb_PRE0 & ~(255U << 24)) | (scrollX << 24);
	rowCel->ccb_PRE1 = (rowCel->ccb_PRE1 & ~PRE1_TLHPCNT_MASK) | (NES_screen_width + scrollX - 1);
}

static void updateSmoothScrolling()
{
	int y;
	for (y=0; y<32; ++y) {
		updateSmoothScrollingRow(y);
	}
}

static void drawNESscreenCELs()
{
	drawCels(screenRowCel[0]);
}

static void runEmulationFrame()
{
	static int frame = 0;

	unsigned short counter = 0;
	unsigned short scanline = 0;

	#ifdef PER_CHARLINE_RENDERER
		const uint32 lineStep = 8;
	#else
		const uint32 lineStep = 1;
	#endif

	bool skipThisFrame = frame || skipRendering;


	if (!skipCPU) {
		CPU_execute(start_int);

		// set ppu_status D7 to 1 and enter vblank
		ppu_status |= 0x80;
		write_memory(0x2002,ppu_status);

		counter += CPU_execute(12); // needed for some roms

		if(exec_nmi_on_vblank) {
			counter += NMI(counter);
		}

		counter += CPU_execute(vblank_cycle_timeout);

		// vblank ends (ppu_status D7) is set to 0, sprite_zero (ppu_status D6) is set to 0
		ppu_status &= 0x3F;

		// and write to mem
		write_memory(0x2002,ppu_status);

		loopyV = loopyT;
	}

	updateNesInput();
	
	for(scanline = 0; scanline < NES_screen_height; scanline+=lineStep) {
		if(!sprite_zero) {
			int i;
			for (i=0; i<lineStep; ++i) {
				const uint32 y = scanline + i;
				if (y > 0) check_sprite_hit(y);
			}
		}
		
		if (!skipThisFrame) {
			if ((scanline & 7) == 0) {
				scrollRowX[scanline >> 3] = loopyX & 7;
			}

			if (scanline == 0) updatePalmap32();
			//if ((scanline & 7) == 0) updatePalmap32();
			//if ((scanline & 15) == 0) updatePalmap32();

			update_scanline_values(scanline, lineStep);

			// We may not need the second check. Either a lame hack to position screen or it actually does have to do with different NES timings
			if (background_on && !(systemType == SYSTEM_NTSC && scanline < 8)) {
				render_background(scanline);
			}
		}

		if (!skipCPU) {
			counter += CPU_execute(lineStep*scanline_refresh);

			if(mmc3_irq_enable == 1) {
				int i;
				bool mmc3_has_irq = false;
				for (i=0; i<lineStep; ++i) {
					if(scanline + i == mmc3_irq_counter) {
						mmc3_has_irq = true;
						break;
					}
				}
				if(mmc3_has_irq) {
					IRQ(counter);
					mmc3_irq_counter--;
				}
			}
		}
	}

	if (!skipThisFrame) {
		render_sprites();
		updateSmoothScrolling();
	}

	// Draw Screen
	drawNESscreenCELs();

	/*if(!interrupt_flag) {
		counter += IRQ(counter);
	}*/

	frame = (frame + 1) % (frameskipNum + 1);
}

/*static void reset_emulation()
{
	if(load_rom(romfn) == 1) {
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		free(romcache);
		exit(1);
	}

	if(MAPPER == 4)
		mmc3_reset();

	CPU_reset();

	resetNesInput();

	runEmulationFrame();
}*/


void runEmu()
{
	int i;

	if (DEBUG_MEM_FREQS) {
		mr_nohw = 0;
		mr_hw = 0;
		mr_0x2002 = 0;
		mr_0x2007 = 0;
		mr_0x4016 = 0;
		mw_other = 0;
		mw_ppu = 0;
		mw_0x2002 = 0;
		mw_0x4014 = 0;
		mw_0x4016 = 0;
		mw_mirror_low = 0;

		mw_ppu_0x2000 = 0;
		mw_ppu_0x2001 = 0;
		mw_ppu_0x2003 = 0;
		mw_ppu_0x2004 = 0;
		mw_ppu_0x2005 = 0;
		mw_ppu_0x2006 = 0;
		mw_ppu_0x2007 = 0;
		mw_ppu_0x4014 = 0;
	}
	
	// Frameskip to speed up things for testing
	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		frameskipNum = (frameskipNum + 1) & 3;
		for (i=0; i<frameskipNum; ++i) {
			int c = i + 1;
			drawThickPixel(2*(i+1), 116, MakeRGB15(7 + (c << 3), 3 + (c << 2), c << 1));
		}
		for (i=frameskipNum; i<3; ++i) {
			drawThickPixel(2*(i+1), 116, 0);
		}
	}

	// No rendering emulation (to purely benchmark CPU)
	skipRendering = isJoyButtonPressed(JOY_BUTTON_RPAD);

	// Pause CPU execution (to benchmark rendering of the last frame only);
	skipCPU = isJoyButtonPressed(JOY_BUTTON_LPAD);

	if (!pause_emulation) {
		runEmulationFrame();
	}

	if (DEBUG_MEM_FREQS) {
		drawNumber(0, 8, mr_nohw);		// 1000
		drawNumber(0, 16, mr_hw);		// 1000

		drawNumber(0, 32, mr_0x2002);	// 1000
		drawNumber(0, 40, mr_0x2007);	// 0
		drawNumber(0, 48, mr_0x4016);	// 8


		drawNumber(0, 64, mw_other);	// 650
		drawNumber(0, 72, mw_ppu);		// 12

		drawNumber(0, 88, mw_0x2002);	// 2000
		drawNumber(0, 96, mw_0x4014);	// 1
		drawNumber(0, 104, mw_0x4016);	// 2
		drawNumber(0, 112, mw_mirror_low);	// 650

		drawNumber(0, 152, mw_ppu_0x2000);
		drawNumber(0, 160, mw_ppu_0x2001);
		drawNumber(0, 168, mw_ppu_0x2003);
		drawNumber(0, 176, mw_ppu_0x2004);
		drawNumber(0, 184, mw_ppu_0x2005);
		drawNumber(0, 192, mw_ppu_0x2006);
		drawNumber(0, 200, mw_ppu_0x2007);
		drawNumber(0, 208, mw_ppu_0x4014);
	}

	/*for (i=0; i<fileCount; ++i) {
		drawText(0, 8 + i*8, fileStr[i]);
	}
	drawNumber(0, 232, fileCount);*/
}

char *selectFileFromMenu()
{
	Directory      *dir;
	DirectoryEntry  de;

	Item dirItem = OpenDiskFile(romsDirectory);

    dir = OpenDirectoryItem(dirItem);

	fileCount = 0;
	while (ReadDirectory(dir, &de) >= 0)
	{
		if (!(de.de_Flags & FILE_IS_DIRECTORY)) {
			sprintf(fileStr[fileCount], "%s\n", de.de_FileName);
			fileCount++;
		}
	}
	CloseDirectory(dir);

	return "rom.nes";
}

void initLoad(char *filename)
{
	if(analyze_header(filename) == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		exit(1);
	}

	// rom cache memory
	romcache = (unsigned char *)AllocMem(romlen, MEMTYPE_ANY);

	if (load_rom(filename) == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		free(romcache);
		exit(1);
	}
}

void initEmu()
{
	// cpu speed
	unsigned int NTSC_SPEED = 1789725;
	unsigned int PAL_SPEED = 1773447;

	// screen width
	#define NES_SCREEN_WIDTH 256
	
	// screen height
	#define NTSC_HEIGHT 224
	#define PAL_HEIGHT 240

	// screen total height
	#define NTSC_TOTAL_HEIGHT 261
	#define PAL_TOTAL_HEIGHT 313

	// vblank int
	unsigned short NTSC_VBLANK_INT = NTSC_SPEED / 60;
	unsigned short PAL_VBLANK_INT = PAL_SPEED / 50;

	// scanline refresh (hblank)
	unsigned short NTSC_SCANLINE_REFRESH = NTSC_VBLANK_INT / NTSC_TOTAL_HEIGHT;
	unsigned short PAL_SCANLINE_REFRESH = PAL_VBLANK_INT / PAL_TOTAL_HEIGHT;

	// vblank int cycle timeout
	unsigned int NTSC_VBLANK_CYCLE_TIMEOUT = (NTSC_TOTAL_HEIGHT-NTSC_HEIGHT) * NTSC_VBLANK_INT / NTSC_TOTAL_HEIGHT;
	unsigned int PAL_VBLANK_CYCLE_TIMEOUT = (PAL_TOTAL_HEIGHT-PAL_HEIGHT) * PAL_VBLANK_INT / PAL_TOTAL_HEIGHT;


	char *filename = selectFileFromMenu();

	if (filename) {
		initLoad(filename);
	} else {
		exit(1);
	}


	if (MAPPER == 4) {
		mmc3_reset();
	}

	// Forcing it to PAL for now, probably I have to detect the type from the ROM loaded in the future
	systemType = SYSTEM_PAL;

	NES_screen_width = NES_SCREEN_WIDTH;
	if(systemType == SYSTEM_PAL) {
		NES_screen_height = PAL_HEIGHT;
	} else if(systemType == SYSTEM_NTSC) {
		NES_screen_height = NTSC_HEIGHT;
	} else return;

	init_ppu();

	initNESscreenCELs();
	initNESpal3DO();

	// first reset the cpu at poweron
	CPU_reset();

	// reset joystick
	resetNesInput();

	if(systemType == SYSTEM_PAL) {
		start_int = 341;
		vblank_int = PAL_VBLANK_INT;
		vblank_cycle_timeout = PAL_VBLANK_CYCLE_TIMEOUT;
		scanline_refresh = PAL_SCANLINE_REFRESH;
	} else if(systemType == SYSTEM_NTSC) {
		start_int = 325;
		vblank_int = NTSC_VBLANK_INT;
		vblank_cycle_timeout = NTSC_VBLANK_CYCLE_TIMEOUT;
		scanline_refresh = NTSC_SCANLINE_REFRESH;
	}
}

int main()
{
	coreInit(initEmu, CORE_VRAM_SINGLEBUFFER | CORE_SHOW_FPS | CORE_NO_CLEAR_FRAME | /*CORE_SHOW_MEM | */CORE_NO_VSYNC);
	coreRun(runEmu);
}

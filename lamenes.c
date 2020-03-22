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

#include "3DO/core.h"
#include "3DO/input.h"
#include "3DO/system_graphics.h"
#include "3DO/tools.h"

#define MAX_FILES_NUM 1024
#define MAX_FILENAME_LENGTH 256


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

int renderer = RENDERER_PER_TILE;


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

	bool skipThisFrame = frame || skipRendering;

	uint32 lineStep = 1;
	if (renderer!=RENDERER_PER_LINE) {
		lineStep = 8;
	}

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
	skipRendering = isJoyButtonPressed(JOY_BUTTON_LPAD);

	// Pause CPU execution (to benchmark rendering of the last frame only);
	//skipCPU = isJoyButtonPressed(JOY_BUTTON_RPAD);

	// I will steal this button to toggle between the more accurate and the faster renderer
	if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
		uint16 color = 0;
		if (renderer == RENDERER_PER_LINE) {
			color = MakeRGB15(7, 31, 15);
			renderer = RENDERER_PER_TILE;
		} else {
			renderer = RENDERER_PER_LINE;
		}
		drawThickPixel(158, 2, color);
	}

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
}

int returnStringLength(char *str)
{
	int i = 0;
	while (i < MAX_FILENAME_LENGTH) {
		if (*str++ == 0) return i;
		++i;
	}
	return -1;
}

void showHelpScreen()
{
	const uint16 titleColor = MakeRGB15(31, 24, 15);
	const uint16 textColor = MakeRGB15(24, 28, 31);
	const uint16 specialColor = MakeRGB15(7, 31, 7);
	const uint16 bluerColor = MakeRGB15(7, 15, 31);
	const uint16 bugoColor = MakeRGB15(31, 15, 7);

	while(!wasAnyJoyButtonPressed()) {

		updateInput();

		setTextColor(titleColor);
		drawTextX2(32, 16, "Not So LameNES");
		drawTextX2(32, 32, "--------------");

		setTextColor(textColor);
		drawText(8, 64, "Here are the instructions..");

		setTextColor(specialColor);	drawText(0, 80, "DPAD, A, B, select start");
		setTextColor(textColor);	drawText(8, 88, "matches the NES gamepad");

		setTextColor(specialColor);	drawText(0, 104, "Press C");
		setTextColor(textColor);	drawText(8, 112, "toggle between 0-3 frameskip");
									drawText(8, 120, "look dots on the lower left");

		setTextColor(specialColor);	drawText(0, 136, "Hold LPAD");
		setTextColor(textColor);	drawText(8, 144, "pauses rendering (CPU only running)");

		setTextColor(specialColor);	drawText(0, 160, "Press RPAD");
		setTextColor(textColor);	drawText(8, 168, "switch faster renderer (incompatible)");
									drawText(8, 176, "Works with Mario but fails with rest");
									drawText(8, 184, "dot in upper right if fast renderer on");

		setTextColor(bluerColor);
		drawText(8, 204, "That's all folks!");
		drawText(16, 212, "Press any button to continue!");
		setTextColor(bugoColor); drawText(96, 230, "Bugo the Cat signing off..");

		displayScreen();
	}
	clearAllBuffers();
}

char *selectFileFromMenu()
{
	int i;
	char *romsDirectory = "Roms";
	uint32 fileCount;
	char *fileStr[MAX_FILES_NUM];

	Directory      *dir;
	DirectoryEntry  de;
	char *selectedRom = NULL;
	char dirRom[MAX_FILENAME_LENGTH + 4 + 1];

	Item dirItem = OpenDiskFile(romsDirectory);
	int selectIndex = 0;
	const int maxFilesPerScreen = 30;
	bool firstUpdate = true;
	int prevFilePage = 0;
	
	const uint16 colorWhite = MakeRGB15(31, 31, 31);
	const uint16 colorGrey = MakeRGB15(15, 15, 15);

	// Read all the files from the Roms directory
    dir = OpenDirectoryItem(dirItem);
	fileCount = 0;
	while (ReadDirectory(dir, &de) >= 0 && fileCount < MAX_FILES_NUM)
	{
		if (!(de.de_Flags & FILE_IS_DIRECTORY)) {
			const int fileStrLen = returnStringLength(de.de_FileName);
			if (fileStrLen > 0 && fileStrLen <= MAX_FILENAME_LENGTH) {
				fileStr[fileCount] = (char*)AllocMem(fileStrLen, MEMTYPE_TRACKSIZE);
				sprintf(fileStr[fileCount], "%s\0", de.de_FileName);
				fileCount++;
			}
		}
	}
	CloseDirectory(dir);

	// Main rom selection menu
	while(!selectedRom) {

		updateInput();

		if (isJoyButtonPressedOnce(JOY_BUTTON_DOWN)) {
			if (selectIndex < fileCount-1) ++selectIndex;
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_UP)) {
			if (selectIndex > 0) --selectIndex;
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_LPAD)) {
			selectIndex -= maxFilesPerScreen;
			if (selectIndex < 0) selectIndex = 0;
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_RPAD)) {
			selectIndex += maxFilesPerScreen;
			if (selectIndex > fileCount-1) selectIndex = fileCount-1;
		}
		if (isJoyButtonPressedOnce(JOY_BUTTON_A)) {
			selectedRom = fileStr[selectIndex];
		}

		if (wasAnyJoyButtonPressed() || firstUpdate) {
			const int filePage = selectIndex / maxFilesPerScreen;
			const int pageStartIndex = filePage * maxFilesPerScreen;

			if (filePage != prevFilePage) clearAllBuffers();
			prevFilePage = filePage;

			for (i=0; i<maxFilesPerScreen; ++i) {
				const int realIndex = pageStartIndex + i;

				uint16 color = colorGrey;
				if (selectIndex == realIndex) {
					color = colorWhite;
				}

				if (realIndex < fileCount) {
					setTextColor(color);
					drawText(0, i*8, fileStr[realIndex]);
				}
			}

			displayScreen();
			firstUpdate = false;
		}
	}


	clearAllBuffers();
	setTextColor(colorWhite);

	// Create the full rom folder/file string
	sprintf(dirRom, "%s/%s\0", romsDirectory, selectedRom);

	// Free all the filename strings allocated
	for (i=0; i<fileCount; ++i) {
		FreeMem(fileStr[i], -1);
	}

	return dirRom;
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
	
	char *filename = NULL;
	

	showHelpScreen();

	filename = selectFileFromMenu();
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
	coreInit(initEmu, CORE_VRAM_SINGLEBUFFER | CORE_NO_CLEAR_FRAME | CORE_SHOW_FPS | CORE_NO_VSYNC);
	coreRun(runEmu);
}

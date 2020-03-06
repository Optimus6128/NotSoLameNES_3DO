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

char romfn[256];

// cache the rom in memory to access the data quickly
unsigned char *romcache;

unsigned int start_int;
unsigned int vblank_int;
unsigned int vblank_cycle_timeout;
unsigned int scanline_refresh;

int systemType;

unsigned char pause_emulation = 0;

unsigned short NES_screen_width;
unsigned short NES_screen_height;

CCB *screenCel;
uint16 palette3DO[NUM_PAL_COLORS];

long romlen;

int frameskipNum = 0;
bool skipBackgroundRendering = false;

static void runEmulationFrame()
{
	static int frame = 0;

	unsigned short counter = 0;
	unsigned short scanline = 0;

	bool skipThisFrame = frame || skipBackgroundRendering;


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

	updateNesInput();
	
	for(scanline = 0; scanline < NES_screen_height; scanline++) {
		if(!sprite_zero && scanline > 0) {
			check_sprite_hit(scanline);
		}

		if (!skipThisFrame)
			render_background(scanline);

		counter += CPU_execute(scanline_refresh);

		if(mmc3_irq_enable == 1) {
			if(scanline == mmc3_irq_counter) {
				IRQ(counter);
				mmc3_irq_counter--;
			}
		}
	}

	if (!skipThisFrame)
		render_sprites();

	// Draw Screen
	drawCels(screenCel);

	/*if(!interrupt_flag) {
		counter += IRQ(counter);
	}*/

	frame = (frame + 1) % (frameskipNum + 1);
}

static void runEmulationFrameOnly()
{
	unsigned short scanline = 0;

	updateNesInput();

	for(scanline = 0; scanline < NES_screen_height; scanline++) {
		if(!sprite_zero && scanline > 0) {
			check_sprite_hit(scanline);
		}

		render_background(scanline);
	}

	render_sprites();

	// Draw Screen
	drawCels(screenCel);
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

static void initNESscreenCEL()
{
	screenCel = CreateCel(NES_screen_width + 8, NES_screen_height + 16, 16, CREATECEL_UNCODED, NULL);
	screenCel->ccb_Flags |= (CCB_LAST | CCB_BGND);
	screenCel->ccb_XPos = 32 << 16;
	screenCel->ccb_PRE1 = (screenCel->ccb_PRE1 &= ~PRE1_TLHPCNT_MASK) | (NES_screen_width - 1);
}

static void initNESpal3DO()
{
	int i;
	for (i=0; i<64; ++i) {
		palette3DO[i] = MAKE_NES_TO_3DO_PAL(palette[i].r, palette[i].g, palette[i].b);
	}
}

void runEmu()
{
	// Frameskip to speed up things for testing
	if (isJoyButtonPressedOnce(JOY_BUTTON_C)) {
		frameskipNum = (frameskipNum + 1) & 3;
		setBackgroundColor((frameskipNum + 1) * (frameskipNum + 2));
	}

	// No rendering emulation (to purely benchmark CPU)
	skipBackgroundRendering = isJoyButtonPressed(JOY_BUTTON_RPAD);

	if (!pause_emulation) {
		if (!isJoyButtonPressed(JOY_BUTTON_LPAD))
			runEmulationFrame();
		else
			runEmulationFrameOnly();	// No CPU update (to purely benchmark rendering)
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


	if(analyze_header("rom.nes") == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		exit(1);
	}

	// rom cache memory
	romcache = (unsigned char *)AllocMem(romlen, MEMTYPE_ANY);

	if (load_rom("rom.nes") == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		free(romcache);
		exit(1);
	}

	if (MAPPER == 4)
	{
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

	initNESscreenCEL();
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
	coreInit(initEmu, CORE_SHOW_FPS | CORE_NO_VSYNC);
	coreRun(runEmu);
}

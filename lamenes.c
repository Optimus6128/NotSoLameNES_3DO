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
#include "input.h"
#include "memory.h"

#include "lib/str_chrchk.h"
#include "lib/str_replace.h"

// included mappers
#include "mappers/mmc1.h"	// 1
#include "mappers/unrom.h"	// 2
#include "mappers/cnrom.h"	// 3
#include "mappers/mmc3.h"	// 4

#include "3DO/GestionAffichage.h"
#include "3DO/GestionSprites.h"
#include "3DO/GestionTextes.h"
#include <graphics.h>


char romfn[256];

// cache the rom in memory to access the data quickly
unsigned char *romcache;

unsigned int start_int;
unsigned int vblank_int;
unsigned int vblank_cycle_timeout;
unsigned int scanline_refresh;

int systemType;

unsigned char CPU_is_running = 1;
unsigned char pause_emulation = 0;

unsigned short width;
unsigned short height;

CCB *screenCel;

long romlen;

void check_button_events()
{
	uint32	gButtons;
	unsigned char i;

	DoControlPad(1, &gButtons, (ControlUp | ControlDown | ControlLeft | ControlRight));

	for (i=0;i<9;i++)
		clear_input((char *) i);

	if (gButtons & ControlDown) {
		set_input((char *) 1);
	}

	if (gButtons & ControlUp) {
		set_input((char *) 2);
	}

	if (gButtons & ControlLeft) {
		set_input((char *) 3);
	}

	if (gButtons & ControlRight) {
		set_input((char *) 4);
	}

	if (gButtons & ControlStart) {
		set_input((char *) 5);
	}

	//	Select
	if (gButtons & ControlC) {
		set_input((char *) 6);
	}

	if (gButtons & ControlA) {
		set_input((char *) 7);
	}

	if (gButtons & ControlB) {
		set_input((char *) 8);
	}
}

void start_emulation()
{
	unsigned short counter = 0;
	unsigned short scanline = 0;


	while(CPU_is_running) {
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

		for(scanline = 0; scanline < height; scanline++) {
			if(!sprite_zero) {
				check_sprite_hit(scanline);
			}

			render_background(scanline);

			counter += CPU_execute(scanline_refresh);

			if(mmc3_irq_enable == 1) {
				if(scanline == mmc3_irq_counter) {
					IRQ(counter);
					mmc3_irq_counter--;
				}
			}
		}

		render_sprites();

		drawNESscreenCEL();
		affichageMiseAJour();

		/*
		if(!interrupt_flag) 
		{
			counter += IRQ(counter);
		}
		*/
		
		check_button_events();
	}
}

void reset_emulation()
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

	reset_input();

	start_emulation();
}

void quit_emulation()
{
	// free all memory
	free(sprite_memory);
	free(ppu_memory);
	free(memory);
	free(romcache);

	exit(0);
}

static void initNESscreenCEL()
{
	//int x,y;
	//uint16 *dst;

	screenCel = CreateCel(width + 8, height + 8, 16, CREATECEL_UNCODED, NULL);
	screenCel->ccb_Flags |= (CCB_LAST | CCB_BGND);
	screenCel->ccb_XPos = 32 << 16;

	/*dst = (uint16*)screenCel->ccb_SourcePtr;
	for (y=0; y<screenCel->ccb_Height; ++y) {
		for (x=0; x<screenCel->ccb_Width; ++x) {
			*dst++ = x ^ y;
		}
	}*/
}

static void initNESpal3DO()
{
	int i;
	for (i=0; i<64; ++i) {
		palette3DO[i] = MakeRGB15(palette[i].r, palette[i].g, palette[i].b);
	}
}

int main(void)
{
	// cpu speed
	unsigned int NTSC_SPEED = 1789725;
	unsigned int PAL_SPEED = 1773447;

	// screen width
	#define SCREEN_WIDTH 256
	
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


	// 64k main memory
	memory = (unsigned char *)malloc(65536);

	// 16k video memory
	ppu_memory = (unsigned char *)malloc(16384);

	// 256b sprite memory
	sprite_memory = (unsigned char *)malloc(256);

	if(analyze_header("rom.nes") == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		exit(1);
	}

	// rom cache memory
	romcache = (unsigned char *)malloc(romlen);

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

	width = 256;
	if(systemType == SYSTEM_PAL) {
		height = PAL_HEIGHT;
	} else if(systemType == SYSTEM_NTSC) {
		height = NTSC_HEIGHT;
	} else return -1;


	affichageInitialisation();
	InitializeControlPads();
	affichageRendu();

	initNESscreenCEL();
	initNESpal3DO();


	// first reset the cpu at poweron
	CPU_reset();

	// reset joystick
	reset_input();


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

	while(1) 
	{
		if (!pause_emulation)
		{
			start_emulation();
		}
	}

	return(0);
}

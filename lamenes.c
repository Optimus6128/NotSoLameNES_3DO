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
/*#include <types.h>*/

#include "lame6502/lame6502.h"
#include "lame6502/debugger.h"
#include "lame6502/disas.h"

#include "macros.h"
#include "lamenes.h"
#include "romloader.h"
#include "ppu.h"
#include "palette.h"
#include "input.h"
#include "sdl_functions.h"

#include "lib/str_chrchk.h"
#include "lib/str_replace.h"

/* included mappers */
#include "mappers/mmc1.h"	/* 1 */
#include "mappers/unrom.h"	/* 2 */
#include "mappers/cnrom.h"	/* 3 */
#include "mappers/mmc3.h"	/* 4 */

#ifdef PC
	#include <SDL.h>
#else
	#include "3DO/GestionAffichage.h"
	#include "3DO/GestionSprites.h"
	#include "3DO/GestionTextes.h"
	#include <graphics.h>
#endif


char romfn[256];

/* cache the rom in memory to access the data quickly */
unsigned char *romcache;

unsigned char *ppu_memory;
unsigned char *sprite_memory;

unsigned int pad1_data;
int pad1_readcount = 0;

unsigned int start_int;
unsigned int vblank_int;
unsigned int vblank_cycle_timeout;
unsigned int scanline_refresh;

int systemType;

unsigned char CPU_is_running = 1;
unsigned char pause_emulation = 0;

unsigned short height;
unsigned short width;

unsigned short sdl_screen_height;
unsigned short sdl_screen_width;

unsigned char enable_background = 1;
unsigned char enable_sprites = 1;

unsigned char fullscreen = 0;
unsigned char scale = 1;

unsigned char frameskip = 0;
unsigned char skipframe = 0;

CCB *screenCel;

/*int sdl_delay = 0;*/

char *savfile;
char *statefile;

long romlen;

/*FILE *sav_fp;*/

void open_sav()
{
/*
	sav_fp = fopen(savfile,"rb");

	if(sav_fp) {
		printf("[*] %s found! loading into sram...\n",savfile);
		fseek(sav_fp,0,SEEK_SET);
		fread(&memory[0x6000],1,8192,sav_fp);

		fclose(sav_fp);
	}*/
}

void write_sav()
{/*
	sav_fp = fopen(savfile,"wb");
	fwrite(&memory[0x6000],1,8192,sav_fp);
	fclose(sav_fp);*/
}

void
load_state()
{
/*
	int pf;

	FILE *lst_fp;

	fprintf(stdout,"[*] loading state from file: %s\n",statefile);

	lst_fp = fopen(statefile,"rb");

	if(lst_fp) {
		fread(&program_counter,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,2,SEEK_CUR);
		fread(&stack_pointer,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,3,SEEK_CUR);
		fread(&status_register,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,4,SEEK_CUR);
		fread(&x_reg,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,5,SEEK_CUR);
		fread(&y_reg,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,6,SEEK_CUR);
		fread(&accumulator,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,7,SEEK_CUR);
		fread(&pf,1,1,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,8,SEEK_CUR);
		fread(&ppu_control1,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,10,SEEK_CUR);
		fread(&ppu_control2,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,13,SEEK_CUR);
		fread(&ppu_addr_h,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,14,SEEK_CUR);
		fread(&ppu_addr,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,16,SEEK_CUR);
		fread(&ppu_addr_tmp,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,18,SEEK_CUR);
		fread(&ppu_status,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,20,SEEK_CUR);
		fread(&ppu_status_tmp,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,22,SEEK_CUR);
		fread(&sprite_address,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,24,SEEK_CUR);
		fread(&ppu_bgscr_f,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,26,SEEK_CUR);
		fread(&loopyT,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,28,SEEK_CUR);
		fread(&loopyV,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,30,SEEK_CUR);
		fread(&loopyX,1,2,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,32,SEEK_CUR);
		fread(&memory[0x0000],1,65536,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,65568,SEEK_CUR);
		fread(&ppu_memory[0x0000],1,16384,lst_fp);

		fseek(lst_fp,0,SEEK_SET);
		fseek(lst_fp,81952,SEEK_CUR);
		fread(&sprite_memory[0x0000],1,256,lst_fp);

		SET_SAR(pf)

		fclose(lst_fp);

		fprintf(stdout,"[*] done!\n");
	} else {
		fprintf(stdout,"[!] no save state file found!\n");
	}*/
}

void
save_state()
{
	/*int pf;

	FILE *sst_fp;

	fprintf(stdout,"[*] saving state to file: %s\n",statefile);

	sst_fp = fopen(statefile,"wb");

	pf = ((sign_flag ? 0x80 : 0) |
		(zero_flag ? 0x02 : 0) |
		(carry_flag ? 0x01 : 0) |
		(interrupt_flag ? 0x04 : 0) |
		(decimal_flag ? 0x08 : 0) |
		(overflow_flag ? 0x40 : 0) |
		(break_flag ? 0x10 : 0) | 0x20);

	fwrite(&program_counter,1,2,sst_fp);
	fwrite(&stack_pointer,1,1,sst_fp);
	fwrite(&status_register,1,1,sst_fp);
	fwrite(&x_reg,1,1,sst_fp);
	fwrite(&y_reg,1,1,sst_fp);
	fwrite(&accumulator,1,1,sst_fp);
	fwrite(&pf,1,1,sst_fp);

	fwrite(&ppu_control1,1,2,sst_fp);
	fwrite(&ppu_control2,1,2,sst_fp);
	fwrite(&ppu_addr_h,1,2,sst_fp);
	fwrite(&ppu_addr,1,2,sst_fp);
	fwrite(&ppu_addr_tmp,1,2,sst_fp);
	fwrite(&ppu_status,1,2,sst_fp);
	fwrite(&ppu_status_tmp,1,2,sst_fp);
	fwrite(&sprite_address,1,2,sst_fp);
	fwrite(&ppu_bgscr_f,1,2,sst_fp);

	fwrite(&loopyT,1,2,sst_fp);
	fwrite(&loopyV,1,2,sst_fp);
	fwrite(&loopyX,1,2,sst_fp);

	fwrite(&memory[0x0000],1,65536,sst_fp);
	fwrite(&ppu_memory[0x0000],1,16384,sst_fp);
	fwrite(&sprite_memory[0x0000],1,256,sst_fp);

	fclose(sst_fp);

	fprintf(stdout,"[*] done!\n");*/
}

/*
 * memory read handler
 */
unsigned char memory_read(unsigned int address) {
	/* this is ram or rom so we can return the address */
	if(address < 0x2000 || address > 0x7FFF)
		return memory[address];

	/* the addresses between 0x2000 and 0x5000 are for input/ouput */
	if(address == 0x2002) {
		ppu_status_tmp = ppu_status;

		/* set ppu_status (D7) to 0 (vblank_on) */
		ppu_status &= 0x7F;
		write_memory(0x2002,ppu_status);

		/* set ppu_status (D6) to 0 (sprite_zero) */
		ppu_status &= 0x1F;
		write_memory(0x2002,ppu_status);

		/* reset VRAM Address Register #1 */
		ppu_bgscr_f = 0x00;

		/* reset VRAM Address Register #2 */
		ppu_addr_h = 0x00;

		/* return bits 7-4 of unmodifyed ppu_status with bits 3-0 of the ppu_addr_tmp */
		return (ppu_status_tmp & 0xE0) | (ppu_addr_tmp & 0x1F);
	}

	if(address == 0x2007) {
		tmp = ppu_addr_tmp;
		ppu_addr_tmp = ppu_addr;

		if(increment_32 == 0) {
			ppu_addr++;
		} else {
			ppu_addr += 0x20;
		}

		return ppu_memory[tmp];
	}

	/* pAPU data (sound) */
	/*
	if(address == 0x4015) {
		return memory[address];
	}
	*/
	
	/* joypad1 data */
	if(address == 0x4016) {
		switch(pad1_readcount) {
			case 0:
			memory[address] = pad1_A;
			pad1_readcount++;
			break;

			case 1:
			memory[address] = pad1_B;
			pad1_readcount++;
			break;

			case 2:
			memory[address] = pad1_SELECT;
			pad1_readcount++;
			break;

			case 3:
			memory[address] = pad1_START;
			pad1_readcount++;
			break;

			case 4:
			memory[address] = pad1_UP;
			pad1_readcount++;
			break;

			case 5:
			memory[address] = pad1_DOWN;
			pad1_readcount++;
			break;

			case 6:
			memory[address] = pad1_LEFT;
			pad1_readcount++;
			break;

			case 7:
			memory[address] = pad1_RIGHT;
			pad1_readcount = 0;
			break;
		}

		return memory[address];
	}

	if(address == 0x4017) {
		return memory[address];
	}

	return memory[address];
}

/*
 * memory write handler
 */
void
write_memory(unsigned int address,unsigned char data)
{

	/* PPU Status */
	if(address == 0x2002) {
		memory[address] = data;
		return;
	}

	/* PPU Video Memory area */
	if(address > 0x1fff && address < 0x4000) {
		write_ppu_memory(address,data);
		return;
	}

	/* Sprite DMA Register */
	if(address == 0x4014) {
		write_ppu_memory(address,data);
		return;
	}

	/* Joypad 1 */
	if(address == 0x4016) {
		memory[address] = 0x40;

		return;
	}

	/* Joypad 2 */
	/*if(address == 0x4017) {
		memory[address] = 0x48;
		return;
	}*/

	/* pAPU Sound Registers */
	/*
	if(address > 0x3fff && address < 0x4016) {
		write_sound(address,data); not emulated yet!
		memory[address] = data;
		return;
	}
	*/
	/* SRAM Registers */
	/*
	if(address > 0x5fff && address < 0x8000) {
		if(SRAM == 1)
			write_sav();

		memory[address] = data;
		return;
	}
	*/
	/* RAM registers */
	if(address < 0x2000) {
		memory[address] = data;
		memory[address+2048] = data; /* mirror of 0-800 */
		memory[address+4096] = data; /* mirror of 0-800 */
		memory[address+6144] = data; /* mirror of 0-800 */
		return;
	}
/*
	if(MAPPER == 1) {
		mmc1_access(address,data);
		return;
	}

	if(MAPPER == 2) {
		unrom_access(address,data);
		return;
	}

	if(MAPPER == 3) {
		cnrom_access(address,data);
		return;
	}

	if(MAPPER == 4) {
		mmc3_access(address,data);
		return;
	}
	*/
	memory[address] = data;
}

void
show_header()
{
	/*fprintf(stdout,"\n"
	"************************************************************\n"
	"*** LameNES version 0.1 by Joey Loman <joey@lamenes.org> ***\n"
	"************************************************************\n"
	"\n"
	);*/
}

void start_emulation()
{
	unsigned short counter = 0;
	unsigned short scanline = 0;


	while(CPU_is_running) {
		CPU_execute(start_int);

		/* set ppu_status D7 to 1 and enter vblank */
		ppu_status |= 0x80;
		write_memory(0x2002,ppu_status);

		counter += CPU_execute(12); /* needed for some roms */

		if(exec_nmi_on_vblank) {
			counter += NMI(counter);
		}

		counter += CPU_execute(vblank_cycle_timeout);

		/* vblank ends (ppu_status D7) is set to 0, sprite_zero (ppu_status D6) is set to 0 */
		ppu_status &= 0x3F;
	
		/* and write to mem */
		write_memory(0x2002,ppu_status);

		loopyV = loopyT;

		if(skipframe > frameskip)
			skipframe = 0;

		screen_lock();
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

					/*break;*/
				}
			}
		}

		render_sprites();
		screen_unlock();

		if(skipframe == 0) {
			#ifdef PC
				SDL_Flip(screen);
			#else
				drawNESscreenCEL();
				affichageMiseAJour();
			#endif
		}

		skipframe++;

		/*
		if(!interrupt_flag) 
		{
			counter += IRQ(counter);
		}
		*/

		check_SDL_event();
	}
}

void reset_emulation()
{
	/*printf("[*] resetting emulation...\n");*/

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
	/* free all memory */
	free(sprite_memory);
	free(ppu_memory);
	free(memory);
	free(romcache);

	/*printf("[!] quiting LameNES!\n\n");*/

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
	/* cpu speed */
	unsigned int NTSC_SPEED = 1789725;
	unsigned int PAL_SPEED = 1773447;

	/* screen width */
	#define SCREEN_WIDTH 256
	
	/* screen height */
	#define NTSC_HEIGHT 224
	#define PAL_HEIGHT 240

	/* screen total height */
	#define NTSC_TOTAL_HEIGHT 261
	#define PAL_TOTAL_HEIGHT 313

	/* vblank int */
	unsigned short NTSC_VBLANK_INT = NTSC_SPEED / 60;
	unsigned short PAL_VBLANK_INT = PAL_SPEED / 50;

	/* scanline refresh (hblank)*/
	unsigned short NTSC_SCANLINE_REFRESH = NTSC_VBLANK_INT / NTSC_TOTAL_HEIGHT;
	unsigned short PAL_SCANLINE_REFRESH = PAL_VBLANK_INT / PAL_TOTAL_HEIGHT;

	/* vblank int cycle timeout */
	unsigned int NTSC_VBLANK_CYCLE_TIMEOUT = (NTSC_TOTAL_HEIGHT-NTSC_HEIGHT) * NTSC_VBLANK_INT / NTSC_TOTAL_HEIGHT;
	unsigned int PAL_VBLANK_CYCLE_TIMEOUT = (PAL_TOTAL_HEIGHT-PAL_HEIGHT) * PAL_VBLANK_INT / PAL_TOTAL_HEIGHT;

	show_header();

	/* 64k main memory */
	memory = (unsigned char *)malloc(65536);

	/* 16k video memory */
	ppu_memory = (unsigned char *)malloc(16384);

	/* 256b sprite memory */
	sprite_memory = (unsigned char *)malloc(256);

	if(analyze_header("rom.nes") == 1)
	{
		free(sprite_memory);
		free(ppu_memory);
		free(memory);
		exit(1);
	}

	/* rom cache memory */
	romcache = (unsigned char *)malloc(romlen);

	/*printf("[*] PRG = %x, CHR = %x, OS_MIRROR = %d, FS_MIRROR = %d, TRAINER = %d"
		", SRAM = %d, MIRRORING = %d\n",
		PRG,CHR,OS_MIRROR, FS_MIRROR,TRAINER,SRAM,MIRRORING);

	printf("[*] mapper: %d found!\n",MAPPER);*/

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

/*
	statefile = str_replace(romfn,".nes",".sst");
	savfile = str_replace(romfn,".nes",".sav");

	if(SRAM == 1) {
		open_sav();
	}
*/

	if(systemType == SYSTEM_PAL) {
		height = PAL_HEIGHT;
		width = 256;
	} else if(systemType == SYSTEM_NTSC) {
		height = NTSC_HEIGHT;
		width = 256;
	} else return -1;


	initNESscreenCEL();
	initNESpal3DO();


	sdl_screen_height = height * scale;
	sdl_screen_width = width * scale;

	if(systemType == SYSTEM_PAL) {
		init_SDL(0,fullscreen);
	} else if(systemType == SYSTEM_NTSC) {
		init_SDL(1,fullscreen);
	}

	/*
	 * first reset the cpu at poweron
	 */
	CPU_reset();

	/*
	 * reset joystick
	 */
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

		check_SDL_event();
	}

	/* never reached */
	return(0);
}

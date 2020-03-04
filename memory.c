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

#include "memory.h"
#include "ppu.h"
#include "input.h"
#include "lame6502.h"
#include "macros.h"

unsigned char *memory;
unsigned char *ppu_memory;
unsigned char *sprite_memory;

char *savfile;
char *statefile;

int pad1_readcount = 0;

// FILE *sav_fp;

/*
void open_sav()
{
	sav_fp = fopen(savfile,"rb");

	if(sav_fp) {
		printf("[*] %s found! loading into sram...\n",savfile);
		fseek(sav_fp,0,SEEK_SET);
		fread(&memory[0x6000],1,8192,sav_fp);

		fclose(sav_fp);
	}
}

void write_sav()
{
	sav_fp = fopen(savfile,"wb");
	fwrite(&memory[0x6000],1,8192,sav_fp);
	fclose(sav_fp);
}

void load_state()
{
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
	}
}

void save_state()
{
	int pf;

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

	fprintf(stdout,"[*] done!\n");
}*/


// memory read handler
unsigned char memory_read(unsigned int address) {
	// this is ram or rom so we can return the address
	if(address < 0x2000 || address > 0x7FFF)
		return memory[address];

	// the addresses between 0x2000 and 0x5000 are for input/ouput
	if(address == 0x2002) {
		ppu_status_tmp = ppu_status;

		// set ppu_status (D7) to 0 (vblank_on)
		ppu_status &= 0x7F;
		write_memory(0x2002,ppu_status);

		// set ppu_status (D6) to 0 (sprite_zero)
		ppu_status &= 0x1F;
		write_memory(0x2002,ppu_status);

		// reset VRAM Address Register #1
		ppu_bgscr_f = 0x00;

		// reset VRAM Address Register #2
		ppu_addr_h = 0x00;

		// return bits 7-4 of unmodifyed ppu_status with bits 3-0 of the ppu_addr_tmp
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

	// pAPU data (sound)
	/*
	if(address == 0x4015) {
		return memory[address];
	}
	*/
	
	// joypad1 data
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
	}

	/*if(address == 0x4017) {
		return memory[address];
	}*/

	return memory[address];
}

// memory write handler
void write_memory(unsigned int address,unsigned char data)
{

	// PPU Status
	if(address == 0x2002) {
		memory[address] = data;
		return;
	}

	// PPU Video Memory area
	if(address > 0x1fff && address < 0x4000) {
		write_ppu_memory(address,data);
		return;
	}

	// Sprite DMA Register
	if(address == 0x4014) {
		write_ppu_memory(address,data);
		return;
	}

	// Joypad 1
	if(address == 0x4016) {
		memory[address] = 0x40;

		return;
	}

	// Joypad 2
	/*if(address == 0x4017) {
		memory[address] = 0x48;
		return;
	}*/

	// pAPU Sound Registers
	/*
	if(address > 0x3fff && address < 0x4016) {
		write_sound(address,data); not emulated yet!
		memory[address] = data;
		return;
	}
	*/

	// SRAM Registers
	/*
	if(address > 0x5fff && address < 0x8000) {
		if(SRAM == 1)
			write_sav();

		memory[address] = data;
		return;
	}
	*/

	// RAM registers
	if(address < 0x2000) {	// Should be 0x800 instead of 0x2000? Did it mean 2000 in DEC? (or specifically 2048)
							// (Why this has to mirror four times? Is it necessary? Need to read more about NES hardware)
		memory[address] = data;
		memory[address+2048] = data; // mirror of 0-0x800
		memory[address+4096] = data; // mirror of 0-0x800
		memory[address+6144] = data; // mirror of 0-0x800
		return;
	}

	// Disable the mappers for now to optimize thing for the single ROM I am trying
	// I will handle this whole memory_write/read calls in a more optimized way in the future anyway
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

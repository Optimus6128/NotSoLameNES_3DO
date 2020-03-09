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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lame6502/lame6502.h"
#include "lame6502/disas.h"

#include "lamenes.h"
#include "ppu.h"
#include "macros.h"
#include "romloader.h"
#include "palette.h"
#include "memory.h"

// gfx cache -> [hor][ver]
unsigned char bgcache[256+8][240];
unsigned char sprcache[256+8][240];
unsigned char shouldCheckSprCache[240];

uint32 tilemix[4][256][256];

// ppu control registers
unsigned int ppu_control1 = 0x00;
unsigned int ppu_control2 = 0x00;

// used to flip between first and second write (0x2006)
unsigned int ppu_addr_h = 0x00;

unsigned int ppu_addr = 0x2000;
unsigned int ppu_addr_tmp = 0x2000;

// used for scrolling techniques
unsigned int loopyT = 0x00;
unsigned int loopyV = 0x00;
unsigned int loopyX = 0x00;

// ppu status/temp registers
unsigned int ppu_status;
unsigned int ppu_status_tmp = 0x00;

unsigned int sprite_address = 0x00;

// used to flip between first and second write (0x2005)
unsigned int ppu_bgscr_f = 0x00;

// used to export the current scanline for the debugger
int current_scanline;

void init_ppu()
{
	int i,j,a,b;
	for (a=0; a<4; ++a) {
		for (i=0; i<256; ++i) {
			for (j=0; j<256; ++j) {
				uint32 *tilemixPtr = &tilemix[a][i][j];
				for (b=0; b<8; ++b) {
					int c = (((i >> (7-b)) & 1) << 1) | ((j >> (7-b)) & 1);
					if (c!=0) c |= (a << 2);
					*tilemixPtr = *tilemixPtr | (c << (28 - (b << 2)));
				}
				tilemixPtr++;
			}
		}
	}
}

int mw_ppu_0x2000 = 0;
int mw_ppu_0x2001 = 0;
int mw_ppu_0x2003 = 0;
int mw_ppu_0x2004 = 0;
int mw_ppu_0x2005 = 0;
int mw_ppu_0x2006 = 0;
int mw_ppu_0x2007 = 0;
int mw_ppu_0x4014 = 0;

void write_ppu_memory(unsigned int address,unsigned char data)
{	
	int i;

	if(address == 0x2000) {
		ppu_addr_tmp = data;

		ppu_control1 = data;

		memory[address] = data;

		loopyT &= 0xf3ff; // ~(0000110000000000)
		loopyT |= (data & 3) << 10; // (00000011)
        if (DEBUG_MEM_FREQS) mw_ppu_0x2000++;
		return;
    }

	if(address == 0x2001) {
		ppu_addr_tmp = data;

		ppu_control2 = data;
		memory[address] = data;
        if (DEBUG_MEM_FREQS) mw_ppu_0x2001++;
		return;
    }

	// sprite_memory address register
	if(address == 0x2003) {
		ppu_addr_tmp = data;

		sprite_address = data;
		memory[address] = data;
        if (DEBUG_MEM_FREQS) mw_ppu_0x2003++;
		return;
	}

	// sprite_memory i/o register
	if(address == 0x2004) {
		ppu_addr_tmp = data;

		sprite_memory[sprite_address] = data;
		sprite_address++;
        if (DEBUG_MEM_FREQS) mw_ppu_0x2004++;
		memory[address] = data;
		return;
	}

	// vram address register #1 (scrolling)
	if(address == 0x2005) {
		ppu_addr_tmp = data;
        if (DEBUG_MEM_FREQS) mw_ppu_0x2005++;
		if(ppu_bgscr_f == 0x00) {
			loopyT &= 0xFFE0; // (0000000000011111)
			loopyT |= (data & 0xF8) >> 3; // (11111000)
			loopyX = data & 0x07; // (00000111)

			ppu_bgscr_f = 0x01;

			memory[address] = data;

			return;
		}

		if(ppu_bgscr_f == 0x01) {
			loopyT &= 0xFC1F; // (0000001111100000)
			loopyT |= (data & 0xF8) << 2; //(0111000000000000)
			loopyT &= 0x8FFF; //(11111000)
			loopyT |= (data & 0x07) << 12; // (00000111)

			ppu_bgscr_f = 0x00;

			memory[address] = data;

			return;
		}
	}

	// vram address register #2
	if(address == 0x2006) {
		ppu_addr_tmp = data;
        if (DEBUG_MEM_FREQS) mw_ppu_0x2006++;
		// First write -> Store the high byte 6 bits and clear out the last two
		if(ppu_addr_h == 0x00) {
			ppu_addr = (data << 8);

			loopyT &= 0x00FF; // (0011111100000000)
			loopyT |= (data & 0x3F) << 8; // (1100000000000000) (00111111)

			ppu_addr_h = 0x01;

			memory[address] = data;

			return;
		}

		// Second write -> Store the low byte 8 bits
		if(ppu_addr_h == 0x01) {
			ppu_addr |= data;

			loopyT &= 0xFF00; // (0000000011111111)
			loopyT |= data; // (11111111)
			loopyV = loopyT; // v=t

			ppu_addr_h = 0x00;

			memory[address] = data;

			return;
		}
	}

	// vram i/o register
	if(address == 0x2007) {
		// if the vram_write_flag is on, vram writes should ignored 
        if (DEBUG_MEM_FREQS) mw_ppu_0x2007++;
		if(vram_write_flag)
			return;

		ppu_addr_tmp = data;

		ppu_memory[ppu_addr] = data;

		// nametable mirroring
		if((ppu_addr > 0x1999) && (ppu_addr < 0x3000)) {
			if(OS_MIRROR == 1) {
				ppu_memory[ppu_addr + 0x400] = data;
				ppu_memory[ppu_addr + 0x800] = data;
				ppu_memory[ppu_addr + 0x1200] = data;
			} /*else if(FS_MIRROR == 1) {
				printf("FS_MIRRORING detected! do nothing\n");
			} */else {
				if(MIRRORING == 0) {
					// horizontal
					ppu_memory[ppu_addr + 0x400] = data;
				} else {
					// vertical
					ppu_memory[ppu_addr + 0x800] = data;
				}
			}
		}

		// palette mirror
		if(ppu_addr == 0x3f10) {
			ppu_memory[0x3f00] = data;
		}

		ppu_addr_tmp = ppu_addr;

		if(!increment_32) {
			ppu_addr++;
		} else {
			ppu_addr += 0x20;
		}
		memory[address] = data;
		return;
	}

	// transfer 256 bytes of memory into sprite_memory
    if(address == 0x4014) {
		for(i = 0; i < 256; i++) {
			sprite_memory[i] = memory[0x100 * data + i];
		}
        if (DEBUG_MEM_FREQS) mw_ppu_0x4014++;
	}

	return;
}

void render_background(int scanline)
{
	int tile_count;

	int nt_addr;
	int at_addr;

	int x_scroll;
	int y_scroll;

	int pt_addr;

	//int attribs;

	uint16 *dst;
	//uint32 *tilemixAttribOffset;

	if (!background_on || (systemType == SYSTEM_NTSC && scanline < 8)) return;
	
	dst = (uint16*)screenCel->ccb_SourcePtr + scanline * screenCel->ccb_Width;

	current_scanline = scanline;

	// loopy scanline start -> v:0000010000011111=t:0000010000011111 | v=t
	loopyV &= 0xfbe0;
	loopyV |= (loopyT & 0x041f);

	x_scroll = (loopyV & 0x1f);
	y_scroll = (loopyV & 0x03e0) >> 5;

	nt_addr = 0x2000 + (loopyV & 0x0fff);
	at_addr = 0x2000 + (loopyV & 0x0c00) + 0x03c0 + ((y_scroll & 0xfffc) << 1) + (x_scroll >> 2);
	
	// draw 33 tiles in a scanline (32 + 1 for scrolling)
	for(tile_count = 0; tile_count < 33; tile_count++) {
		const int attribs = (ppu_memory[at_addr] >> (((y_scroll & 2) << 1) + (x_scroll & 2))) & 3;
		const uint32 *tilemixAttribOffset = (uint32*)&tilemix[attribs];

		// nt_data (ppu_memory[nt_addr]) * 16 = pattern table address
		pt_addr = (ppu_memory[nt_addr] << 4) + ((loopyV & 0x7000) >> 12);

		// check if the pattern address needs to be high
		if(background_addr_hi)
			pt_addr+=0x1000;

		
		{
			const int p1 = ppu_memory[pt_addr];
			const int p2 = ppu_memory[pt_addr + 8];
			const unsigned char *ppuSrc = &ppu_memory[0x3f00];
			const uint32 tilemixNibbles = *(tilemixAttribOffset + (p2 << 8) + p1);

			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 28) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 24) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 20) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 16) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 12) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 8) & 15]];
			*dst++ = palette3DO[ppuSrc[(tilemixNibbles >> 4) & 15]];
			*dst++ = palette3DO[ppuSrc[tilemixNibbles & 15]];
		}

		nt_addr++;
		x_scroll++;

		// boundary check
		// dual-tile
		if((x_scroll & 0x0001) == 0) {
			// quad-tile
			if((x_scroll & 0x0003) == 0) {
				// check if we crossed a nametable
				if((x_scroll & 0x1F) == 0) {
					// switch name/attrib tables
					nt_addr ^= 0x0400;
					at_addr ^= 0x0400;
					nt_addr -= 0x0020;
					at_addr -= 0x0008;
					x_scroll -= 0x0020;
				}

				at_addr++;
			}
		}
	}

	// subtile y_offset == 7
	if((loopyV & 0x7000) == 0x7000) {
		// subtile y_offset = 0
		loopyV &= 0x8fff;

		// nametable line == 29
		if((loopyV & 0x03e0) == 0x03a0) {
			// switch nametables (bit 11)
			loopyV ^= 0x0800;

			// name table line = 0
			loopyV &= 0xfc1f;
		} else {
			// nametable line == 31
			if((loopyV & 0x03e0) == 0x03e0) {
				// name table line = 0
				loopyV &= 0xfc1f;
			} else {
          			loopyV += 0x0020;
			}
		}
	} else {
		// next subtile y_offset
		loopyV += 0x1000;
	}
}


void render_sprite(int x, int y, int pattern_number, int attribs, int spr_nr)
{
	int disp_spr_back;
	int flip_spr_hor;
	int flip_spr_ver;

	int i;
	int j;

	int spr_start;
	int sprite_pattern_table;
	
	unsigned char sprite[8*16];

	unsigned char *spritePtr;
	uint16 *dst;
	const int attribsAdd = (attribs & 0x03) << 0x02;
	
	if (!sprite_on) return;

	disp_spr_back = attribs & 0x20;
	flip_spr_hor = attribs & 0x40;
	flip_spr_ver = attribs & 0x80;

	if(!sprite_addr_hi) {
		sprite_pattern_table=0x0000;
	} else {
		sprite_pattern_table=0x1000;
	}

	// - pattern_number * 16
	spr_start = sprite_pattern_table + ((pattern_number << 3) << 1);

	if (spr_nr==0) {
		int i;
		int sprHeight = 8;
		if (sprite_16) sprHeight = 16;
		for (i=0; i<sprHeight; ++i) {
			const uint32 yi = y + i;
			if (yi < NES_screen_height)
				shouldCheckSprCache[yi] = 1;
		}
	}


	dst = (uint16*)screenCel->ccb_SourcePtr + y * screenCel->ccb_Width + x;

	if(!sprite_16) {
		int spriteVal;

		// 8 x 8 sprites
		// fetch bits
		spritePtr = (unsigned char*)sprite;
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(j = 0; j < 8; j++) {
				const unsigned char p1 = ppu_memory[spr_start + j];
				const unsigned char p2 = ppu_memory[spr_start + 8 + j];
				spriteVal = ((p2 >> 6) & 2) | ((p1 >> 7) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 5) & 2) | ((p1 >> 6) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 4) & 2) | ((p1 >> 5) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 3) & 2) | ((p1 >> 4) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 2) & 2) | ((p1 >> 3) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 1) & 2) | ((p1 >> 2) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 0) & 2) | ((p1 >> 1) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 << 1) & 2) | ((p1 >> 0) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(j = 0; j < 8; j++) {
				const unsigned char p1 = ppu_memory[spr_start + j];
				const unsigned char p2 = ppu_memory[spr_start + 8 + j];
				spriteVal = ((p2 << 1) & 2) | ((p1 >> 0) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 0) & 2) | ((p1 >> 1) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 1) & 2) | ((p1 >> 2) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 2) & 2) | ((p1 >> 3) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 3) & 2) | ((p1 >> 4) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 4) & 2) | ((p1 >> 5) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 5) & 2) | ((p1 >> 6) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 6) & 2) | ((p1 >> 7) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(j = 7; j >= 0; j--) {
				const unsigned char p1 = ppu_memory[spr_start + j];
				const unsigned char p2 = ppu_memory[spr_start + 8 + j];
				spriteVal = ((p2 >> 6) & 2) | ((p1 >> 7) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 5) & 2) | ((p1 >> 6) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 4) & 2) | ((p1 >> 5) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 3) & 2) | ((p1 >> 4) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 2) & 2) | ((p1 >> 3) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 1) & 2) | ((p1 >> 2) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 0) & 2) | ((p1 >> 1) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 << 1) & 2) | ((p1 >> 0) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(j = 7; j >= 0; j--) {
				const unsigned char p1 = ppu_memory[spr_start + j];
				const unsigned char p2 = ppu_memory[spr_start + 8 + j];
				spriteVal = ((p2 << 1) & 2) | ((p1 >> 0) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 0) & 2) | ((p1 >> 1) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 1) & 2) | ((p1 >> 2) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 2) & 2) | ((p1 >> 3) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 3) & 2) | ((p1 >> 4) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 4) & 2) | ((p1 >> 5) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 5) & 2) | ((p1 >> 6) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
				spriteVal = ((p2 >> 6) & 2) | ((p1 >> 7) & 1); if (spriteVal!=0) spriteVal += attribsAdd; *spritePtr++ = spriteVal;
			}
		}

		spritePtr = (unsigned char*)sprite;
		for(j = 0; j < 8; j++) {
			unsigned char *sprcachePtr = (unsigned char*)&sprcache[y+j][x];
			unsigned short *bgcachePtr = dst + j * screenCel->ccb_Width;
			for(i = 0; i < 8; i++) {
				// cache pixel for sprite zero detection
				const unsigned char value = *spritePtr++;
				if(spr_nr == 0)
					*sprcachePtr++ = value;

				if(value != 0) {
					// sprite priority check
					if(!disp_spr_back) {
						*(dst + i) = palette3DO[ppu_memory[0x3f10 + value]];
					} else {
						// draw the sprite pixel if the background pixel is transparent (0)
						if(*bgcachePtr++ == 0) {
							*(dst + i) = palette3DO[ppu_memory[0x3f10 + value]];
						}
					}
				}
			}
			dst += screenCel->ccb_Width;
		}
	} else {
		// 8 x 16 sprites
		// fetch bits
		spritePtr = (unsigned char*)sprite;
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(j = 0; j < 16; j++) {
				for(i = 7; i >= 0; i--) {
					int spriteVal = (((ppu_memory[spr_start + 8 + j] >> i) & 1) << 1) | ((ppu_memory[spr_start + j] >> i) & 1);
					if (spriteVal!=0) spriteVal += attribsAdd;
					*spritePtr++ = spriteVal;
				}
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(j = 0; j < 16; j++) {
				for(i = 0; i < 8; i++) {
					int spriteVal = (((ppu_memory[spr_start + 8 + j] >> i) & 1) << 1) | ((ppu_memory[spr_start + j] >> i) & 1);
					if (spriteVal!=0) spriteVal += attribsAdd;
					*spritePtr++ = spriteVal;
				}
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(j = 15; j >= 0; j--) {
				for(i = 7; i >= 0; i--) {
					int spriteVal = (((ppu_memory[spr_start + 8 + j] >> i) & 1) << 1) | ((ppu_memory[spr_start + j] >> i) & 1);
					if (spriteVal!=0) spriteVal += attribsAdd;
					*spritePtr++ = spriteVal;
				}
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(j = 15; j >= 0; j--) {
				for(i = 0; i < 8; i++) {
					int spriteVal = (((ppu_memory[spr_start + 8 + j] >> i) & 1) << 1) | ((ppu_memory[spr_start + j] >> i) & 1);
					if (spriteVal!=0) spriteVal += attribsAdd;
					*spritePtr++ = spriteVal;
				}
			}
		}

		spritePtr = (unsigned char*)sprite;
		for(j = 0; j < 16; j++) {
			unsigned char *sprcachePtr = (unsigned char*)&sprcache[y+j][x];
			unsigned short *bgcachePtr = dst + j * screenCel->ccb_Width;
			for(i = 0; i < 8; i++) {
				// cache pixel for sprite zero detection
				const unsigned char value = *spritePtr++;
				if(spr_nr == 0)
					*sprcachePtr++ = value;

				if(value != 0) {
					// sprite priority check
					if(!disp_spr_back) {
						*(dst + i) = palette3DO[ppu_memory[0x3f10 + value]];
					} else {
						// draw the sprite pixel if the background pixel is transparent (0)
						if(*bgcachePtr++ == 0) {
							*(dst + i) = palette3DO[ppu_memory[0x3f10 + value]];
						}
					}
				}
			}
			dst += screenCel->ccb_Width;
		}
	}
}

void check_sprite_hit(int scanline)
{
	// sprite zero detection
	int i;
	//uint8 *bgcachePtr = (uint8*)&bgcache[scanline-1][0];
	uint16 *bgcachePtr = (uint16*)screenCel->ccb_SourcePtr + (scanline-1) * screenCel->ccb_Width;
	uint8 *sprcachePtr = (uint8*)&sprcache[scanline-1][0];
	
	if (!shouldCheckSprCache[scanline]) return;

	for(i = 0; i < NES_screen_width; i++) {
		if((bgcachePtr[i] | sprcachePtr[i])!=0) {
			// set the sprite zero flag
			ppu_status |= 0x40;
			return;
		}
	}
}

void render_sprites()
{
	int i = 0;

	// clear sprite cache
	memset(sprcache,0,sizeof(sprcache));
	memset(shouldCheckSprCache, 0, sizeof(shouldCheckSprCache));

	// fetch all 64 sprites out of the sprite memory 4 bytes at a time and render the sprite
	// sprites are drawn in priority from sprite 64 to 0
	for(i = 63; i >= 0; i--) 
	{
		render_sprite(sprite_memory[i * 4 + 3], sprite_memory[i * 4], sprite_memory[i * 4 + 1], sprite_memory[i * 4 + 2], i);
	}
}

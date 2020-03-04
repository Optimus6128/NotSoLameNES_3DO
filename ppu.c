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

#include "3DO/GestionAffichage.h"
#include "3DO/GestionSprites.h"
#include "3DO/GestionTextes.h"


#include "lamenes.h"
#include "ppu.h"
#include "macros.h"
#include "romloader.h"
#include "palette.h"
#include "memory.h"

// gfx cache -> [hor] [ver]
unsigned char bgcache[256+8] [256+8];
unsigned char sprcache[256+8] [256+8];
unsigned char shouldCheckSprCache[256+8];

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

void write_ppu_memory(unsigned int address,unsigned char data)
{	
	int i;

	if(address == 0x2000) {
		ppu_addr_tmp = data;

		ppu_control1 = data;

		memory[address] = data;

		loopyT &= 0xf3ff; // (0000110000000000)
		loopyT |= (data & 3) << 10; // (00000011)

		return;
    }

	if(address == 0x2001) {
		ppu_addr_tmp = data;

		ppu_control2 = data;
		memory[address] = data;

		return;
    }

	// sprite_memory address register
	if(address == 0x2003) {
		ppu_addr_tmp = data;

		sprite_address = data;
		memory[address] = data;

		return;
	}

	// sprite_memory i/o register
	if(address == 0x2004) {
		ppu_addr_tmp = data;

		sprite_memory[sprite_address] = data;
		sprite_address++;

		memory[address] = data;
		return;
	}

	// vram address register #1 (scrolling)
	if(address == 0x2005) {
		ppu_addr_tmp = data;

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
	}

	return;
}

void draw_pixel(int x, int y, int nescolor)
{
	if (nescolor != 0) {
		uint16 *dst = (uint16*)screenCel->ccb_SourcePtr + y * screenCel->ccb_Width + x;
		*dst = palette3DO[nescolor];
	}
}

void render_background(int scanline)
{
	int tile_count;

	int i;

	int nt_addr;
	int at_addr;

	int x_scroll;
	int y_scroll;

	int pt_addr;

	int attribs;

	
	uint16 *dst;
	uint8 *bgcachePtr;

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

	if((y_scroll & 0x0002) == 0) {
		if((x_scroll & 0x0002) == 0) {
			attribs = (ppu_memory[at_addr] & 0x03) << 2;
		} else {
			attribs = (ppu_memory[at_addr] & 0x0C);
		}
	} else {
		if((x_scroll & 0x0002) == 0) {
			attribs = (ppu_memory[at_addr] & 0x30) >> 2;
		} else {
			attribs = (ppu_memory[at_addr] & 0xC0) >> 4;
		}
	}

	// draw 33 tiles in a scanline (32 + 1 for scrolling)
	bgcachePtr = (uint8*)&bgcache[scanline][0];
	for(tile_count = 0; tile_count < 33; tile_count++) {

		// nt_data (ppu_memory[nt_addr]) * 16 = pattern table address
		pt_addr = (ppu_memory[nt_addr] << 4) + ((loopyV & 0x7000) >> 12);

		// check if the pattern address needs to be high
		if(background_addr_hi)
			pt_addr+=0x1000;

		for(i = 0; i < 8; i++) {
			const int bit1 = (ppu_memory[pt_addr] >> (7 - i)) & 1;
			const int bit2 = (ppu_memory[pt_addr + 8] >> (7 - i)) & 1;
			int tile = (bit2 << 1) | bit1;

			if(tile != 0) {
				tile += attribs;
			}

			*dst++ = palette3DO[ppu_memory[0x3f00 + tile]];
			*bgcachePtr++ = tile;
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

			if((y_scroll & 0x0002) == 0) {
				if((x_scroll & 0x0002) == 0) {
					attribs = ((ppu_memory[at_addr]) & 0x03) << 2;
				} else {
					attribs = ((ppu_memory[at_addr]) & 0x0C);
				}
			} else {
				if((x_scroll & 0x0002) == 0) {
					attribs = ((ppu_memory[at_addr]) & 0x30) >> 2;
				} else {
					attribs = ((ppu_memory[at_addr]) & 0xC0) >> 4;
				}
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


void render_sprite(int y, int x, int pattern_number, int attribs, int spr_nr)
{
	int disp_spr_back;
	int flip_spr_hor;
	int flip_spr_ver;

	int i;
	int j;

	int spr_start;
	int sprite_pattern_table;
	
	unsigned char bit1[8][16];
	unsigned char bit2[8][16];
	unsigned char sprite[8][16];

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
			shouldCheckSprCache[y+i] = 1;
		}
	}

	if(!sprite_16) {
		// 8 x 8 sprites
		// fetch bits
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 0; j < 8; j++) {
					bit1[7 - i] [j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[7 - i] [j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 0; j < 8; j++) {
					bit1[i] [j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[i] [j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 7; j >= 0; j--) {
					bit1[7 - i] [7 - j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[7 - i] [7 - j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 7; j >= 0; j--) {
					bit1[i] [7 - j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[i] [7 - j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		}

		// merge bits
		for(i = 0; i < 8; i++) {
			for(j = 0; j < 8; j++) {
				sprite[i][j] = (bit2[i][j] << 1) | bit1[i][j];
			}
		}	

		// add sprite attribute colors
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 0; j < 8; j++) {
					if(sprite[7 - i] [j] != 0) {
						if(sprite[7 - i] [j] != 0)
							sprite[7 - i] [j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 0; j < 8; j++) {
					if(sprite[i] [j] != 0) {
						if(sprite[i] [j] != 0)
							sprite[i] [j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 7; j >= 0; j--) {
					if(sprite[7 - i] [7 - j] != 0) {
						if(sprite[7 - i] [7 - j] != 0)
							sprite[7 - i] [7 - j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 7; j >= 0; j--) {
					if(sprite[i] [7 - j] != 0) {
						if(sprite[i] [7 - j] != 0)
							sprite[i] [7 - j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		}

		for(i = 0; i < 8; i++) {
			for(j = 0; j < 8; j++) {
				// cache pixel for sprite zero detection
				if(spr_nr == 0)
					sprcache[y + j][x + i] = sprite[i] [j];

				if(sprite[i] [j] != 0) {
					// sprite priority check
					if(!disp_spr_back) {
						if(sprite_on) {
							draw_pixel(x + i, y + j, ppu_memory[0x3f10 + (sprite[i] [j])]);
						}
					} else {
						if(sprite_on) {
							// draw the sprite pixel if the background pixel is transparent (0)
							if(bgcache[y+j] [x+i] == 0) {
								draw_pixel(x + i, y + j, ppu_memory[0x3f10 + (sprite[i] [j])]);
							}
						}
					}
				}
			}
		}
	} else {
		// 8 x 16 sprites
		// fetch bits
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 0; j < 16; j++) {
					bit1[7 - i] [j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[7 - i] [j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 0; j < 16; j++) {
					bit1[i] [j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[i] [j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 15; j >= 0; j--) {
					bit1[7 - i] [15 - j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[7 - i] [15 - j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 15; j >= 0; j--) {
					bit1[i] [15 - j] = (ppu_memory[spr_start + j] >> i) & 1;
					bit2[i] [15 - j] = (ppu_memory[spr_start + 8 + j] >> i) & 1;
				}
			}
		}

		// merge bits
		for(i = 0; i < 8; i++) {
			for(j = 0; j < 16; j++) {
				sprite[i][j] = (bit2[i][j] << 1) | bit1[i][j];
			}
		}	

		// add sprite attribute colors
		if((!flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 0; j < 16; j++) {
					if(sprite[7 - i] [j] != 0) {
						if(sprite[7 - i] [j] != 0)
							sprite[7 - i] [j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((flip_spr_hor) && (!flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 0; j < 16; j++) {
					if(sprite[i] [j] != 0) {
						if(sprite[i] [j] != 0)
							sprite[i] [j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((!flip_spr_hor) && (flip_spr_ver)) {
			for(i = 7; i >= 0; i--) {
				for(j = 15; j >= 0; j--) {
					if(sprite[7 - i] [15 - j] != 0) {
						if(sprite[7 - i] [15 - j] != 0)
							sprite[7 - i] [15 - j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		} else if((flip_spr_hor) && (flip_spr_ver)) {
			for(i = 0; i < 8; i++) {
				for(j = 15; j >= 0; j--) {
					if(sprite[i] [15 - j] != 0) {
						if(sprite[i] [15 - j] != 0)
							sprite[i] [15 - j] += ((attribs & 0x03) << 0x02);
					}
				}
			}
		}

		for(i = 0; i < 8; i++) {
			for(j = 0; j < 16; j++) {
				// cache pixel for sprite zero detection
				if(spr_nr == 0)
					sprcache[y + j][x + i] = sprite[i] [j];

				if(sprite[i] [j] != 0) {
					// sprite priority check
					if(!disp_spr_back) {
						if(sprite_on) {
							draw_pixel(x + i, y + j, ppu_memory[0x3f10 + (sprite[i] [j])]);
						}
					} else {
						// draw the sprite pixel if the background pixel is transparent (0)
						if(bgcache[y+j] [x+i] == 0) {
							if(sprite_on) {
								draw_pixel(x + i, y + j, ppu_memory[0x3f10 + (sprite[i] [j])]);
							}
						}
					}
				}
			}
		}
	}
}

void check_sprite_hit(int scanline)
{
	// sprite zero detection
	int i;
	uint8 *bgcachePtr = (uint8*)&bgcache[scanline-1][0];
	uint8 *sprcachePtr = (uint8*)&sprcache[scanline-1][0];
	
	if (!shouldCheckSprCache[scanline]) return;

	for(i = 0; i < width; i++) {
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
		render_sprite(sprite_memory[i * 4],sprite_memory[i * 4 + 3],sprite_memory[i * 4 + 1],sprite_memory[i * 4 + 2],i);
	}
}

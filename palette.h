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

#ifndef LAMENES_PALETTE_H
#define LAMENES_PALETTE_H

#define NUM_PAL_COLORS 64

#define MAKE_NES_TO_3DO_PAL(r,g,b) (((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3))

typedef struct Palette {
	int r;
	int g;
	int b;
} Palette;

static Palette palette[NUM_PAL_COLORS] = {
	{0x7C,0x7C,0x7C},{0x00,0x00,0xFC},{0x00,0x00,0xBC},{0x44,0x28,0xBC},{0x94,0x00,0x84},{0xA8,0x00,0x20},{0xA8,0x10,0x00},{0x88,0x14,0x00},
	{0x50,0x30,0x00},{0x00,0x78,0x00},{0x00,0x68,0x00},{0x00,0x58,0x00},{0x00,0x40,0x58},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
	{0xBC,0xBC,0xBC},{0x00,0x78,0xF8},{0x00,0x58,0xF8},{0x68,0x44,0xFC},{0xD8,0x00,0xCC},{0xE4,0x00,0x58},{0xF8,0x38,0x00},{0xE4,0x5C,0x10},
	{0xAC,0x7C,0x00},{0x00,0xB8,0x00},{0x00,0xA8,0x00},{0x00,0xA8,0x44},{0x00,0x88,0x88},{0x00,0x00,0x00},{0x00,0x00,0x00},{0x00,0x00,0x00},
	{0xF8,0xF8,0xF8},{0x3C,0xBC,0xFC},{0x68,0x88,0xFC},{0x98,0x78,0xF8},{0xF8,0x78,0xF8},{0xF8,0x58,0x98},{0xF8,0x78,0x58},{0xFC,0xA0,0x44},
	{0xF8,0xB8,0x00},{0xB8,0xF8,0x18},{0x58,0xD8,0x54},{0x58,0xF8,0x98},{0x00,0xE8,0xD8},{0x78,0x78,0x78},{0x00,0x00,0x00},{0x00,0x00,0x00},
	{0xFC,0xFC,0xFC},{0xA4,0xE4,0xFC},{0xB8,0xB8,0xF8},{0xD8,0xB8,0xF8},{0xF8,0xB8,0xF8},{0xF8,0xA4,0xC0},{0xF0,0xD0,0xB0},{0xFC,0xE0,0xA8},
	{0xF8,0xD8,0x78},{0xD8,0xF8,0x78},{0xB8,0xF8,0xB8},{0xB8,0xF8,0xD8},{0x00,0xFC,0xFC},{0xF8,0xD8,0xF8},{0x00,0x00,0x00},{0x00,0x00,0x00}
};

extern uint16 palette3DO[NUM_PAL_COLORS];

#endif

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


#include "lamenes.h"
#include "nes_input.h"

#include "3DO/input.h"


unsigned char pad1[PAD_BUTTONS_NUM];

// Corresponds to 3DO/input.h JOY_BUTTONS enums
static int joyButtonsMap[JOY_BUTTONS_NUM] = { 
	PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT,
	PAD_B, PAD_A,
	PAD_PAUSE_EMU, PAD_LOAD_STATE, PAD_SAVE_STATE,
	PAD_SELECT, PAD_START
};


void updateNesInput()
{
	int i;
	for (i=0; i<JOY_BUTTONS_NUM; ++i) {
		if (isJoyButtonPressed(i)) {
			pad1[joyButtonsMap[i]] = 0x01;
		} else {
			pad1[joyButtonsMap[i]] = 0x40;
		}
	}
}

void resetNesInput()
{
	int i;
	for (i=0; i<PAD_BUTTONS_NUM; ++i) {
		pad1[i] = 0x40;
	}
}

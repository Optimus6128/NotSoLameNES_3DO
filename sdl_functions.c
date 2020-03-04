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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lame6502/lame6502.h"

#include "lamenes.h"
#include "input.h"

#ifdef PC
	#include <SDL/SDL.h>
	SDL_Surface *screen;
	SDL_Event event;
#else

	#include "3DO/GestionAffichage.h"
	#include "3DO/GestionSprites.h"
	#include "3DO/GestionTextes.h"
	#include "3DO/system.h"
#endif

void screen_lock()
{
	#ifdef PC
        if(SDL_MUSTLOCK(screen)) {
                if(SDL_LockSurface(screen) < 0) {
                        return;
                }
        }
    #endif
}

void screen_unlock()
{
	#ifdef PC
        if(SDL_MUSTLOCK(screen)) {
                SDL_UnlockSurface(screen);
        }
    #endif
}

void init_SDL(int type, int fullscreen)
{
	#ifdef PC
		SDL_Init(SDL_INIT_VIDEO);
		screen = SDL_SetVideoMode(sdl_screen_width, sdl_screen_height, 16, SDL_SWSURFACE);
		SDL_FillRect(screen, NULL, 0);
		SDL_Flip(screen);
	#else
	
		affichageInitialisation();
		
		InitializeControlPads();
		
		affichageRendu();
	
	#endif
	
	return;
}

void check_SDL_event()
{
#ifdef PC
	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_KEYDOWN) {
			switch(event.key.keysym.sym) {
				case SDLK_DOWN:
				set_input((char *) 1);
				break;

				case SDLK_UP:
				set_input((char *) 2);
				break;

				case SDLK_LEFT:
				set_input((char *) 3);
				break;

				case SDLK_RIGHT:
				set_input((char *) 4);
				break;

				case SDLK_LCTRL:
				set_input((char *) 5);
				break;

				case SDLK_LSHIFT:
				set_input((char *) 6);
				break;

				case SDLK_p:
				if(pause_emulation) 
				{
					printf("[*] LameNES continue emulation!\n");
					CPU_is_running = 1;
					pause_emulation = 0;
				} else if(!pause_emulation) 
				{
					printf("[*] LameNES paused!\n");
					CPU_is_running = 0;
					pause_emulation = 1;
				}
				break;

				case SDLK_x:
				set_input((char *) 7);
				break;

				case SDLK_z:
				set_input((char *) 8);
				break;

				case SDLK_q:
				quit_emulation();
				break;

				case SDLK_ESCAPE:
				CPU_is_running = 0;
				break;

				case SDLK_F1:
				/* reset */
				reset_emulation();
				break;

				case SDLK_F3:
				/* load state */
				load_state();
				break;

				case SDLK_F6:
				/* save state */
				save_state();
				break;

				case SDLK_F10:
				if(enable_background == 1) {
					enable_background = 0;
				} else {
					enable_background = 1;
				}
				break;

				case SDLK_F11:
				if(enable_sprites == 1) {
					enable_sprites = 0;
				} else {
					enable_sprites = 1;
				}
				break;

				default:
				break;
			}
		}

		if(event.type == SDL_KEYUP) {
			switch(event.key.keysym.sym){
				case SDLK_DOWN:
				clear_input((char *) 1);
				break;

				case SDLK_UP:
				clear_input((char *) 2);
				break;

				case SDLK_LEFT:
				clear_input((char *) 3);
				break;

				case SDLK_RIGHT:
				clear_input((char *) 4);
				break;

				case SDLK_LCTRL:
				clear_input((char *) 5);
				break;

				case SDLK_LSHIFT:
				clear_input((char *) 6);
				break;

				case SDLK_x:
				clear_input((char *) 7);
				break;

				case SDLK_z:
				clear_input((char *) 8);
				break;

				default:
				break;
			}
		}
	}
#else	
	uint32	gButtons;
	unsigned char i;
	
	DoControlPad(1, &gButtons, (ControlUp | ControlDown | ControlLeft | ControlRight));
	
	for (i=0;i<9;i++)
		clear_input((char *) i);
		
	if (gButtons & ControlDown)	
	{
		set_input((char *) 1);
	}
	
	if (gButtons & ControlUp)	
	{
		set_input((char *) 2);
	}
	
	if (gButtons & ControlLeft)	
	{
		set_input((char *) 3);
	}
	
	if (gButtons & ControlRight)	
	{
		set_input((char *) 4);
	}
	
	if (gButtons & ControlStart)	
	{
		set_input((char *) 5);
	}
	
	/*	Select */
	if (gButtons & ControlC)	
	{
		set_input((char *) 6);
	}

	if (gButtons & ControlA)	
	{
		set_input((char *) 7);
	}	
	
	if (gButtons & ControlB)	
	{
		set_input((char *) 8);
	}	
#endif
	
}

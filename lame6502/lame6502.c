/*
 * lame6502 - a portable 6502 emulator
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

/*
 * lame6502.c - 6502 cpu functions and registers
 */

//#define DEBUG
//#define DISASSAMBLE
//#define STACK_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "instructions.h"
#include "disas.h"
#include "memory.h"


/*
 * 6502 internal registers
 */

/* 16 bits wide */
unsigned int program_counter;

/* 8 bits wide */
unsigned char stack_pointer;
unsigned char status_register;
unsigned char x_reg;		/* index reg */
unsigned char y_reg;		/* index reg */
unsigned char accumulator;	/* core */

unsigned int addr;
unsigned int tmp, tmp2, tmp3;


/* status_register flags */
int zero_flag;		/* this is set if the last operation returned a result of zero */

int sign_flag;		/* 
		 	* (N flag) this is simply a reflection of the highest bit of
		 	* the result of the last operation. A number with it's high bit
		 	* set is considered to be negative (0xff = -1); they are called
		 	* "Signed" numbers
		 	*/

int overflow_flag;	/* this is set if the last operation resulted in a sign change. */

int break_flag;		/* this is set only if the BRK instruction is executed. */

int decimal_flag;	/* if set, all Addition/Subtraction operations will be calculated using
			 * "Binary-coded Decimal"-formatted values (eg., $69 = 69), and will 
			 * return a BCD value. Decimal mode is unavaliable on the NES' 6502, 
			 * which is just as well.
			 */

int interrupt_flag;	/* if set, IRQ interrupts will be disabled */

int carry_flag;		/* this holds the "carry" out of the most significant bit of the last
			 * addition/subtraction/shift/rotate instruction. If an addition
			 * produces a result greater than 255, this is set. If a subtraction
			 * produces a result less than zero, this is cleared (Subtract
			 * operations use the opposite of the Carry (1=0, 0=1)).
			 */

int cycle_count;


/*void update_status_register()
{
	status_register = ((sign_flag ? 0x80 : 0) | (zero_flag ? 0x02 : 0) | (carry_flag ? 0x01 : 0) |
			(interrupt_flag ? 0x04 : 0) | (decimal_flag ? 0x08 : 0) | (overflow_flag ? 0x40 : 0) |
			(break_flag ? 0x10 : 0) | 0x20);
}*/


int IRQ(int cycles)
{
	unsigned short result;
	result = GET_SR();
	PUSH_ST((program_counter & 0xff00) >> 8);
	PUSH_ST(program_counter & 0xff);
	PUSH_ST(result);
	break_flag = 0;
	interrupt_flag = 1;
	program_counter = (memory[0xffff] << 8) | memory[0xfffe];
	return cycles -= 7;
}

int NMI(int cycles)
{
	unsigned short result;
	result = GET_SR();
	
	PUSH_ST((program_counter & 0xff00) >> 8);
	PUSH_ST(program_counter & 0xff);
	PUSH_ST(result);
	break_flag = 0;
	interrupt_flag = 1;
	program_counter = (memory[0xfffb] << 8) | memory[0xfffa];

	return cycles -= 7;
}


void CPU_reset(void)
{
	status_register = 0x20;

	zero_flag = 1;
	sign_flag = 0;
	overflow_flag = 0;
	break_flag = 0;
	decimal_flag = 0;
	interrupt_flag = 0;
	carry_flag = 0;

	stack_pointer = 0xff;

	program_counter = (memory[0xfffd] << 8) | memory[0xfffc];

	accumulator=x_reg=y_reg=0;
}


int CPU_execute(int cycles) 
{
	unsigned char opcode;

	cycle_count = cycles;
	do 
	{
		//update_status_register();
		//status_register = carry_flag | (zero_flag << 1) | (interrupt_flag << 2) | (decimal_flag << 3) | (break_flag << 4) | (1<<5) | (overflow_flag << 6) | (sign_flag << 7);
		//We don't even need to create status_register every opcode run! It's only used on save_state/load_state which we don't support.
		//Flags are read/written from/to separate ints
		
		opcode=memory[program_counter++];

		switch(opcode) 
		{
			#include "opcodes.h"
		}

	} while(cycle_count > 0);

	return cycles - cycle_count;
}






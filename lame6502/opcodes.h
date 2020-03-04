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
 * opcodes.h - 6502 opcode handler
 */

/* ADC  -  Add to Accumulator with Carry */
case 0x69: ADC_IM(2);
case 0x65: ADC_ZP(3);
case 0x75: ADC_ZPIX(4);
case 0x6D: ADC_A(4);
case 0x7D: ADC_AIX(5);
case 0x79: ADC_AIY(4);
case 0x61: ADC_IDI(6);
case 0x71: ADC_INI(5);

/* AND  -  AND Memory with Accumulator */
case 0x29: AND_IM(2);
case 0x25: AND_ZP(3);
case 0x35: AND_ZPIX(4);
case 0x2D: AND_A(4);
case 0x3D: AND_AIX(5);
case 0x39: AND_AIY(4);
case 0x21: AND_IDI(6);
case 0x31: AND_INI(5);

/* ASL  -  Arithmatic Shift Left */
case 0x0A: ARITH_SL_ACC(2);
case 0x06: ARITH_SL_ZP(5);
case 0x16: ARITH_SL_ZPIX(6);
case 0x0E: ARITH_SL_A(6);
case 0x1E: ARITH_SL_AIX(7);

/* BCC  -  Branch on Carry Clear */
case 0x90: BRANCH_CC(2);

/* BCS  -  Branch on Carry Set */
case 0xB0: BRANCH_CS(2);

/* BEQ  -  Branch Zero Set */
case 0xF0: BRANCH_ZS(2);

/* note: bit moet 5 instr zijn ipv 2? */
/* BIT  -  Test Bits in Memory with Accumulator */
case 0x24: BIT_TEST_ZP(3);
case 0x2C: BIT_TEST_A(4);

/* BMI  -  Branch on Result Minus */
case 0x30: BRANCH_RM(2);

/* BNE  -  Branch on Z reset */
case 0xD0: BRANCH_ZR(2);

/* BPL  -  Branch on Result Plus (or Positive) */
case 0x10: BRANCH_RP(2);

/* BRA? */

/* BRK  -  Force a Break */
case 0x00: BREAK(7);

/* BVC  -  Branch on Overflow Clear */
case 0x50: BRANCH_OC(2);

/* BVS  -  Branch on Overflow Set */
case 0x70: BRANCH_OS(4);

/* CLC  -  Clear Carry Flag */
case 0x18: CLEAR_CF(2);

/* CLD  -  Clear Decimal Mode */
case 0xD8: CLEAR_DM(2);
 
/* CLI  -  Clear Interrupt Disable */
case 0x58: CLEAR_ID(2);

/* CLV  -  Clear Overflow Flag */
case 0xB8: CLEAR_OF(2);

/* CMP  -  Compare Memory and Accumulator */
case 0xC9: COMP_MEM_IM(accumulator,2);
case 0xC5: COMP_MEM_ZP(accumulator,3);
case 0xD5: COMP_MEM_ZPIX(accumulator,4);
case 0xCD: COMP_MEM_A(accumulator,4);
case 0xDD: COMP_MEM_AIX(accumulator,5);
case 0xD9: COMP_MEM_AIY(accumulator,4);
case 0xC1: COMP_MEM_IDI(accumulator,6);
case 0xD1: COMP_MEM_INI(accumulator,6);

/* CPX  -  Compare Memory and X register */
case 0xE0: COMP_MEM_IM(x_reg,2);
case 0xE4: COMP_MEM_ZP(x_reg,3);
case 0xEC: COMP_MEM_A(x_reg,4);

/* CPY  -  Compare Memory and Y register */
case 0xC0: COMP_MEM_IM(y_reg,2);
case 0xC4: COMP_MEM_ZP(y_reg,3);
case 0xCC: COMP_MEM_A(y_reg,4);

/* DEA?? */

/* DEC  -  Decrement Memory by One */
case 0xC6: DECR_MEM_ZP(5);
case 0xD6: DECR_MEM_ZPIX(6);
case 0xCE: DECR_MEM_A(6);
case 0xDE: DECR_MEM_AIX(7);

/* DEX  -  Decrement X */
case 0xCA: DECR(x_reg,2);

/* DEY  -  Decrement Y */
case 0x88: DECR(y_reg,2);

/* EOR  -  Exclusive-OR Memory with Accumulator */
case 0x49: EXCL_OR_MEM_IM(2);
case 0x45: EXCL_OR_MEM_ZP(3);
case 0x55: EXCL_OR_MEM_ZPIX(4);
case 0x4D: EXCL_OR_MEM_A(6);
case 0x5D: EXCL_OR_MEM_AIX(5);
case 0x59: EXCL_OR_MEM_AIY(4);
case 0x41: EXCL_OR_MEM_IDI(6);
case 0x51: EXCL_OR_MEM_INI(5);

/* INC  -  Increment Memory by one */
case 0xE6: INCR_MEM_ZP(5);
case 0xF6: INCR_MEM_ZPIX(6);
case 0xEE: INCR_MEM_A(6);
case 0xFE: INCR_MEM_AIX(7);

/* INX  -  Increment X by one */
case 0xE8: INCR(x_reg,2);

/* INY  -  Increment Y by one */
case 0xC8: INCR(y_reg,2);

/* mis nog 1 JMP instructie */
/* JMP - Jump */
case 0x4c: JMP_A(3);
case 0x6c: JMP_AI(5);

/* JSR - Jump to subroutine */
case 0x20: JSR(6);

/* LDA - Load Accumulator with memory */
case 0xA9: LOAD_IM(accumulator,2);
case 0xA5: LOAD_ZP(accumulator,3);
case 0xB5: LOAD_ZPIX(accumulator,4);
case 0xAD: LOAD_A(accumulator,4);
case 0xBD: LOAD_AIX(accumulator,4);
case 0xB9: LOAD_AIY(accumulator,4);
case 0xA1: LOAD_IDI(accumulator,6);
case 0xB1: LOAD_INI(accumulator,5);

/* LDX - Load X with Memory */
case 0xA2: LOAD_IM(x_reg,2);
case 0xA6: LOAD_ZP(x_reg,3);
case 0xB6: LOAD_ZPIY(x_reg,4);
case 0xAE: LOAD_A(x_reg,4);
case 0xBE: LOAD_AIY(x_reg,4);

/* LDY - Load Y with Memory */
case 0xA0: LOAD_IM(y_reg,2);
case 0xA4: LOAD_ZP(y_reg,3);
case 0xB4: LOAD_ZPIX(y_reg,4);
case 0xAC: LOAD_A(y_reg,4);
case 0xBC: LOAD_AIX(y_reg,4);

/* LSR  -  Logical Shift Right */
case 0x4A: LOGIC_SHIFT_R_ACC(2);
case 0x46: LOGIC_SHIFT_R_ZP(5);
case 0x56: LOGIC_SHIFT_R_ZPIX(6);
case 0x4E: LOGIC_SHIFT_R_A(6);
case 0x5E: LOGIC_SHIFT_R_AIX(7);

/* NOP - No Operation (79 instructies?) */
case 0xEA: NOP(2);

/* ORA  -  OR Memory with Accumulator */
case 0x09: OR_MEM_IM(2);
case 0x05: OR_MEM_ZP(3);
case 0x15: OR_MEM_ZPIX(4);
case 0x0D: OR_MEM_A(4);
case 0x1D: OR_MEM_AIX(5);
case 0x19: OR_MEM_AIY(4);
case 0x01: OR_MEM_IDI(6);
case 0x11: OR_MEM_INI(5);

/* PHA  -  Push Accumulator on Stack */
case 0x48: PUSH_A(accumulator,3);

/* PHP  -  Push Processor Status on Stack */
case 0x08: PUSH_PS(3);

/* PHX? */

/* PHY? */

/* PLA  -  Pull Accumulator from Stack */
case 0x68: PULL_A(accumulator,4);

/* PLP  -  Pull Processor Status from Stack */
case 0x28: PULL_PS(4);

/* PLX? */

/* PLY? */

/* ROL  -  Rotate Left */
case 0x2A: ROTATE_LEFT_ACC(2);
case 0x26: ROTATE_LEFT_ZP(5);
case 0x36: ROTATE_LEFT_ZPIX(6);
case 0x2E: ROTATE_LEFT_A(6);
case 0x3E: ROTATE_LEFT_AIX(7);

/* ROR  -  Rotate Right */
case 0x6A: ROTATE_RIGHT_ACC(2);
case 0x66: ROTATE_RIGHT_ZP(5);
case 0x76: ROTATE_RIGHT_ZPIX(6);
case 0x6E: ROTATE_RIGHT_A(6);
case 0x7E: ROTATE_RIGHT_AIX(7);

/* RTI  -  Return from Interrupt */
case 0x40: RET_INT(4);

/* RTS  -  Return from Subroutine */
case 0x60: RET_SUB(4);

/* SBC  -  Subtract from Accumulator with Carry (IDI_ZP?) */
case 0xE9: SUB_ACC_IM(2);
case 0xE5: SUB_ACC_ZP(3);
case 0xF5: SUB_ACC_ZPIX(4);
case 0xED: SUB_ACC_A(4);
case 0xFD: SUB_ACC_AIX(5);
case 0xF9: SUB_ACC_AIY(4);
case 0xE1: SUB_ACC_IDI(6);
case 0xF1: SUB_ACC_INI(5);

/* SEC  -  Set Carry Flag */
case 0x38: SET_C_FLAG(2);

/* SED  -  Set Decimal Mode */
case 0xF8: SET_D_MODE(2);

/* SEI - Set Interrupt Disable */
case 0x78: SET_INT_DIS(2);

/* STA - Store Accumulator in Memory (IDI_ZP?) */
case 0x85: STORE_ZP(accumulator,3);
case 0x95: STORE_ZPIX(accumulator,4);
case 0x8D: STORE_A(accumulator,4);
case 0x9D: STORE_AIX(accumulator,5);
case 0x99: STORE_AIY(accumulator,5);
case 0x81: STORE_IDI(accumulator,6);
case 0x91: STORE_INI(accumulator,6);

/* STX - Store X in Memory */
case 0x86: STORE_ZP(x_reg,3);
case 0x96: STORE_ZPIY(x_reg,4);
case 0x8E: STORE_A(x_reg,4);

/* STY - Store Y in Memory */
case 0x84: STORE_ZP(y_reg,3);
case 0x94: STORE_ZPIX(y_reg,4);
case 0x8C: STORE_A(y_reg,4);

/* STZ? */

/* TAX  -  Transfer Accumulator to X */
case 0xAA: TRANSFER_REG(accumulator,x_reg,2);

/* TAY  -  Transfer Accumulator to Y */
case 0xA8: TRANSFER_REG(accumulator,y_reg,2);

/* TRB? */

/* TSB? */

/* TSX  -  Transfer Stack to X */
case 0xBA: TRANSFER_STACK_FROM(x_reg,2);

/* TXA  -  Transfer X to Accumulator */
case 0x8A: TRANSFER_REG(x_reg,accumulator,2);

/* TXS  -  Transfer X to Stack */
case 0x9A: TRANSFER_STACK_TO(x_reg,2);

/* TYA  -  Transfer Y to Accumulator */
case 0x98: TRANSFER_REG(y_reg,accumulator,2);

/* Unrecognized instructions */
default:

break;

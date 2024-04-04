/*****************************************************************************
 *
 *   z80.c
 *   Portable Z80 emulator V3.9
 *
 *   Copyright Juergen Buchmueller, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     pullmoll@t-online.de
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *   TODO:
 *    - If LD A,I or LD A,R is interrupted, P/V flag gets reset, even if IFF2
 *      was set before this instruction
 *    - Ideally, the tiny differences between Z80 types should be supported,
 *      currently known differences:
 *       - LD A,I/R P/V flag reset glitch is fixed on CMOS Z80
 *       - OUT (C),0 outputs 0 on NMOS Z80, $FF on CMOS Z80
 *       - SCF/CCF X/Y flags is ((flags | A) & 0x28) on SGS/SHARP/ZiLOG NMOS Z80,
 *         (flags & A & 0x28) on NEC NMOS Z80, other models unknown.
 *         However, people from the Speccy scene mention that SCF/CCF X/Y results
 *         are inconsistant and may be influenced by I and R registers.
 *      This Z80 emulator assumes a ZiLOG NMOS model.
 *
 *   Additional changes [Eke-Eke]:
 *    - Removed z80_burn function (unused)
 *    - Discarded multi-chip support (unused)
 *    - Fixed cycle counting for FD and DD prefixed instructions
 *    - Fixed behavior of chained FD and DD prefixes (R register should be only incremented by one
 *    - Implemented cycle-accurate INI/IND (needed by SMS emulation)
 *    - Fixed Z80 reset
 *    - Made SZHVC_add & SZHVC_sub tables statically allocated
 *    - Fixed compiler warning when BIG_SWITCH is defined
 *   Changes in 3.9:
 *    - Fixed cycle counts for LD IYL/IXL/IYH/IXH,n [Marshmellow]
 *    - Fixed X/Y flags in CCF/SCF/BIT, ZEXALL is happy now [hap]
 *    - Simplified DAA, renamed MEMPTR (3.8) to WZ, added TODO [hap]
 *    - Fixed IM2 interrupt cycles [eke]
 *   Changes in 3.8 [Miodrag Milanovic]:
 *   - Added MEMPTR register (according to informations provided
 *     by Vladimir Kladov
 *   - BIT n,(HL) now return valid values due to use of MEMPTR
 *   - Fixed BIT 6,(XY+o) undocumented instructions
 *   Changes in 3.7 [Aaron Giles]:
 *   - Changed NMI handling. NMIs are now latched in set_irq_state
 *     but are not taken there. Instead they are taken at the start of the
 *     execute loop.
 *   - Changed IRQ handling. IRQ state is set in set_irq_state but not taken
 *     except during the inner execute loop.
 *   - Removed x86 assembly hacks and obsolete timing loop catchers.
 *   Changes in 3.6:
 *   - Got rid of the code that would inexactly emulate a Z80, i.e. removed
 *     all the #if Z80_EXACT #else branches.
 *   - Removed leading underscores from local register name shortcuts as
 *     this violates the C99 standard.
 *   - Renamed the registers inside the Z80 context to lower case to avoid
 *     ambiguities (shortcuts would have had the same names as the fields
 *     of the structure).
 *   Changes in 3.5:
 *   - Implemented OTIR, INIR, etc. without look-up table for PF flag.
 *     [Ramsoft, Sean Young]
 *   Changes in 3.4:
 *   - Removed Z80-MSX specific code as it's not needed any more.
 *   - Implemented DAA without look-up table [Ramsoft, Sean Young]
 *   Changes in 3.3:
 *   - Fixed undocumented flags XF & YF in the non-asm versions of CP,
 *     and all the 16 bit arithmetic instructions. [Sean Young]
 *   Changes in 3.2:
 *   - Fixed undocumented flags XF & YF of RRCA, and CF and HF of
 *     INI/IND/OUTI/OUTD/INIR/INDR/OTIR/OTDR [Sean Young]
 *   Changes in 3.1:
 *   - removed the REPEAT_AT_ONCE execution of LDIR/CPIR etc. opcodes
 *     for readabilities sake and because the implementation was buggy
 *     (and I was not able to find the difference)
 *   Changes in 3.0:
 *   - 'finished' switch to dynamically overrideable cycle count tables
 *   Changes in 2.9:
 *   - added methods to access and override the cycle count tables
 *   - fixed handling and timing of multiple DD/FD prefixed opcodes
 *   Changes in 2.8:
 *   - OUTI/OUTD/OTIR/OTDR also pre-decrement the B register now.
 *     This was wrong because of a bug fix on the wrong side
 *     (astrocade sound driver).
 *   Changes in 2.7:
 *    - removed z80_vm specific code, it's not needed (and never was).
 *   Changes in 2.6:
 *    - BUSY_LOOP_HACKS needed to call change_pc() earlier, before
 *    checking the opcodes at the new address, because otherwise they
 *    might access the old (wrong or even NULL) banked memory region.
 *    Thanks to Sean Young for finding this nasty bug.
 *   Changes in 2.5:
 *    - Burning cycles always adjusts the ICount by a multiple of 4.
 *    - In REPEAT_AT_ONCE cases the R register wasn't incremented twice
 *    per repetition as it should have been. Those repeated opcodes
 *    could also underflow the ICount.
 *    - Simplified TIME_LOOP_HACKS for BC and added two more for DE + HL
 *    timing loops. I think those hacks weren't endian safe before too.
 *   Changes in 2.4:
 *    - z80_reset zaps the entire context, sets IX and IY to 0xffff(!) and
 *    sets the Z flag. With these changes the Tehkan World Cup driver
 *    _seems_ to work again.
 *   Changes in 2.3:
 *    - External termination of the execution loop calls z80_burn() and
 *    z80_vm_burn() to burn an amount of cycles (R adjustment)
 *    - Shortcuts which burn CPU cycles (BUSY_LOOP_HACKS and TIME_LOOP_HACKS)
 *    now also adjust the R register depending on the skipped opcodes.
 *   Changes in 2.2:
 *    - Fixed bugs in CPL, SCF and CCF instructions flag handling.
 *    - Changed variable EA and ARG16() function to UINT32; this
 *    produces slightly more efficient code.
 *    - The DD/FD XY CB opcodes where XY is 40-7F and Y is not 6/E
 *    are changed to calls to the X6/XE opcodes to reduce object size.
 *    They're hardly ever used so this should not yield a speed penalty.
 *   New in 2.0:
 *    - Optional more exact Z80 emulation (#define Z80_EXACT 1) according
 *    to a detailed description by Sean Young which can be found at:
 *      http://www.msxnet.org/tech/z80-documented.pdf
 *****************************************************************************/

#include "gpgx/cpu/z80/z80.h"

#include "gpgx/cpu/z80/z80_macro.h"
#include "gpgx/cpu/z80/z80_table_index.h"

namespace gpgx::cpu::z80 {

//==============================================================================
// Z80

//------------------------------------------------------------------------------

#define OP(prefix,opcode)  void Z80::prefix##_##opcode(void)


//------------------------------------------------------------------------------
// opcodes with CB prefix rotate, shift and bit operations.

OP(cb,00) { B = RLC(B);                      } /// RLC  B
OP(cb,01) { C = RLC(C);                      } /// RLC  C
OP(cb,02) { D = RLC(D);                      } /// RLC  D
OP(cb,03) { E = RLC(E);                      } /// RLC  E
OP(cb,04) { H = RLC(H);                      } /// RLC  H
OP(cb,05) { L = RLC(L);                      } /// RLC  L
OP(cb,06) { WM(HL, RLC(RM(HL)));             } /// RLC  (HL)
OP(cb,07) { A = RLC(A);                      } /// RLC  A

OP(cb,08) { B = RRC(B);                      } /// RRC  B
OP(cb,09) { C = RRC(C);                      } /// RRC  C
OP(cb,0a) { D = RRC(D);                      } /// RRC  D
OP(cb,0b) { E = RRC(E);                      } /// RRC  E
OP(cb,0c) { H = RRC(H);                      } /// RRC  H
OP(cb,0d) { L = RRC(L);                      } /// RRC  L
OP(cb,0e) { WM(HL, RRC(RM(HL)));             } /// RRC  (HL)
OP(cb,0f) { A = RRC(A);                      } /// RRC  A

OP(cb,10) { B = RL(B);                       } /// RL   B
OP(cb,11) { C = RL(C);                       } /// RL   C
OP(cb,12) { D = RL(D);                       } /// RL   D
OP(cb,13) { E = RL(E);                       } /// RL   E
OP(cb,14) { H = RL(H);                       } /// RL   H
OP(cb,15) { L = RL(L);                       } /// RL   L
OP(cb,16) { WM(HL, RL(RM(HL)));              } /// RL   (HL)
OP(cb,17) { A = RL(A);                       } /// RL   A

OP(cb,18) { B = RR(B);                       } /// RR   B
OP(cb,19) { C = RR(C);                       } /// RR   C
OP(cb,1a) { D = RR(D);                       } /// RR   D
OP(cb,1b) { E = RR(E);                       } /// RR   E
OP(cb,1c) { H = RR(H);                       } /// RR   H
OP(cb,1d) { L = RR(L);                       } /// RR   L
OP(cb,1e) { WM(HL, RR(RM(HL)));              } /// RR   (HL)
OP(cb,1f) { A = RR(A);                       } /// RR   A

OP(cb,20) { B = SLA(B);                      } /// SLA  B
OP(cb,21) { C = SLA(C);                      } /// SLA  C
OP(cb,22) { D = SLA(D);                      } /// SLA  D
OP(cb,23) { E = SLA(E);                      } /// SLA  E
OP(cb,24) { H = SLA(H);                      } /// SLA  H
OP(cb,25) { L = SLA(L);                      } /// SLA  L
OP(cb,26) { WM(HL, SLA(RM(HL)));             } /// SLA  (HL)
OP(cb,27) { A = SLA(A);                      } /// SLA  A

OP(cb,28) { B = SRA(B);                      } /// SRA  B
OP(cb,29) { C = SRA(C);                      } /// SRA  C
OP(cb,2a) { D = SRA(D);                      } /// SRA  D
OP(cb,2b) { E = SRA(E);                      } /// SRA  E
OP(cb,2c) { H = SRA(H);                      } /// SRA  H
OP(cb,2d) { L = SRA(L);                      } /// SRA  L
OP(cb,2e) { WM(HL, SRA(RM(HL)));             } /// SRA  (HL)
OP(cb,2f) { A = SRA(A);                      } /// SRA  A

OP(cb,30) { B = SLL(B);                      } /// SLL  B
OP(cb,31) { C = SLL(C);                      } /// SLL  C
OP(cb,32) { D = SLL(D);                      } /// SLL  D
OP(cb,33) { E = SLL(E);                      } /// SLL  E
OP(cb,34) { H = SLL(H);                      } /// SLL  H
OP(cb,35) { L = SLL(L);                      } /// SLL  L
OP(cb,36) { WM(HL, SLL(RM(HL)));             } /// SLL  (HL)
OP(cb,37) { A = SLL(A);                      } /// SLL  A

OP(cb,38) { B = SRL(B);                      } /// SRL  B
OP(cb,39) { C = SRL(C);                      } /// SRL  C
OP(cb,3a) { D = SRL(D);                      } /// SRL  D
OP(cb,3b) { E = SRL(E);                      } /// SRL  E
OP(cb,3c) { H = SRL(H);                      } /// SRL  H
OP(cb,3d) { L = SRL(L);                      } /// SRL  L
OP(cb,3e) { WM(HL, SRL(RM(HL)));             } /// SRL  (HL)
OP(cb,3f) { A = SRL(A);                      } /// SRL  A

OP(cb,40) { BIT(0, B);                       } /// BIT  0,B
OP(cb,41) { BIT(0, C);                       } /// BIT  0,C
OP(cb,42) { BIT(0, D);                       } /// BIT  0,D
OP(cb,43) { BIT(0, E);                       } /// BIT  0,E
OP(cb,44) { BIT(0, H);                       } /// BIT  0,H
OP(cb,45) { BIT(0, L);                       } /// BIT  0,L
OP(cb,46) { BIT_HL(0, RM(HL));               } /// BIT  0,(HL)
OP(cb,47) { BIT(0, A);                       } /// BIT  0,A

OP(cb,48) { BIT(1, B);                       } /// BIT  1,B
OP(cb,49) { BIT(1, C);                       } /// BIT  1,C
OP(cb,4a) { BIT(1, D);                       } /// BIT  1,D
OP(cb,4b) { BIT(1, E);                       } /// BIT  1,E
OP(cb,4c) { BIT(1, H);                       } /// BIT  1,H
OP(cb,4d) { BIT(1, L);                       } /// BIT  1,L
OP(cb,4e) { BIT_HL(1, RM(HL));               } /// BIT  1,(HL)
OP(cb,4f) { BIT(1, A);                       } /// BIT  1,A

OP(cb,50) { BIT(2, B);                       } /// BIT  2,B
OP(cb,51) { BIT(2, C);                       } /// BIT  2,C
OP(cb,52) { BIT(2, D);                       } /// BIT  2,D
OP(cb,53) { BIT(2, E);                       } /// BIT  2,E
OP(cb,54) { BIT(2, H);                       } /// BIT  2,H
OP(cb,55) { BIT(2, L);                       } /// BIT  2,L
OP(cb,56) { BIT_HL(2, RM(HL));               } /// BIT  2,(HL)
OP(cb,57) { BIT(2, A);                       } /// BIT  2,A

OP(cb,58) { BIT(3, B);                       } /// BIT  3,B
OP(cb,59) { BIT(3, C);                       } /// BIT  3,C
OP(cb,5a) { BIT(3, D);                       } /// BIT  3,D
OP(cb,5b) { BIT(3, E);                       } /// BIT  3,E
OP(cb,5c) { BIT(3, H);                       } /// BIT  3,H
OP(cb,5d) { BIT(3, L);                       } /// BIT  3,L
OP(cb,5e) { BIT_HL(3, RM(HL));               } /// BIT  3,(HL)
OP(cb,5f) { BIT(3, A);                       } /// BIT  3,A

OP(cb,60) { BIT(4, B);                       } /// BIT  4,B
OP(cb,61) { BIT(4, C);                       } /// BIT  4,C
OP(cb,62) { BIT(4, D);                       } /// BIT  4,D
OP(cb,63) { BIT(4, E);                       } /// BIT  4,E
OP(cb,64) { BIT(4, H);                       } /// BIT  4,H
OP(cb,65) { BIT(4, L);                       } /// BIT  4,L
OP(cb,66) { BIT_HL(4, RM(HL));               } /// BIT  4,(HL)
OP(cb,67) { BIT(4, A);                       } /// BIT  4,A

OP(cb,68) { BIT(5, B);                       } /// BIT  5,B
OP(cb,69) { BIT(5, C);                       } /// BIT  5,C
OP(cb,6a) { BIT(5, D);                       } /// BIT  5,D
OP(cb,6b) { BIT(5, E);                       } /// BIT  5,E
OP(cb,6c) { BIT(5, H);                       } /// BIT  5,H
OP(cb,6d) { BIT(5, L);                       } /// BIT  5,L
OP(cb,6e) { BIT_HL(5, RM(HL));               } /// BIT  5,(HL)
OP(cb,6f) { BIT(5, A);                       } /// BIT  5,A

OP(cb,70) { BIT(6, B);                       } /// BIT  6,B
OP(cb,71) { BIT(6, C);                       } /// BIT  6,C
OP(cb,72) { BIT(6, D);                       } /// BIT  6,D
OP(cb,73) { BIT(6, E);                       } /// BIT  6,E
OP(cb,74) { BIT(6, H);                       } /// BIT  6,H
OP(cb,75) { BIT(6, L);                       } /// BIT  6,L
OP(cb,76) { BIT_HL(6, RM(HL));               } /// BIT  6,(HL)
OP(cb,77) { BIT(6, A);                       } /// BIT  6,A

OP(cb,78) { BIT(7, B);                       } /// BIT  7,B
OP(cb,79) { BIT(7, C);                       } /// BIT  7,C
OP(cb,7a) { BIT(7, D);                       } /// BIT  7,D
OP(cb,7b) { BIT(7, E);                       } /// BIT  7,E
OP(cb,7c) { BIT(7, H);                       } /// BIT  7,H
OP(cb,7d) { BIT(7, L);                       } /// BIT  7,L
OP(cb,7e) { BIT_HL(7, RM(HL));               } /// BIT  7,(HL)
OP(cb,7f) { BIT(7, A);                       } /// BIT  7,A

OP(cb,80) { B = RES(0, B);                   } /// RES  0,B
OP(cb,81) { C = RES(0, C);                   } /// RES  0,C
OP(cb,82) { D = RES(0, D);                   } /// RES  0,D
OP(cb,83) { E = RES(0, E);                   } /// RES  0,E
OP(cb,84) { H = RES(0, H);                   } /// RES  0,H
OP(cb,85) { L = RES(0, L);                   } /// RES  0,L
OP(cb,86) { WM(HL, RES(0, RM(HL)));          } /// RES  0,(HL)
OP(cb,87) { A = RES(0, A);                   } /// RES  0,A

OP(cb,88) { B = RES(1, B);                   } /// RES  1,B
OP(cb,89) { C = RES(1, C);                   } /// RES  1,C
OP(cb,8a) { D = RES(1, D);                   } /// RES  1,D
OP(cb,8b) { E = RES(1, E);                   } /// RES  1,E
OP(cb,8c) { H = RES(1, H);                   } /// RES  1,H
OP(cb,8d) { L = RES(1, L);                   } /// RES  1,L
OP(cb,8e) { WM(HL, RES(1, RM(HL)));          } /// RES  1,(HL)
OP(cb,8f) { A = RES(1, A);                   } /// RES  1,A

OP(cb,90) { B = RES(2, B);                   } /// RES  2,B
OP(cb,91) { C = RES(2, C);                   } /// RES  2,C
OP(cb,92) { D = RES(2, D);                   } /// RES  2,D
OP(cb,93) { E = RES(2, E);                   } /// RES  2,E
OP(cb,94) { H = RES(2, H);                   } /// RES  2,H
OP(cb,95) { L = RES(2, L);                   } /// RES  2,L
OP(cb,96) { WM(HL, RES(2, RM(HL)));          } /// RES  2,(HL)
OP(cb,97) { A = RES(2, A);                   } /// RES  2,A

OP(cb,98) { B = RES(3, B);                   } /// RES  3,B
OP(cb,99) { C = RES(3, C);                   } /// RES  3,C
OP(cb,9a) { D = RES(3, D);                   } /// RES  3,D
OP(cb,9b) { E = RES(3, E);                   } /// RES  3,E
OP(cb,9c) { H = RES(3, H);                   } /// RES  3,H
OP(cb,9d) { L = RES(3, L);                   } /// RES  3,L
OP(cb,9e) { WM(HL, RES(3, RM(HL)));          } /// RES  3,(HL)
OP(cb,9f) { A = RES(3, A);                   } /// RES  3,A

OP(cb,a0) { B = RES(4, B);                   } /// RES  4,B
OP(cb,a1) { C = RES(4, C);                   } /// RES  4,C
OP(cb,a2) { D = RES(4, D);                   } /// RES  4,D
OP(cb,a3) { E = RES(4, E);                   } /// RES  4,E
OP(cb,a4) { H = RES(4, H);                   } /// RES  4,H
OP(cb,a5) { L = RES(4, L);                   } /// RES  4,L
OP(cb,a6) { WM(HL, RES(4, RM(HL)));          } /// RES  4,(HL)
OP(cb,a7) { A = RES(4, A);                   } /// RES  4,A

OP(cb,a8) { B = RES(5, B);                   } /// RES  5,B
OP(cb,a9) { C = RES(5, C);                   } /// RES  5,C
OP(cb,aa) { D = RES(5, D);                   } /// RES  5,D
OP(cb,ab) { E = RES(5, E);                   } /// RES  5,E
OP(cb,ac) { H = RES(5, H);                   } /// RES  5,H
OP(cb,ad) { L = RES(5, L);                   } /// RES  5,L
OP(cb,ae) { WM(HL, RES(5, RM(HL)));          } /// RES  5,(HL)
OP(cb,af) { A = RES(5, A);                   } /// RES  5,A

OP(cb,b0) { B = RES(6, B);                   } /// RES  6,B
OP(cb,b1) { C = RES(6, C);                   } /// RES  6,C
OP(cb,b2) { D = RES(6, D);                   } /// RES  6,D
OP(cb,b3) { E = RES(6, E);                   } /// RES  6,E
OP(cb,b4) { H = RES(6, H);                   } /// RES  6,H
OP(cb,b5) { L = RES(6, L);                   } /// RES  6,L
OP(cb,b6) { WM(HL, RES(6, RM(HL)));          } /// RES  6,(HL)
OP(cb,b7) { A = RES(6, A);                   } /// RES  6,A

OP(cb,b8) { B = RES(7, B);                   } /// RES  7,B
OP(cb,b9) { C = RES(7, C);                   } /// RES  7,C
OP(cb,ba) { D = RES(7, D);                   } /// RES  7,D
OP(cb,bb) { E = RES(7, E);                   } /// RES  7,E
OP(cb,bc) { H = RES(7, H);                   } /// RES  7,H
OP(cb,bd) { L = RES(7, L);                   } /// RES  7,L
OP(cb,be) { WM(HL, RES(7, RM(HL)));          } /// RES  7,(HL)
OP(cb,bf) { A = RES(7, A);                   } /// RES  7,A

OP(cb,c0) { B = SET(0, B);                   } /// SET  0,B
OP(cb,c1) { C = SET(0, C);                   } /// SET  0,C
OP(cb,c2) { D = SET(0, D);                   } /// SET  0,D
OP(cb,c3) { E = SET(0, E);                   } /// SET  0,E
OP(cb,c4) { H = SET(0, H);                   } /// SET  0,H
OP(cb,c5) { L = SET(0, L);                   } /// SET  0,L
OP(cb,c6) { WM(HL, SET(0, RM(HL)));          } /// SET  0,(HL)
OP(cb,c7) { A = SET(0, A);                   } /// SET  0,A

OP(cb,c8) { B = SET(1, B);                   } /// SET  1,B
OP(cb,c9) { C = SET(1, C);                   } /// SET  1,C
OP(cb,ca) { D = SET(1, D);                   } /// SET  1,D
OP(cb,cb) { E = SET(1, E);                   } /// SET  1,E
OP(cb,cc) { H = SET(1, H);                   } /// SET  1,H
OP(cb,cd) { L = SET(1, L);                   } /// SET  1,L
OP(cb,ce) { WM(HL, SET(1, RM(HL)));          } /// SET  1,(HL)
OP(cb,cf) { A = SET(1, A);                   } /// SET  1,A

OP(cb,d0) { B = SET(2, B);                   } /// SET  2,B
OP(cb,d1) { C = SET(2, C);                   } /// SET  2,C
OP(cb,d2) { D = SET(2, D);                   } /// SET  2,D
OP(cb,d3) { E = SET(2, E);                   } /// SET  2,E
OP(cb,d4) { H = SET(2, H);                   } /// SET  2,H
OP(cb,d5) { L = SET(2, L);                   } /// SET  2,L
OP(cb,d6) { WM(HL, SET(2, RM(HL)));          } /// SET  2,(HL)
OP(cb,d7) { A = SET(2, A);                   } /// SET  2,A

OP(cb,d8) { B = SET(3, B);                   } /// SET  3,B
OP(cb,d9) { C = SET(3, C);                   } /// SET  3,C
OP(cb,da) { D = SET(3, D);                   } /// SET  3,D
OP(cb,db) { E = SET(3, E);                   } /// SET  3,E
OP(cb,dc) { H = SET(3, H);                   } /// SET  3,H
OP(cb,dd) { L = SET(3, L);                   } /// SET  3,L
OP(cb,de) { WM(HL, SET(3, RM(HL)));          } /// SET  3,(HL)
OP(cb,df) { A = SET(3, A);                   } /// SET  3,A

OP(cb,e0) { B = SET(4, B);                   } /// SET  4,B
OP(cb,e1) { C = SET(4, C);                   } /// SET  4,C
OP(cb,e2) { D = SET(4, D);                   } /// SET  4,D
OP(cb,e3) { E = SET(4, E);                   } /// SET  4,E
OP(cb,e4) { H = SET(4, H);                   } /// SET  4,H
OP(cb,e5) { L = SET(4, L);                   } /// SET  4,L
OP(cb,e6) { WM(HL, SET(4, RM(HL)));          } /// SET  4,(HL)
OP(cb,e7) { A = SET(4, A);                   } /// SET  4,A

OP(cb,e8) { B = SET(5, B);                   } /// SET  5,B
OP(cb,e9) { C = SET(5, C);                   } /// SET  5,C
OP(cb,ea) { D = SET(5, D);                   } /// SET  5,D
OP(cb,eb) { E = SET(5, E);                   } /// SET  5,E
OP(cb,ec) { H = SET(5, H);                   } /// SET  5,H
OP(cb,ed) { L = SET(5, L);                   } /// SET  5,L
OP(cb,ee) { WM(HL, SET(5, RM(HL)));          } /// SET  5,(HL)
OP(cb,ef) { A = SET(5, A);                   } /// SET  5,A

OP(cb,f0) { B = SET(6, B);                   } /// SET  6,B
OP(cb,f1) { C = SET(6, C);                   } /// SET  6,C
OP(cb,f2) { D = SET(6, D);                   } /// SET  6,D
OP(cb,f3) { E = SET(6, E);                   } /// SET  6,E
OP(cb,f4) { H = SET(6, H);                   } /// SET  6,H
OP(cb,f5) { L = SET(6, L);                   } /// SET  6,L
OP(cb,f6) { WM(HL, SET(6, RM(HL)));          } /// SET  6,(HL)
OP(cb,f7) { A = SET(6, A);                   } /// SET  6,A

OP(cb,f8) { B = SET(7, B);                   } /// SET  7,B
OP(cb,f9) { C = SET(7, C);                   } /// SET  7,C
OP(cb,fa) { D = SET(7, D);                   } /// SET  7,D
OP(cb,fb) { E = SET(7, E);                   } /// SET  7,E
OP(cb,fc) { H = SET(7, H);                   } /// SET  7,H
OP(cb,fd) { L = SET(7, L);                   } /// SET  7,L
OP(cb,fe) { WM(HL, SET(7, RM(HL)));          } /// SET  7,(HL)
OP(cb,ff) { A = SET(7, A);                   } /// SET  7,A

//------------------------------------------------------------------------------
// opcodes with DD/FD CB prefix rotate, shift and bit operations with (IX+o).

OP(xycb,00) { B = RLC(RM(m_EA)); WM(m_EA, B);              } /// RLC  B=(XY+o)
OP(xycb,01) { C = RLC(RM(m_EA)); WM(m_EA, C);              } /// RLC  C=(XY+o)
OP(xycb,02) { D = RLC(RM(m_EA)); WM(m_EA, D);              } /// RLC  D=(XY+o)
OP(xycb,03) { E = RLC(RM(m_EA)); WM(m_EA, E);              } /// RLC  E=(XY+o)
OP(xycb,04) { H = RLC(RM(m_EA)); WM(m_EA, H);              } /// RLC  H=(XY+o)
OP(xycb,05) { L = RLC(RM(m_EA)); WM(m_EA, L);              } /// RLC  L=(XY+o)
OP(xycb,06) { WM(m_EA, RLC(RM(m_EA)));                     } /// RLC  (XY+o)
OP(xycb,07) { A = RLC(RM(m_EA)); WM(m_EA, A);              } /// RLC  A=(XY+o)

OP(xycb,08) { B = RRC(RM(m_EA)); WM(m_EA, B);              } /// RRC  B=(XY+o)
OP(xycb,09) { C = RRC(RM(m_EA)); WM(m_EA, C);              } /// RRC  C=(XY+o)
OP(xycb,0a) { D = RRC(RM(m_EA)); WM(m_EA, D);              } /// RRC  D=(XY+o)
OP(xycb,0b) { E = RRC(RM(m_EA)); WM(m_EA, E);              } /// RRC  E=(XY+o)
OP(xycb,0c) { H = RRC(RM(m_EA)); WM(m_EA, H);              } /// RRC  H=(XY+o)
OP(xycb,0d) { L = RRC(RM(m_EA)); WM(m_EA, L);              } /// RRC  L=(XY+o)
OP(xycb,0e) { WM(m_EA, RRC(RM(m_EA)));                     } /// RRC  (XY+o)
OP(xycb,0f) { A = RRC(RM(m_EA)); WM(m_EA, A);              } /// RRC  A=(XY+o)

OP(xycb,10) { B = RL(RM(m_EA)); WM(m_EA, B);               } /// RL   B=(XY+o)
OP(xycb,11) { C = RL(RM(m_EA)); WM(m_EA, C);               } /// RL   C=(XY+o)
OP(xycb,12) { D = RL(RM(m_EA)); WM(m_EA, D);               } /// RL   D=(XY+o)
OP(xycb,13) { E = RL(RM(m_EA)); WM(m_EA, E);               } /// RL   E=(XY+o)
OP(xycb,14) { H = RL(RM(m_EA)); WM(m_EA, H);               } /// RL   H=(XY+o)
OP(xycb,15) { L = RL(RM(m_EA)); WM(m_EA, L);               } /// RL   L=(XY+o)
OP(xycb,16) { WM(m_EA, RL(RM(m_EA)));                      } /// RL   (XY+o)
OP(xycb,17) { A = RL(RM(m_EA)); WM(m_EA, A);               } /// RL   A=(XY+o)

OP(xycb,18) { B = RR(RM(m_EA)); WM(m_EA, B);               } /// RR   B=(XY+o)
OP(xycb,19) { C = RR(RM(m_EA)); WM(m_EA, C);               } /// RR   C=(XY+o)
OP(xycb,1a) { D = RR(RM(m_EA)); WM(m_EA, D);               } /// RR   D=(XY+o)
OP(xycb,1b) { E = RR(RM(m_EA)); WM(m_EA, E);               } /// RR   E=(XY+o)
OP(xycb,1c) { H = RR(RM(m_EA)); WM(m_EA, H);               } /// RR   H=(XY+o)
OP(xycb,1d) { L = RR(RM(m_EA)); WM(m_EA, L);               } /// RR   L=(XY+o)
OP(xycb,1e) { WM(m_EA, RR(RM(m_EA)));                      } /// RR   (XY+o)
OP(xycb,1f) { A = RR(RM(m_EA)); WM(m_EA, A);               } /// RR   A=(XY+o)

OP(xycb,20) { B = SLA(RM(m_EA)); WM(m_EA, B);              } /// SLA  B=(XY+o)
OP(xycb,21) { C = SLA(RM(m_EA)); WM(m_EA, C);              } /// SLA  C=(XY+o)
OP(xycb,22) { D = SLA(RM(m_EA)); WM(m_EA, D);              } /// SLA  D=(XY+o)
OP(xycb,23) { E = SLA(RM(m_EA)); WM(m_EA, E);              } /// SLA  E=(XY+o)
OP(xycb,24) { H = SLA(RM(m_EA)); WM(m_EA, H);              } /// SLA  H=(XY+o)
OP(xycb,25) { L = SLA(RM(m_EA)); WM(m_EA, L);              } /// SLA  L=(XY+o)
OP(xycb,26) { WM(m_EA, SLA(RM(m_EA)));                     } /// SLA  (XY+o)
OP(xycb,27) { A = SLA(RM(m_EA)); WM(m_EA, A);              } /// SLA  A=(XY+o)

OP(xycb,28) { B = SRA(RM(m_EA)); WM(m_EA, B);              } /// SRA  B=(XY+o)
OP(xycb,29) { C = SRA(RM(m_EA)); WM(m_EA, C);              } /// SRA  C=(XY+o)
OP(xycb,2a) { D = SRA(RM(m_EA)); WM(m_EA, D);              } /// SRA  D=(XY+o)
OP(xycb,2b) { E = SRA(RM(m_EA)); WM(m_EA, E);              } /// SRA  E=(XY+o)
OP(xycb,2c) { H = SRA(RM(m_EA)); WM(m_EA, H);              } /// SRA  H=(XY+o)
OP(xycb,2d) { L = SRA(RM(m_EA)); WM(m_EA, L);              } /// SRA  L=(XY+o)
OP(xycb,2e) { WM(m_EA, SRA(RM(m_EA)));                     } /// SRA  (XY+o)
OP(xycb,2f) { A = SRA(RM(m_EA)); WM(m_EA, A);              } /// SRA  A=(XY+o)

OP(xycb,30) { B = SLL(RM(m_EA)); WM(m_EA, B);              } /// SLL  B=(XY+o)
OP(xycb,31) { C = SLL(RM(m_EA)); WM(m_EA, C);              } /// SLL  C=(XY+o)
OP(xycb,32) { D = SLL(RM(m_EA)); WM(m_EA, D);              } /// SLL  D=(XY+o)
OP(xycb,33) { E = SLL(RM(m_EA)); WM(m_EA, E);              } /// SLL  E=(XY+o)
OP(xycb,34) { H = SLL(RM(m_EA)); WM(m_EA, H);              } /// SLL  H=(XY+o)
OP(xycb,35) { L = SLL(RM(m_EA)); WM(m_EA, L);              } /// SLL  L=(XY+o)
OP(xycb,36) { WM(m_EA, SLL(RM(m_EA)));                     } /// SLL  (XY+o)
OP(xycb,37) { A = SLL(RM(m_EA)); WM(m_EA, A);              } /// SLL  A=(XY+o)

OP(xycb,38) { B = SRL(RM(m_EA)); WM(m_EA, B);              } /// SRL  B=(XY+o)
OP(xycb,39) { C = SRL(RM(m_EA)); WM(m_EA, C);              } /// SRL  C=(XY+o)
OP(xycb,3a) { D = SRL(RM(m_EA)); WM(m_EA, D);              } /// SRL  D=(XY+o)
OP(xycb,3b) { E = SRL(RM(m_EA)); WM(m_EA, E);              } /// SRL  E=(XY+o)
OP(xycb,3c) { H = SRL(RM(m_EA)); WM(m_EA, H);              } /// SRL  H=(XY+o)
OP(xycb,3d) { L = SRL(RM(m_EA)); WM(m_EA, L);              } /// SRL  L=(XY+o)
OP(xycb,3e) { WM(m_EA, SRL(RM(m_EA)));                     } /// SRL  (XY+o)
OP(xycb,3f) { A = SRL(RM(m_EA)); WM(m_EA, A);              } /// SRL  A=(XY+o)

OP(xycb,40) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,41) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,42) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,43) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,44) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,45) { xycb_46();                                   } /// BIT  0,(XY+o)
OP(xycb,46) { BIT_XY(0, RM(m_EA));                         } /// BIT  0,(XY+o)
OP(xycb,47) { xycb_46();                                   } /// BIT  0,(XY+o)

OP(xycb,48) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,49) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,4a) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,4b) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,4c) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,4d) { xycb_4e();                                   } /// BIT  1,(XY+o)
OP(xycb,4e) { BIT_XY(1, RM(m_EA));                         } /// BIT  1,(XY+o)
OP(xycb,4f) { xycb_4e();                                   } /// BIT  1,(XY+o)

OP(xycb,50) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,51) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,52) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,53) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,54) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,55) { xycb_56();                                   } /// BIT  2,(XY+o)
OP(xycb,56) { BIT_XY(2, RM(m_EA));                         } /// BIT  2,(XY+o)
OP(xycb,57) { xycb_56();                                   } /// BIT  2,(XY+o)

OP(xycb,58) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,59) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,5a) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,5b) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,5c) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,5d) { xycb_5e();                                   } /// BIT  3,(XY+o)
OP(xycb,5e) { BIT_XY(3, RM(m_EA));                         } /// BIT  3,(XY+o)
OP(xycb,5f) { xycb_5e();                                   } /// BIT  3,(XY+o)

OP(xycb,60) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,61) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,62) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,63) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,64) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,65) { xycb_66();                                   } /// BIT  4,(XY+o)
OP(xycb,66) { BIT_XY(4, RM(m_EA));                         } /// BIT  4,(XY+o)
OP(xycb,67) { xycb_66();                                   } /// BIT  4,(XY+o)

OP(xycb,68) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,69) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,6a) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,6b) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,6c) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,6d) { xycb_6e();                                   } /// BIT  5,(XY+o)
OP(xycb,6e) { BIT_XY(5, RM(m_EA));                         } /// BIT  5,(XY+o)
OP(xycb,6f) { xycb_6e();                                   } /// BIT  5,(XY+o)

OP(xycb,70) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,71) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,72) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,73) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,74) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,75) { xycb_76();                                   } /// BIT  6,(XY+o)
OP(xycb,76) { BIT_XY(6, RM(m_EA));                         } /// BIT  6,(XY+o)
OP(xycb,77) { xycb_76();                                   } /// BIT  6,(XY+o)

OP(xycb,78) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,79) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,7a) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,7b) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,7c) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,7d) { xycb_7e();                                   } /// BIT  7,(XY+o)
OP(xycb,7e) { BIT_XY(7, RM(m_EA));                         } /// BIT  7,(XY+o)
OP(xycb,7f) { xycb_7e();                                   } /// BIT  7,(XY+o)

OP(xycb,80) { B = RES(0, RM(m_EA)); WM(m_EA, B);           } /// RES  0,B=(XY+o)
OP(xycb,81) { C = RES(0, RM(m_EA)); WM(m_EA, C);           } /// RES  0,C=(XY+o)
OP(xycb,82) { D = RES(0, RM(m_EA)); WM(m_EA, D);           } /// RES  0,D=(XY+o)
OP(xycb,83) { E = RES(0, RM(m_EA)); WM(m_EA, E);           } /// RES  0,E=(XY+o)
OP(xycb,84) { H = RES(0, RM(m_EA)); WM(m_EA, H);           } /// RES  0,H=(XY+o)
OP(xycb,85) { L = RES(0, RM(m_EA)); WM(m_EA, L);           } /// RES  0,L=(XY+o)
OP(xycb,86) { WM(m_EA, RES(0, RM(m_EA)));                  } /// RES  0,(XY+o)
OP(xycb,87) { A = RES(0, RM(m_EA)); WM(m_EA, A);           } /// RES  0,A=(XY+o)

OP(xycb,88) { B = RES(1, RM(m_EA)); WM(m_EA, B);           } /// RES  1,B=(XY+o)
OP(xycb,89) { C = RES(1, RM(m_EA)); WM(m_EA, C);           } /// RES  1,C=(XY+o)
OP(xycb,8a) { D = RES(1, RM(m_EA)); WM(m_EA, D);           } /// RES  1,D=(XY+o)
OP(xycb,8b) { E = RES(1, RM(m_EA)); WM(m_EA, E);           } /// RES  1,E=(XY+o)
OP(xycb,8c) { H = RES(1, RM(m_EA)); WM(m_EA, H);           } /// RES  1,H=(XY+o)
OP(xycb,8d) { L = RES(1, RM(m_EA)); WM(m_EA, L);           } /// RES  1,L=(XY+o)
OP(xycb,8e) { WM(m_EA, RES(1, RM(m_EA)));                  } /// RES  1,(XY+o)
OP(xycb,8f) { A = RES(1, RM(m_EA)); WM(m_EA, A);           } /// RES  1,A=(XY+o)

OP(xycb,90) { B = RES(2, RM(m_EA)); WM(m_EA, B);           } /// RES  2,B=(XY+o)
OP(xycb,91) { C = RES(2, RM(m_EA)); WM(m_EA, C);           } /// RES  2,C=(XY+o)
OP(xycb,92) { D = RES(2, RM(m_EA)); WM(m_EA, D);           } /// RES  2,D=(XY+o)
OP(xycb,93) { E = RES(2, RM(m_EA)); WM(m_EA, E);           } /// RES  2,E=(XY+o)
OP(xycb,94) { H = RES(2, RM(m_EA)); WM(m_EA, H);           } /// RES  2,H=(XY+o)
OP(xycb,95) { L = RES(2, RM(m_EA)); WM(m_EA, L);           } /// RES  2,L=(XY+o)
OP(xycb,96) { WM(m_EA, RES(2, RM(m_EA)));                  } /// RES  2,(XY+o)
OP(xycb,97) { A = RES(2, RM(m_EA)); WM(m_EA, A);           } /// RES  2,A=(XY+o)

OP(xycb,98) { B = RES(3, RM(m_EA)); WM(m_EA, B);           } /// RES  3,B=(XY+o)
OP(xycb,99) { C = RES(3, RM(m_EA)); WM(m_EA, C);           } /// RES  3,C=(XY+o)
OP(xycb,9a) { D = RES(3, RM(m_EA)); WM(m_EA, D);           } /// RES  3,D=(XY+o)
OP(xycb,9b) { E = RES(3, RM(m_EA)); WM(m_EA, E);           } /// RES  3,E=(XY+o)
OP(xycb,9c) { H = RES(3, RM(m_EA)); WM(m_EA, H);           } /// RES  3,H=(XY+o)
OP(xycb,9d) { L = RES(3, RM(m_EA)); WM(m_EA, L);           } /// RES  3,L=(XY+o)
OP(xycb,9e) { WM(m_EA, RES(3, RM(m_EA)));                  } /// RES  3,(XY+o)
OP(xycb,9f) { A = RES(3, RM(m_EA)); WM(m_EA, A);           } /// RES  3,A=(XY+o)

OP(xycb,a0) { B = RES(4, RM(m_EA)); WM(m_EA, B);           } /// RES  4,B=(XY+o)
OP(xycb,a1) { C = RES(4, RM(m_EA)); WM(m_EA, C);           } /// RES  4,C=(XY+o)
OP(xycb,a2) { D = RES(4, RM(m_EA)); WM(m_EA, D);           } /// RES  4,D=(XY+o)
OP(xycb,a3) { E = RES(4, RM(m_EA)); WM(m_EA, E);           } /// RES  4,E=(XY+o)
OP(xycb,a4) { H = RES(4, RM(m_EA)); WM(m_EA, H);           } /// RES  4,H=(XY+o)
OP(xycb,a5) { L = RES(4, RM(m_EA)); WM(m_EA, L);           } /// RES  4,L=(XY+o)
OP(xycb,a6) { WM(m_EA, RES(4, RM(m_EA)));                  } /// RES  4,(XY+o)
OP(xycb,a7) { A = RES(4, RM(m_EA)); WM(m_EA, A);           } /// RES  4,A=(XY+o)

OP(xycb,a8) { B = RES(5, RM(m_EA)); WM(m_EA, B);           } /// RES  5,B=(XY+o)
OP(xycb,a9) { C = RES(5, RM(m_EA)); WM(m_EA, C);           } /// RES  5,C=(XY+o)
OP(xycb,aa) { D = RES(5, RM(m_EA)); WM(m_EA, D);           } /// RES  5,D=(XY+o)
OP(xycb,ab) { E = RES(5, RM(m_EA)); WM(m_EA, E);           } /// RES  5,E=(XY+o)
OP(xycb,ac) { H = RES(5, RM(m_EA)); WM(m_EA, H);           } /// RES  5,H=(XY+o)
OP(xycb,ad) { L = RES(5, RM(m_EA)); WM(m_EA, L);           } /// RES  5,L=(XY+o)
OP(xycb,ae) { WM(m_EA, RES(5, RM(m_EA)));                  } /// RES  5,(XY+o)
OP(xycb,af) { A = RES(5, RM(m_EA)); WM(m_EA, A);           } /// RES  5,A=(XY+o)

OP(xycb,b0) { B = RES(6, RM(m_EA)); WM(m_EA, B);           } /// RES  6,B=(XY+o)
OP(xycb,b1) { C = RES(6, RM(m_EA)); WM(m_EA, C);           } /// RES  6,C=(XY+o)
OP(xycb,b2) { D = RES(6, RM(m_EA)); WM(m_EA, D);           } /// RES  6,D=(XY+o)
OP(xycb,b3) { E = RES(6, RM(m_EA)); WM(m_EA, E);           } /// RES  6,E=(XY+o)
OP(xycb,b4) { H = RES(6, RM(m_EA)); WM(m_EA, H);           } /// RES  6,H=(XY+o)
OP(xycb,b5) { L = RES(6, RM(m_EA)); WM(m_EA, L);           } /// RES  6,L=(XY+o)
OP(xycb,b6) { WM(m_EA, RES(6, RM(m_EA)));                  } /// RES  6,(XY+o)
OP(xycb,b7) { A = RES(6, RM(m_EA)); WM(m_EA, A);           } /// RES  6,A=(XY+o)

OP(xycb,b8) { B = RES(7, RM(m_EA)); WM(m_EA, B);           } /// RES  7,B=(XY+o)
OP(xycb,b9) { C = RES(7, RM(m_EA)); WM(m_EA, C);           } /// RES  7,C=(XY+o)
OP(xycb,ba) { D = RES(7, RM(m_EA)); WM(m_EA, D);           } /// RES  7,D=(XY+o)
OP(xycb,bb) { E = RES(7, RM(m_EA)); WM(m_EA, E);           } /// RES  7,E=(XY+o)
OP(xycb,bc) { H = RES(7, RM(m_EA)); WM(m_EA, H);           } /// RES  7,H=(XY+o)
OP(xycb,bd) { L = RES(7, RM(m_EA)); WM(m_EA, L);           } /// RES  7,L=(XY+o)
OP(xycb,be) { WM(m_EA, RES(7, RM(m_EA)));                  } /// RES  7,(XY+o)
OP(xycb,bf) { A = RES(7, RM(m_EA)); WM(m_EA, A);           } /// RES  7,A=(XY+o)

OP(xycb,c0) { B = SET(0, RM(m_EA)); WM(m_EA, B);           } /// SET  0,B=(XY+o)
OP(xycb,c1) { C = SET(0, RM(m_EA)); WM(m_EA, C);           } /// SET  0,C=(XY+o)
OP(xycb,c2) { D = SET(0, RM(m_EA)); WM(m_EA, D);           } /// SET  0,D=(XY+o)
OP(xycb,c3) { E = SET(0, RM(m_EA)); WM(m_EA, E);           } /// SET  0,E=(XY+o)
OP(xycb,c4) { H = SET(0, RM(m_EA)); WM(m_EA, H);           } /// SET  0,H=(XY+o)
OP(xycb,c5) { L = SET(0, RM(m_EA)); WM(m_EA, L);           } /// SET  0,L=(XY+o)
OP(xycb,c6) { WM(m_EA, SET(0, RM(m_EA)));                  } /// SET  0,(XY+o)
OP(xycb,c7) { A = SET(0, RM(m_EA)); WM(m_EA, A);           } /// SET  0,A=(XY+o)

OP(xycb,c8) { B = SET(1, RM(m_EA)); WM(m_EA, B);           } /// SET  1,B=(XY+o)
OP(xycb,c9) { C = SET(1, RM(m_EA)); WM(m_EA, C);           } /// SET  1,C=(XY+o)
OP(xycb,ca) { D = SET(1, RM(m_EA)); WM(m_EA, D);           } /// SET  1,D=(XY+o)
OP(xycb,cb) { E = SET(1, RM(m_EA)); WM(m_EA, E);           } /// SET  1,E=(XY+o)
OP(xycb,cc) { H = SET(1, RM(m_EA)); WM(m_EA, H);           } /// SET  1,H=(XY+o)
OP(xycb,cd) { L = SET(1, RM(m_EA)); WM(m_EA, L);           } /// SET  1,L=(XY+o)
OP(xycb,ce) { WM(m_EA, SET(1, RM(m_EA)));                  } /// SET  1,(XY+o)
OP(xycb,cf) { A = SET(1, RM(m_EA)); WM(m_EA, A);           } /// SET  1,A=(XY+o)

OP(xycb,d0) { B = SET(2, RM(m_EA)); WM(m_EA, B);           } /// SET  2,B=(XY+o)
OP(xycb,d1) { C = SET(2, RM(m_EA)); WM(m_EA, C);           } /// SET  2,C=(XY+o)
OP(xycb,d2) { D = SET(2, RM(m_EA)); WM(m_EA, D);           } /// SET  2,D=(XY+o)
OP(xycb,d3) { E = SET(2, RM(m_EA)); WM(m_EA, E);           } /// SET  2,E=(XY+o)
OP(xycb,d4) { H = SET(2, RM(m_EA)); WM(m_EA, H);           } /// SET  2,H=(XY+o)
OP(xycb,d5) { L = SET(2, RM(m_EA)); WM(m_EA, L);           } /// SET  2,L=(XY+o)
OP(xycb,d6) { WM(m_EA, SET(2, RM(m_EA)));                  } /// SET  2,(XY+o)
OP(xycb,d7) { A = SET(2, RM(m_EA)); WM(m_EA, A);           } /// SET  2,A=(XY+o)

OP(xycb,d8) { B = SET(3, RM(m_EA)); WM(m_EA, B);           } /// SET  3,B=(XY+o)
OP(xycb,d9) { C = SET(3, RM(m_EA)); WM(m_EA, C);           } /// SET  3,C=(XY+o)
OP(xycb,da) { D = SET(3, RM(m_EA)); WM(m_EA, D);           } /// SET  3,D=(XY+o)
OP(xycb,db) { E = SET(3, RM(m_EA)); WM(m_EA, E);           } /// SET  3,E=(XY+o)
OP(xycb,dc) { H = SET(3, RM(m_EA)); WM(m_EA, H);           } /// SET  3,H=(XY+o)
OP(xycb,dd) { L = SET(3, RM(m_EA)); WM(m_EA, L);           } /// SET  3,L=(XY+o)
OP(xycb,de) { WM(m_EA, SET(3, RM(m_EA)));                  } /// SET  3,(XY+o)
OP(xycb,df) { A = SET(3, RM(m_EA)); WM(m_EA, A);           } /// SET  3,A=(XY+o)

OP(xycb,e0) { B = SET(4, RM(m_EA)); WM(m_EA, B);           } /// SET  4,B=(XY+o)
OP(xycb,e1) { C = SET(4, RM(m_EA)); WM(m_EA, C);           } /// SET  4,C=(XY+o)
OP(xycb,e2) { D = SET(4, RM(m_EA)); WM(m_EA, D);           } /// SET  4,D=(XY+o)
OP(xycb,e3) { E = SET(4, RM(m_EA)); WM(m_EA, E);           } /// SET  4,E=(XY+o)
OP(xycb,e4) { H = SET(4, RM(m_EA)); WM(m_EA, H);           } /// SET  4,H=(XY+o)
OP(xycb,e5) { L = SET(4, RM(m_EA)); WM(m_EA, L);           } /// SET  4,L=(XY+o)
OP(xycb,e6) { WM(m_EA, SET(4, RM(m_EA)));                  } /// SET  4,(XY+o)
OP(xycb,e7) { A = SET(4, RM(m_EA)); WM(m_EA, A);           } /// SET  4,A=(XY+o)

OP(xycb,e8) { B = SET(5, RM(m_EA)); WM(m_EA, B);           } /// SET  5,B=(XY+o)
OP(xycb,e9) { C = SET(5, RM(m_EA)); WM(m_EA, C);           } /// SET  5,C=(XY+o)
OP(xycb,ea) { D = SET(5, RM(m_EA)); WM(m_EA, D);           } /// SET  5,D=(XY+o)
OP(xycb,eb) { E = SET(5, RM(m_EA)); WM(m_EA, E);           } /// SET  5,E=(XY+o)
OP(xycb,ec) { H = SET(5, RM(m_EA)); WM(m_EA, H);           } /// SET  5,H=(XY+o)
OP(xycb,ed) { L = SET(5, RM(m_EA)); WM(m_EA, L);           } /// SET  5,L=(XY+o)
OP(xycb,ee) { WM(m_EA, SET(5, RM(m_EA)));                  } /// SET  5,(XY+o)
OP(xycb,ef) { A = SET(5, RM(m_EA)); WM(m_EA, A);           } /// SET  5,A=(XY+o)

OP(xycb,f0) { B = SET(6, RM(m_EA)); WM(m_EA, B);           } /// SET  6,B=(XY+o)
OP(xycb,f1) { C = SET(6, RM(m_EA)); WM(m_EA, C);           } /// SET  6,C=(XY+o)
OP(xycb,f2) { D = SET(6, RM(m_EA)); WM(m_EA, D);           } /// SET  6,D=(XY+o)
OP(xycb,f3) { E = SET(6, RM(m_EA)); WM(m_EA, E);           } /// SET  6,E=(XY+o)
OP(xycb,f4) { H = SET(6, RM(m_EA)); WM(m_EA, H);           } /// SET  6,H=(XY+o)
OP(xycb,f5) { L = SET(6, RM(m_EA)); WM(m_EA, L);           } /// SET  6,L=(XY+o)
OP(xycb,f6) { WM(m_EA, SET(6, RM(m_EA)));                  } /// SET  6,(XY+o)
OP(xycb,f7) { A = SET(6, RM(m_EA)); WM(m_EA, A);           } /// SET  6,A=(XY+o)

OP(xycb,f8) { B = SET(7, RM(m_EA)); WM(m_EA, B);           } /// SET  7,B=(XY+o)
OP(xycb,f9) { C = SET(7, RM(m_EA)); WM(m_EA, C);           } /// SET  7,C=(XY+o)
OP(xycb,fa) { D = SET(7, RM(m_EA)); WM(m_EA, D);           } /// SET  7,D=(XY+o)
OP(xycb,fb) { E = SET(7, RM(m_EA)); WM(m_EA, E);           } /// SET  7,E=(XY+o)
OP(xycb,fc) { H = SET(7, RM(m_EA)); WM(m_EA, H);           } /// SET  7,H=(XY+o)
OP(xycb,fd) { L = SET(7, RM(m_EA)); WM(m_EA, L);           } /// SET  7,L=(XY+o)
OP(xycb,fe) { WM(m_EA, SET(7, RM(m_EA)));                  } /// SET  7,(XY+o)
OP(xycb,ff) { A = SET(7, RM(m_EA)); WM(m_EA, A);           } /// SET  7,A=(XY+o)

//------------------------------------------------------------------------------
// IX register related opcodes (DD prefix).

OP(dd,00) { Illegal1(); op_00();                              } /// DB   DD
OP(dd,01) { Illegal1(); op_01();                              } /// DB   DD
OP(dd,02) { Illegal1(); op_02();                              } /// DB   DD
OP(dd,03) { Illegal1(); op_03();                              } /// DB   DD
OP(dd,04) { Illegal1(); op_04();                              } /// DB   DD
OP(dd,05) { Illegal1(); op_05();                              } /// DB   DD
OP(dd,06) { Illegal1(); op_06();                              } /// DB   DD
OP(dd,07) { Illegal1(); op_07();                              } /// DB   DD

OP(dd,08) { Illegal1(); op_08();                              } /// DB   DD
OP(dd,09) { ADD16(&m_ix, m_bc);                               } /// ADD  IX,BC
OP(dd,0a) { Illegal1(); op_0a();                              } /// DB   DD
OP(dd,0b) { Illegal1(); op_0b();                              } /// DB   DD
OP(dd,0c) { Illegal1(); op_0c();                              } /// DB   DD
OP(dd,0d) { Illegal1(); op_0d();                              } /// DB   DD
OP(dd,0e) { Illegal1(); op_0e();                              } /// DB   DD
OP(dd,0f) { Illegal1(); op_0f();                              } /// DB   DD

OP(dd,10) { Illegal1(); op_10();                              } /// DB   DD
OP(dd,11) { Illegal1(); op_11();                              } /// DB   DD
OP(dd,12) { Illegal1(); op_12();                              } /// DB   DD
OP(dd,13) { Illegal1(); op_13();                              } /// DB   DD
OP(dd,14) { Illegal1(); op_14();                              } /// DB   DD
OP(dd,15) { Illegal1(); op_15();                              } /// DB   DD
OP(dd,16) { Illegal1(); op_16();                              } /// DB   DD
OP(dd,17) { Illegal1(); op_17();                              } /// DB   DD

OP(dd,18) { Illegal1(); op_18();                              } /// DB   DD
OP(dd,19) { ADD16(&m_ix, m_de);                               } /// ADD  IX,DE
OP(dd,1a) { Illegal1(); op_1a();                              } /// DB   DD
OP(dd,1b) { Illegal1(); op_1b();                              } /// DB   DD
OP(dd,1c) { Illegal1(); op_1c();                              } /// DB   DD
OP(dd,1d) { Illegal1(); op_1d();                              } /// DB   DD
OP(dd,1e) { Illegal1(); op_1e();                              } /// DB   DD
OP(dd,1f) { Illegal1(); op_1f();                              } /// DB   DD

OP(dd,20) { Illegal1(); op_20();                              } /// DB   DD
OP(dd,21) { IX = ARG16();                                     } /// LD   IX,w
OP(dd,22) { m_EA = ARG16(); WM16(m_EA, &m_ix); WZ = m_EA + 1; } /// LD   (w),IX
OP(dd,23) { IX++;                                             } /// INC  IX
OP(dd,24) { HX = INC(HX);                                     } /// INC  HX
OP(dd,25) { HX = DEC(HX);                                     } /// DEC  HX
OP(dd,26) { HX = ARG();                                       } /// LD   HX,n
OP(dd,27) { Illegal1(); op_27();                              } /// DB   DD

OP(dd,28) { Illegal1(); op_28();                              } /// DB   DD
OP(dd,29) { ADD16(&m_ix, m_ix);                               } /// ADD  IX,IX
OP(dd,2a) { m_EA = ARG16(); RM16(m_EA, &m_ix); WZ = m_EA + 1; } /// LD   IX,(w)
OP(dd,2b) { IX--;                                             } /// DEC  IX
OP(dd,2c) { LX = INC(LX);                                     } /// INC  LX
OP(dd,2d) { LX = DEC(LX);                                     } /// DEC  LX
OP(dd,2e) { LX = ARG();                                       } /// LD   LX,n
OP(dd,2f) { Illegal1(); op_2f();                              } /// DB   DD

OP(dd,30) { Illegal1(); op_30();                              } /// DB   DD
OP(dd,31) { Illegal1(); op_31();                              } /// DB   DD
OP(dd,32) { Illegal1(); op_32();                              } /// DB   DD
OP(dd,33) { Illegal1(); op_33();                              } /// DB   DD
OP(dd,34) { EAX(); WM(m_EA, INC(RM(m_EA)));                   } /// INC  (IX+o)
OP(dd,35) { EAX(); WM(m_EA, DEC(RM(m_EA)));                   } /// DEC  (IX+o)
OP(dd,36) { EAX(); WM(m_EA, ARG());                           } /// LD   (IX+o),n
OP(dd,37) { Illegal1(); op_37();                              } /// DB   DD

OP(dd,38) { Illegal1(); op_38();                              } /// DB   DD
OP(dd,39) { ADD16(&m_ix, m_sp);                               } /// ADD  IX,SP
OP(dd,3a) { Illegal1(); op_3a();                              } /// DB   DD
OP(dd,3b) { Illegal1(); op_3b();                              } /// DB   DD
OP(dd,3c) { Illegal1(); op_3c();                              } /// DB   DD
OP(dd,3d) { Illegal1(); op_3d();                              } /// DB   DD
OP(dd,3e) { Illegal1(); op_3e();                              } /// DB   DD
OP(dd,3f) { Illegal1(); op_3f();                              } /// DB   DD

OP(dd,40) { Illegal1(); op_40();                              } /// DB   DD
OP(dd,41) { Illegal1(); op_41();                              } /// DB   DD
OP(dd,42) { Illegal1(); op_42();                              } /// DB   DD
OP(dd,43) { Illegal1(); op_43();                              } /// DB   DD
OP(dd,44) { B = HX;                                           } /// LD   B,HX
OP(dd,45) { B = LX;                                           } /// LD   B,LX
OP(dd,46) { EAX(); B = RM(m_EA);                              } /// LD   B,(IX+o)
OP(dd,47) { Illegal1(); op_47();                              } /// DB   DD

OP(dd,48) { Illegal1(); op_48();                              } /// DB   DD
OP(dd,49) { Illegal1(); op_49();                              } /// DB   DD
OP(dd,4a) { Illegal1(); op_4a();                              } /// DB   DD
OP(dd,4b) { Illegal1(); op_4b();                              } /// DB   DD
OP(dd,4c) { C = HX;                                           } /// LD   C,HX
OP(dd,4d) { C = LX;                                           } /// LD   C,LX
OP(dd,4e) { EAX(); C = RM(m_EA);                              } /// LD   C,(IX+o)
OP(dd,4f) { Illegal1(); op_4f();                              } /// DB   DD

OP(dd,50) { Illegal1(); op_50();                              } /// DB   DD
OP(dd,51) { Illegal1(); op_51();                              } /// DB   DD
OP(dd,52) { Illegal1(); op_52();                              } /// DB   DD
OP(dd,53) { Illegal1(); op_53();                              } /// DB   DD
OP(dd,54) { D = HX;                                           } /// LD   D,HX
OP(dd,55) { D = LX;                                           } /// LD   D,LX
OP(dd,56) { EAX(); D = RM(m_EA);                              } /// LD   D,(IX+o)
OP(dd,57) { Illegal1(); op_57();                              } /// DB   DD

OP(dd,58) { Illegal1(); op_58();                              } /// DB   DD
OP(dd,59) { Illegal1(); op_59();                              } /// DB   DD
OP(dd,5a) { Illegal1(); op_5a();                              } /// DB   DD
OP(dd,5b) { Illegal1(); op_5b();                              } /// DB   DD
OP(dd,5c) { E = HX;                                           } /// LD   E,HX
OP(dd,5d) { E = LX;                                           } /// LD   E,LX
OP(dd,5e) { EAX(); E = RM(m_EA);                              } /// LD   E,(IX+o)
OP(dd,5f) { Illegal1(); op_5f();                              } /// DB   DD

OP(dd,60) { HX = B;                                           } /// LD   HX,B
OP(dd,61) { HX = C;                                           } /// LD   HX,C
OP(dd,62) { HX = D;                                           } /// LD   HX,D
OP(dd,63) { HX = E;                                           } /// LD   HX,E
OP(dd,64) {                                                   } /// LD   HX,HX
OP(dd,65) { HX = LX;                                          } /// LD   HX,LX
OP(dd,66) { EAX(); H = RM(m_EA);                              } /// LD   H,(IX+o)
OP(dd,67) { HX = A;                                           } /// LD   HX,A

OP(dd,68) { LX = B;                                           } /// LD   LX,B
OP(dd,69) { LX = C;                                           } /// LD   LX,C
OP(dd,6a) { LX = D;                                           } /// LD   LX,D
OP(dd,6b) { LX = E;                                           } /// LD   LX,E
OP(dd,6c) { LX = HX;                                          } /// LD   LX,HX
OP(dd,6d) {                                                   } /// LD   LX,LX
OP(dd,6e) { EAX(); L = RM(m_EA);                              } /// LD   L,(IX+o)
OP(dd,6f) { LX = A;                                           } /// LD   LX,A

OP(dd,70) { EAX(); WM(m_EA, B);                               } /// LD   (IX+o),B
OP(dd,71) { EAX(); WM(m_EA, C);                               } /// LD   (IX+o),C
OP(dd,72) { EAX(); WM(m_EA, D);                               } /// LD   (IX+o),D
OP(dd,73) { EAX(); WM(m_EA, E);                               } /// LD   (IX+o),E
OP(dd,74) { EAX(); WM(m_EA, H);                               } /// LD   (IX+o),H
OP(dd,75) { EAX(); WM(m_EA, L);                               } /// LD   (IX+o),L
OP(dd,76) { Illegal1(); op_76();                              } /// DB   DD
OP(dd,77) { EAX(); WM(m_EA, A);                               } /// LD   (IX+o),A

OP(dd,78) { Illegal1(); op_78();                              } /// DB   DD
OP(dd,79) { Illegal1(); op_79();                              } /// DB   DD
OP(dd,7a) { Illegal1(); op_7a();                              } /// DB   DD
OP(dd,7b) { Illegal1(); op_7b();                              } /// DB   DD
OP(dd,7c) { A = HX;                                           } /// LD   A,HX
OP(dd,7d) { A = LX;                                           } /// LD   A,LX
OP(dd,7e) { EAX(); A = RM(m_EA);                              } /// LD   A,(IX+o)
OP(dd,7f) { Illegal1(); op_7f();                              } /// DB   DD

OP(dd,80) { Illegal1(); op_80();                              } /// DB   DD
OP(dd,81) { Illegal1(); op_81();                              } /// DB   DD
OP(dd,82) { Illegal1(); op_82();                              } /// DB   DD
OP(dd,83) { Illegal1(); op_83();                              } /// DB   DD
OP(dd,84) { ADD(HX);                                          } /// ADD  A,HX
OP(dd,85) { ADD(LX);                                          } /// ADD  A,LX
OP(dd,86) { EAX(); ADD(RM(m_EA));                             } /// ADD  A,(IX+o)
OP(dd,87) { Illegal1(); op_87();                              } /// DB   DD

OP(dd,88) { Illegal1(); op_88();                              } /// DB   DD
OP(dd,89) { Illegal1(); op_89();                              } /// DB   DD
OP(dd,8a) { Illegal1(); op_8a();                              } /// DB   DD
OP(dd,8b) { Illegal1(); op_8b();                              } /// DB   DD
OP(dd,8c) { ADC(HX);                                          } /// ADC  A,HX
OP(dd,8d) { ADC(LX);                                          } /// ADC  A,LX
OP(dd,8e) { EAX(); ADC(RM(m_EA));                             } /// ADC  A,(IX+o)
OP(dd,8f) { Illegal1(); op_8f();                              } /// DB   DD

OP(dd,90) { Illegal1(); op_90();                              } /// DB   DD
OP(dd,91) { Illegal1(); op_91();                              } /// DB   DD
OP(dd,92) { Illegal1(); op_92();                              } /// DB   DD
OP(dd,93) { Illegal1(); op_93();                              } /// DB   DD
OP(dd,94) { SUB(HX);                                          } /// SUB  HX
OP(dd,95) { SUB(LX);                                          } /// SUB  LX
OP(dd,96) { EAX(); SUB(RM(m_EA));                             } /// SUB  (IX+o)
OP(dd,97) { Illegal1(); op_97();                              } /// DB   DD

OP(dd,98) { Illegal1(); op_98();                              } /// DB   DD
OP(dd,99) { Illegal1(); op_99();                              } /// DB   DD
OP(dd,9a) { Illegal1(); op_9a();                              } /// DB   DD
OP(dd,9b) { Illegal1(); op_9b();                              } /// DB   DD
OP(dd,9c) { SBC(HX);                                          } /// SBC  A,HX
OP(dd,9d) { SBC(LX);                                          } /// SBC  A,LX
OP(dd,9e) { EAX(); SBC(RM(m_EA));                             } /// SBC  A,(IX+o)
OP(dd,9f) { Illegal1(); op_9f();                              } /// DB   DD

OP(dd,a0) { Illegal1(); op_a0();                              } /// DB   DD
OP(dd,a1) { Illegal1(); op_a1();                              } /// DB   DD
OP(dd,a2) { Illegal1(); op_a2();                              } /// DB   DD
OP(dd,a3) { Illegal1(); op_a3();                              } /// DB   DD
OP(dd,a4) { AND(HX);                                          } /// AND  HX
OP(dd,a5) { AND(LX);                                          } /// AND  LX
OP(dd,a6) { EAX(); AND(RM(m_EA));                             } /// AND  (IX+o)
OP(dd,a7) { Illegal1(); op_a7();                              } /// DB   DD

OP(dd,a8) { Illegal1(); op_a8();                              } /// DB   DD
OP(dd,a9) { Illegal1(); op_a9();                              } /// DB   DD
OP(dd,aa) { Illegal1(); op_aa();                              } /// DB   DD
OP(dd,ab) { Illegal1(); op_ab();                              } /// DB   DD
OP(dd,ac) { XOR(HX);                                          } /// XOR  HX
OP(dd,ad) { XOR(LX);                                          } /// XOR  LX
OP(dd,ae) { EAX(); XOR(RM(m_EA));                             } /// XOR  (IX+o)
OP(dd,af) { Illegal1(); op_af();                              } /// DB   DD

OP(dd,b0) { Illegal1(); op_b0();                              } /// DB   DD
OP(dd,b1) { Illegal1(); op_b1();                              } /// DB   DD
OP(dd,b2) { Illegal1(); op_b2();                              } /// DB   DD
OP(dd,b3) { Illegal1(); op_b3();                              } /// DB   DD
OP(dd,b4) { OR(HX);                                           } /// OR   HX
OP(dd,b5) { OR(LX);                                           } /// OR   LX
OP(dd,b6) { EAX(); OR(RM(m_EA));                              } /// OR   (IX+o)
OP(dd,b7) { Illegal1(); op_b7();                              } /// DB   DD

OP(dd,b8) { Illegal1(); op_b8();                              } /// DB   DD
OP(dd,b9) { Illegal1(); op_b9();                              } /// DB   DD
OP(dd,ba) { Illegal1(); op_ba();                              } /// DB   DD
OP(dd,bb) { Illegal1(); op_bb();                              } /// DB   DD
OP(dd,bc) { CP(HX);                                           } /// CP   HX
OP(dd,bd) { CP(LX);                                           } /// CP   LX
OP(dd,be) { EAX(); CP(RM(m_EA));                              } /// CP   (IX+o)
OP(dd,bf) { Illegal1(); op_bf();                              } /// DB   DD

OP(dd,c0) { Illegal1(); op_c0();                              } /// DB   DD
OP(dd,c1) { Illegal1(); op_c1();                              } /// DB   DD
OP(dd,c2) { Illegal1(); op_c2();                              } /// DB   DD
OP(dd,c3) { Illegal1(); op_c3();                              } /// DB   DD
OP(dd,c4) { Illegal1(); op_c4();                              } /// DB   DD
OP(dd,c5) { Illegal1(); op_c5();                              } /// DB   DD
OP(dd,c6) { Illegal1(); op_c6();                              } /// DB   DD
OP(dd,c7) { Illegal1(); op_c7();                              } /// DB   DD

OP(dd,c8) { Illegal1(); op_c8();                              } /// DB   DD
OP(dd,c9) { Illegal1(); op_c9();                              } /// DB   DD
OP(dd,ca) { Illegal1(); op_ca();                              } /// DB   DD
OP(dd,cb) { EAX(); EXEC(xycb, ARG());                         } /// **** DD CB xx
OP(dd,cc) { Illegal1(); op_cc();                              } /// DB   DD
OP(dd,cd) { Illegal1(); op_cd();                              } /// DB   DD
OP(dd,ce) { Illegal1(); op_ce();                              } /// DB   DD
OP(dd,cf) { Illegal1(); op_cf();                              } /// DB   DD

OP(dd,d0) { Illegal1(); op_d0();                              } /// DB   DD
OP(dd,d1) { Illegal1(); op_d1();                              } /// DB   DD
OP(dd,d2) { Illegal1(); op_d2();                              } /// DB   DD
OP(dd,d3) { Illegal1(); op_d3();                              } /// DB   DD
OP(dd,d4) { Illegal1(); op_d4();                              } /// DB   DD
OP(dd,d5) { Illegal1(); op_d5();                              } /// DB   DD
OP(dd,d6) { Illegal1(); op_d6();                              } /// DB   DD
OP(dd,d7) { Illegal1(); op_d7();                              } /// DB   DD

OP(dd,d8) { Illegal1(); op_d8();                              } /// DB   DD
OP(dd,d9) { Illegal1(); op_d9();                              } /// DB   DD
OP(dd,da) { Illegal1(); op_da();                              } /// DB   DD
OP(dd,db) { Illegal1(); op_db();                              } /// DB   DD
OP(dd,dc) { Illegal1(); op_dc();                              } /// DB   DD
OP(dd,dd) { EXEC(dd, ROP());                                  } /// **** DD DD xx
OP(dd,de) { Illegal1(); op_de();                              } /// DB   DD
OP(dd,df) { Illegal1(); op_df();                              } /// DB   DD

OP(dd,e0) { Illegal1(); op_e0();                              } /// DB   DD
OP(dd,e1) { POP(&m_ix);                                       } /// POP  IX
OP(dd,e2) { Illegal1(); op_e2();                              } /// DB   DD
OP(dd,e3) { EXSP(&m_ix);                                      } /// EX   (SP),IX
OP(dd,e4) { Illegal1(); op_e4();                              } /// DB   DD
OP(dd,e5) { PUSH(&m_ix);                                      } /// PUSH IX
OP(dd,e6) { Illegal1(); op_e6();                              } /// DB   DD
OP(dd,e7) { Illegal1(); op_e7();                              } /// DB   DD

OP(dd,e8) { Illegal1(); op_e8();                              } /// DB   DD
OP(dd,e9) { PC = IX;                                          } /// JP   (IX)
OP(dd,ea) { Illegal1(); op_ea();                              } /// DB   DD
OP(dd,eb) { Illegal1(); op_eb();                              } /// DB   DD
OP(dd,ec) { Illegal1(); op_ec();                              } /// DB   DD
OP(dd,ed) { Illegal1(); op_ed();                              } /// DB   DD
OP(dd,ee) { Illegal1(); op_ee();                              } /// DB   DD
OP(dd,ef) { Illegal1(); op_ef();                              } /// DB   DD

OP(dd,f0) { Illegal1(); op_f0();                              } /// DB   DD
OP(dd,f1) { Illegal1(); op_f1();                              } /// DB   DD
OP(dd,f2) { Illegal1(); op_f2();                              } /// DB   DD
OP(dd,f3) { Illegal1(); op_f3();                              } /// DB   DD
OP(dd,f4) { Illegal1(); op_f4();                              } /// DB   DD
OP(dd,f5) { Illegal1(); op_f5();                              } /// DB   DD
OP(dd,f6) { Illegal1(); op_f6();                              } /// DB   DD
OP(dd,f7) { Illegal1(); op_f7();                              } /// DB   DD

OP(dd,f8) { Illegal1(); op_f8();                              } /// DB   DD
OP(dd,f9) { SP = IX;                                          } /// LD   SP,IX
OP(dd,fa) { Illegal1(); op_fa();                              } /// DB   DD
OP(dd,fb) { Illegal1(); op_fb();                              } /// DB   DD
OP(dd,fc) { Illegal1(); op_fc();                              } /// DB   DD
OP(dd,fd) { EXEC(fd, ROP());                                  } /// **** DD FD xx
OP(dd,fe) { Illegal1(); op_fe();                              } /// DB   DD
OP(dd,ff) { Illegal1(); op_ff();                              } /// DB   DD


//------------------------------------------------------------------------------
// IY register related opcodes (FD prefix).

OP(fd,00) { Illegal1(); op_00();                              } /// DB   FD
OP(fd,01) { Illegal1(); op_01();                              } /// DB   FD
OP(fd,02) { Illegal1(); op_02();                              } /// DB   FD
OP(fd,03) { Illegal1(); op_03();                              } /// DB   FD
OP(fd,04) { Illegal1(); op_04();                              } /// DB   FD
OP(fd,05) { Illegal1(); op_05();                              } /// DB   FD
OP(fd,06) { Illegal1(); op_06();                              } /// DB   FD
OP(fd,07) { Illegal1(); op_07();                              } /// DB   FD

OP(fd,08) { Illegal1(); op_08();                              } /// DB   FD
OP(fd,09) { ADD16(&m_iy, m_bc);                               } /// ADD  IY,BC
OP(fd,0a) { Illegal1(); op_0a();                              } /// DB   FD
OP(fd,0b) { Illegal1(); op_0b();                              } /// DB   FD
OP(fd,0c) { Illegal1(); op_0c();                              } /// DB   FD
OP(fd,0d) { Illegal1(); op_0d();                              } /// DB   FD
OP(fd,0e) { Illegal1(); op_0e();                              } /// DB   FD
OP(fd,0f) { Illegal1(); op_0f();                              } /// DB   FD

OP(fd,10) { Illegal1(); op_10();                              } /// DB   FD
OP(fd,11) { Illegal1(); op_11();                              } /// DB   FD
OP(fd,12) { Illegal1(); op_12();                              } /// DB   FD
OP(fd,13) { Illegal1(); op_13();                              } /// DB   FD
OP(fd,14) { Illegal1(); op_14();                              } /// DB   FD
OP(fd,15) { Illegal1(); op_15();                              } /// DB   FD
OP(fd,16) { Illegal1(); op_16();                              } /// DB   FD
OP(fd,17) { Illegal1(); op_17();                              } /// DB   FD

OP(fd,18) { Illegal1(); op_18();                              } /// DB   FD
OP(fd,19) { ADD16(&m_iy, m_de);                               } /// ADD  IY,DE
OP(fd,1a) { Illegal1(); op_1a();                              } /// DB   FD
OP(fd,1b) { Illegal1(); op_1b();                              } /// DB   FD
OP(fd,1c) { Illegal1(); op_1c();                              } /// DB   FD
OP(fd,1d) { Illegal1(); op_1d();                              } /// DB   FD
OP(fd,1e) { Illegal1(); op_1e();                              } /// DB   FD
OP(fd,1f) { Illegal1(); op_1f();                              } /// DB   FD

OP(fd,20) { Illegal1(); op_20();                              } /// DB   FD
OP(fd,21) { IY = ARG16();                                     } /// LD   IY,w
OP(fd,22) { m_EA = ARG16(); WM16(m_EA, &m_iy); WZ = m_EA + 1; } /// LD   (w),IY
OP(fd,23) { IY++;                                             } /// INC  IY
OP(fd,24) { HY = INC(HY);                                     } /// INC  HY
OP(fd,25) { HY = DEC(HY);                                     } /// DEC  HY
OP(fd,26) { HY = ARG();                                       } /// LD   HY,n
OP(fd,27) { Illegal1(); op_27();                              } /// DB   FD

OP(fd,28) { Illegal1(); op_28();                              } /// DB   FD
OP(fd,29) { ADD16(&m_iy, m_iy);                               } /// ADD  IY,IY
OP(fd,2a) { m_EA = ARG16(); RM16(m_EA, &m_iy); WZ = m_EA + 1; } /// LD   IY,(w)
OP(fd,2b) { IY--;                                             } /// DEC  IY
OP(fd,2c) { LY = INC(LY);                                     } /// INC  LY
OP(fd,2d) { LY = DEC(LY);                                     } /// DEC  LY
OP(fd,2e) { LY = ARG();                                       } /// LD   LY,n
OP(fd,2f) { Illegal1(); op_2f();                              } /// DB   FD

OP(fd,30) { Illegal1(); op_30();                              } /// DB   FD
OP(fd,31) { Illegal1(); op_31();                              } /// DB   FD
OP(fd,32) { Illegal1(); op_32();                              } /// DB   FD
OP(fd,33) { Illegal1(); op_33();                              } /// DB   FD
OP(fd,34) { EAY(); WM(m_EA, INC(RM(m_EA)));                   } /// INC  (IY+o)
OP(fd,35) { EAY(); WM(m_EA, DEC(RM(m_EA)));                   } /// DEC  (IY+o)
OP(fd,36) { EAY(); WM(m_EA, ARG());                           } /// LD   (IY+o),n
OP(fd,37) { Illegal1(); op_37();                              } /// DB   FD

OP(fd,38) { Illegal1(); op_38();                              } /// DB   FD
OP(fd,39) { ADD16(&m_iy, m_sp);                               } /// ADD  IY,SP
OP(fd,3a) { Illegal1(); op_3a();                              } /// DB   FD
OP(fd,3b) { Illegal1(); op_3b();                              } /// DB   FD
OP(fd,3c) { Illegal1(); op_3c();                              } /// DB   FD
OP(fd,3d) { Illegal1(); op_3d();                              } /// DB   FD
OP(fd,3e) { Illegal1(); op_3e();                              } /// DB   FD
OP(fd,3f) { Illegal1(); op_3f();                              } /// DB   FD

OP(fd,40) { Illegal1(); op_40();                              } /// DB   FD
OP(fd,41) { Illegal1(); op_41();                              } /// DB   FD
OP(fd,42) { Illegal1(); op_42();                              } /// DB   FD
OP(fd,43) { Illegal1(); op_43();                              } /// DB   FD
OP(fd,44) { B = HY;                                           } /// LD   B,HY
OP(fd,45) { B = LY;                                           } /// LD   B,LY
OP(fd,46) { EAY(); B = RM(m_EA);                              } /// LD   B,(IY+o)
OP(fd,47) { Illegal1(); op_47();                              } /// DB   FD

OP(fd,48) { Illegal1(); op_48();                              } /// DB   FD
OP(fd,49) { Illegal1(); op_49();                              } /// DB   FD
OP(fd,4a) { Illegal1(); op_4a();                              } /// DB   FD
OP(fd,4b) { Illegal1(); op_4b();                              } /// DB   FD
OP(fd,4c) { C = HY;                                           } /// LD   C,HY
OP(fd,4d) { C = LY;                                           } /// LD   C,LY
OP(fd,4e) { EAY(); C = RM(m_EA);                              } /// LD   C,(IY+o)
OP(fd,4f) { Illegal1(); op_4f();                              } /// DB   FD

OP(fd,50) { Illegal1(); op_50();                              } /// DB   FD
OP(fd,51) { Illegal1(); op_51();                              } /// DB   FD
OP(fd,52) { Illegal1(); op_52();                              } /// DB   FD
OP(fd,53) { Illegal1(); op_53();                              } /// DB   FD
OP(fd,54) { D = HY;                                           } /// LD   D,HY
OP(fd,55) { D = LY;                                           } /// LD   D,LY
OP(fd,56) { EAY(); D = RM(m_EA);                              } /// LD   D,(IY+o)
OP(fd,57) { Illegal1(); op_57();                              } /// DB   FD

OP(fd,58) { Illegal1(); op_58();                              } /// DB   FD
OP(fd,59) { Illegal1(); op_59();                              } /// DB   FD
OP(fd,5a) { Illegal1(); op_5a();                              } /// DB   FD
OP(fd,5b) { Illegal1(); op_5b();                              } /// DB   FD
OP(fd,5c) { E = HY;                                           } /// LD   E,HY
OP(fd,5d) { E = LY;                                           } /// LD   E,LY
OP(fd,5e) { EAY(); E = RM(m_EA);                              } /// LD   E,(IY+o)
OP(fd,5f) { Illegal1(); op_5f();                              } /// DB   FD

OP(fd,60) { HY = B;                                           } /// LD   HY,B
OP(fd,61) { HY = C;                                           } /// LD   HY,C
OP(fd,62) { HY = D;                                           } /// LD   HY,D
OP(fd,63) { HY = E;                                           } /// LD   HY,E
OP(fd,64) {                                                   } /// LD   HY,HY
OP(fd,65) { HY = LY;                                          } /// LD   HY,LY
OP(fd,66) { EAY(); H = RM(m_EA);                              } /// LD   H,(IY+o)
OP(fd,67) { HY = A;                                           } /// LD   HY,A

OP(fd,68) { LY = B;                                           } /// LD   LY,B
OP(fd,69) { LY = C;                                           } /// LD   LY,C
OP(fd,6a) { LY = D;                                           } /// LD   LY,D
OP(fd,6b) { LY = E;                                           } /// LD   LY,E
OP(fd,6c) { LY = HY;                                          } /// LD   LY,HY
OP(fd,6d) {                                                   } /// LD   LY,LY
OP(fd,6e) { EAY(); L = RM(m_EA);                              } /// LD   L,(IY+o)
OP(fd,6f) { LY = A;                                           } /// LD   LY,A

OP(fd,70) { EAY(); WM(m_EA, B);                               } /// LD   (IY+o),B
OP(fd,71) { EAY(); WM(m_EA, C);                               } /// LD   (IY+o),C
OP(fd,72) { EAY(); WM(m_EA, D);                               } /// LD   (IY+o),D
OP(fd,73) { EAY(); WM(m_EA, E);                               } /// LD   (IY+o),E
OP(fd,74) { EAY(); WM(m_EA, H);                               } /// LD   (IY+o),H
OP(fd,75) { EAY(); WM(m_EA, L);                               } /// LD   (IY+o),L
OP(fd,76) { Illegal1(); op_76();                              } /// DB   FD
OP(fd,77) { EAY(); WM(m_EA, A);                               } /// LD   (IY+o),A

OP(fd,78) { Illegal1(); op_78();                              } /// DB   FD
OP(fd,79) { Illegal1(); op_79();                              } /// DB   FD
OP(fd,7a) { Illegal1(); op_7a();                              } /// DB   FD
OP(fd,7b) { Illegal1(); op_7b();                              } /// DB   FD
OP(fd,7c) { A = HY;                                           } /// LD   A,HY
OP(fd,7d) { A = LY;                                           } /// LD   A,LY
OP(fd,7e) { EAY(); A = RM(m_EA);                              } /// LD   A,(IY+o)
OP(fd,7f) { Illegal1(); op_7f();                              } /// DB   FD

OP(fd,80) { Illegal1(); op_80();                              } /// DB   FD
OP(fd,81) { Illegal1(); op_81();                              } /// DB   FD
OP(fd,82) { Illegal1(); op_82();                              } /// DB   FD
OP(fd,83) { Illegal1(); op_83();                              } /// DB   FD
OP(fd,84) { ADD(HY);                                          } /// ADD  A,HY
OP(fd,85) { ADD(LY);                                          } /// ADD  A,LY
OP(fd,86) { EAY(); ADD(RM(m_EA));                             } /// ADD  A,(IY+o)
OP(fd,87) { Illegal1(); op_87();                              } /// DB   FD

OP(fd,88) { Illegal1(); op_88();                              } /// DB   FD
OP(fd,89) { Illegal1(); op_89();                              } /// DB   FD
OP(fd,8a) { Illegal1(); op_8a();                              } /// DB   FD
OP(fd,8b) { Illegal1(); op_8b();                              } /// DB   FD
OP(fd,8c) { ADC(HY);                                          } /// ADC  A,HY
OP(fd,8d) { ADC(LY);                                          } /// ADC  A,LY
OP(fd,8e) { EAY(); ADC(RM(m_EA));                             } /// ADC  A,(IY+o)
OP(fd,8f) { Illegal1(); op_8f();                              } /// DB   FD

OP(fd,90) { Illegal1(); op_90();                              } /// DB   FD
OP(fd,91) { Illegal1(); op_91();                              } /// DB   FD
OP(fd,92) { Illegal1(); op_92();                              } /// DB   FD
OP(fd,93) { Illegal1(); op_93();                              } /// DB   FD
OP(fd,94) { SUB(HY);                                          } /// SUB  HY
OP(fd,95) { SUB(LY);                                          } /// SUB  LY
OP(fd,96) { EAY(); SUB(RM(m_EA));                             } /// SUB  (IY+o)
OP(fd,97) { Illegal1(); op_97();                              } /// DB   FD

OP(fd,98) { Illegal1(); op_98();                              } /// DB   FD
OP(fd,99) { Illegal1(); op_99();                              } /// DB   FD
OP(fd,9a) { Illegal1(); op_9a();                              } /// DB   FD
OP(fd,9b) { Illegal1(); op_9b();                              } /// DB   FD
OP(fd,9c) { SBC(HY);                                          } /// SBC  A,HY
OP(fd,9d) { SBC(LY);                                          } /// SBC  A,LY
OP(fd,9e) { EAY(); SBC(RM(m_EA));                             } /// SBC  A,(IY+o)
OP(fd,9f) { Illegal1(); op_9f();                              } /// DB   FD

OP(fd,a0) { Illegal1(); op_a0();                              } /// DB   FD
OP(fd,a1) { Illegal1(); op_a1();                              } /// DB   FD
OP(fd,a2) { Illegal1(); op_a2();                              } /// DB   FD
OP(fd,a3) { Illegal1(); op_a3();                              } /// DB   FD
OP(fd,a4) { AND(HY);                                          } /// AND  HY
OP(fd,a5) { AND(LY);                                          } /// AND  LY
OP(fd,a6) { EAY(); AND(RM(m_EA));                             } /// AND  (IY+o)
OP(fd,a7) { Illegal1(); op_a7();                              } /// DB   FD

OP(fd,a8) { Illegal1(); op_a8();                              } /// DB   FD
OP(fd,a9) { Illegal1(); op_a9();                              } /// DB   FD
OP(fd,aa) { Illegal1(); op_aa();                              } /// DB   FD
OP(fd,ab) { Illegal1(); op_ab();                              } /// DB   FD
OP(fd,ac) { XOR(HY);                                          } /// XOR  HY
OP(fd,ad) { XOR(LY);                                          } /// XOR  LY
OP(fd,ae) { EAY(); XOR(RM(m_EA));                             } /// XOR  (IY+o)
OP(fd,af) { Illegal1(); op_af();                              } /// DB   FD

OP(fd,b0) { Illegal1(); op_b0();                              } /// DB   FD
OP(fd,b1) { Illegal1(); op_b1();                              } /// DB   FD
OP(fd,b2) { Illegal1(); op_b2();                              } /// DB   FD
OP(fd,b3) { Illegal1(); op_b3();                              } /// DB   FD
OP(fd,b4) { OR(HY);                                           } /// OR   HY
OP(fd,b5) { OR(LY);                                           } /// OR   LY
OP(fd,b6) { EAY(); OR(RM(m_EA));                              } /// OR   (IY+o)
OP(fd,b7) { Illegal1(); op_b7();                              } /// DB   FD

OP(fd,b8) { Illegal1(); op_b8();                              } /// DB   FD
OP(fd,b9) { Illegal1(); op_b9();                              } /// DB   FD
OP(fd,ba) { Illegal1(); op_ba();                              } /// DB   FD
OP(fd,bb) { Illegal1(); op_bb();                              } /// DB   FD
OP(fd,bc) { CP(HY);                                           } /// CP   HY
OP(fd,bd) { CP(LY);                                           } /// CP   LY
OP(fd,be) { EAY(); CP(RM(m_EA));                              } /// CP   (IY+o)
OP(fd,bf) { Illegal1(); op_bf();                              } /// DB   FD

OP(fd,c0) { Illegal1(); op_c0();                              } /// DB   FD
OP(fd,c1) { Illegal1(); op_c1();                              } /// DB   FD
OP(fd,c2) { Illegal1(); op_c2();                              } /// DB   FD
OP(fd,c3) { Illegal1(); op_c3();                              } /// DB   FD
OP(fd,c4) { Illegal1(); op_c4();                              } /// DB   FD
OP(fd,c5) { Illegal1(); op_c5();                              } /// DB   FD
OP(fd,c6) { Illegal1(); op_c6();                              } /// DB   FD
OP(fd,c7) { Illegal1(); op_c7();                              } /// DB   FD

OP(fd,c8) { Illegal1(); op_c8();                              } /// DB   FD
OP(fd,c9) { Illegal1(); op_c9();                              } /// DB   FD
OP(fd,ca) { Illegal1(); op_ca();                              } /// DB   FD
OP(fd,cb) { EAY(); EXEC(xycb, ARG());                         } /// **** FD CB xx
OP(fd,cc) { Illegal1(); op_cc();                              } /// DB   FD
OP(fd,cd) { Illegal1(); op_cd();                              } /// DB   FD
OP(fd,ce) { Illegal1(); op_ce();                              } /// DB   FD
OP(fd,cf) { Illegal1(); op_cf();                              } /// DB   FD

OP(fd,d0) { Illegal1(); op_d0();                              } /// DB   FD
OP(fd,d1) { Illegal1(); op_d1();                              } /// DB   FD
OP(fd,d2) { Illegal1(); op_d2();                              } /// DB   FD
OP(fd,d3) { Illegal1(); op_d3();                              } /// DB   FD
OP(fd,d4) { Illegal1(); op_d4();                              } /// DB   FD
OP(fd,d5) { Illegal1(); op_d5();                              } /// DB   FD
OP(fd,d6) { Illegal1(); op_d6();                              } /// DB   FD
OP(fd,d7) { Illegal1(); op_d7();                              } /// DB   FD

OP(fd,d8) { Illegal1(); op_d8();                              } /// DB   FD
OP(fd,d9) { Illegal1(); op_d9();                              } /// DB   FD
OP(fd,da) { Illegal1(); op_da();                              } /// DB   FD
OP(fd,db) { Illegal1(); op_db();                              } /// DB   FD
OP(fd,dc) { Illegal1(); op_dc();                              } /// DB   FD
OP(fd,dd) { EXEC(dd, ROP());                                  } /// **** FD DD xx
OP(fd,de) { Illegal1(); op_de();                              } /// DB   FD
OP(fd,df) { Illegal1(); op_df();                              } /// DB   FD

OP(fd,e0) { Illegal1(); op_e0();                              } /// DB   FD
OP(fd,e1) { POP(&m_iy);                                       } /// POP  IY
OP(fd,e2) { Illegal1(); op_e2();                              } /// DB   FD
OP(fd,e3) { EXSP(&m_iy);                                      } /// EX   (SP),IY
OP(fd,e4) { Illegal1(); op_e4();                              } /// DB   FD
OP(fd,e5) { PUSH(&m_iy);                                      } /// PUSH IY
OP(fd,e6) { Illegal1(); op_e6();                              } /// DB   FD
OP(fd,e7) { Illegal1(); op_e7();                              } /// DB   FD

OP(fd,e8) { Illegal1(); op_e8();                              } /// DB   FD
OP(fd,e9) { PC = IY;                                          } /// JP   (IY)
OP(fd,ea) { Illegal1(); op_ea();                              } /// DB   FD
OP(fd,eb) { Illegal1(); op_eb();                              } /// DB   FD
OP(fd,ec) { Illegal1(); op_ec();                              } /// DB   FD
OP(fd,ed) { Illegal1(); op_ed();                              } /// DB   FD
OP(fd,ee) { Illegal1(); op_ee();                              } /// DB   FD
OP(fd,ef) { Illegal1(); op_ef();                              } /// DB   FD

OP(fd,f0) { Illegal1(); op_f0();                              } /// DB   FD
OP(fd,f1) { Illegal1(); op_f1();                              } /// DB   FD
OP(fd,f2) { Illegal1(); op_f2();                              } /// DB   FD
OP(fd,f3) { Illegal1(); op_f3();                              } /// DB   FD
OP(fd,f4) { Illegal1(); op_f4();                              } /// DB   FD
OP(fd,f5) { Illegal1(); op_f5();                              } /// DB   FD
OP(fd,f6) { Illegal1(); op_f6();                              } /// DB   FD
OP(fd,f7) { Illegal1(); op_f7();                              } /// DB   FD

OP(fd,f8) { Illegal1(); op_f8();                              } /// DB   FD
OP(fd,f9) { SP = IY;                                          } /// LD   SP,IY
OP(fd,fa) { Illegal1(); op_fa();                              } /// DB   FD
OP(fd,fb) { Illegal1(); op_fb();                              } /// DB   FD
OP(fd,fc) { Illegal1(); op_fc();                              } /// DB   FD
OP(fd,fd) { EXEC(fd, ROP());                                  } /// **** FD FD xx
OP(fd,fe) { Illegal1(); op_fe();                              } /// DB   FD
OP(fd,ff) { Illegal1(); op_ff();                              } /// DB   FD

//------------------------------------------------------------------------------
// special opcodes (ED prefix).

OP(ed,00) { Illegal2();                                       } /// DB   ED
OP(ed,01) { Illegal2();                                       } /// DB   ED
OP(ed,02) { Illegal2();                                       } /// DB   ED
OP(ed,03) { Illegal2();                                       } /// DB   ED
OP(ed,04) { Illegal2();                                       } /// DB   ED
OP(ed,05) { Illegal2();                                       } /// DB   ED
OP(ed,06) { Illegal2();                                       } /// DB   ED
OP(ed,07) { Illegal2();                                       } /// DB   ED

OP(ed,08) { Illegal2();                                       } /// DB   ED
OP(ed,09) { Illegal2();                                       } /// DB   ED
OP(ed,0a) { Illegal2();                                       } /// DB   ED
OP(ed,0b) { Illegal2();                                       } /// DB   ED
OP(ed,0c) { Illegal2();                                       } /// DB   ED
OP(ed,0d) { Illegal2();                                       } /// DB   ED
OP(ed,0e) { Illegal2();                                       } /// DB   ED
OP(ed,0f) { Illegal2();                                       } /// DB   ED

OP(ed,10) { Illegal2();                                       } /// DB   ED
OP(ed,11) { Illegal2();                                       } /// DB   ED
OP(ed,12) { Illegal2();                                       } /// DB   ED
OP(ed,13) { Illegal2();                                       } /// DB   ED
OP(ed,14) { Illegal2();                                       } /// DB   ED
OP(ed,15) { Illegal2();                                       } /// DB   ED
OP(ed,16) { Illegal2();                                       } /// DB   ED
OP(ed,17) { Illegal2();                                       } /// DB   ED

OP(ed,18) { Illegal2();                                       } /// DB   ED
OP(ed,19) { Illegal2();                                       } /// DB   ED
OP(ed,1a) { Illegal2();                                       } /// DB   ED
OP(ed,1b) { Illegal2();                                       } /// DB   ED
OP(ed,1c) { Illegal2();                                       } /// DB   ED
OP(ed,1d) { Illegal2();                                       } /// DB   ED
OP(ed,1e) { Illegal2();                                       } /// DB   ED
OP(ed,1f) { Illegal2();                                       } /// DB   ED

OP(ed,20) { Illegal2();                                       } /// DB   ED
OP(ed,21) { Illegal2();                                       } /// DB   ED
OP(ed,22) { Illegal2();                                       } /// DB   ED
OP(ed,23) { Illegal2();                                       } /// DB   ED
OP(ed,24) { Illegal2();                                       } /// DB   ED
OP(ed,25) { Illegal2();                                       } /// DB   ED
OP(ed,26) { Illegal2();                                       } /// DB   ED
OP(ed,27) { Illegal2();                                       } /// DB   ED

OP(ed,28) { Illegal2();                                       } /// DB   ED
OP(ed,29) { Illegal2();                                       } /// DB   ED
OP(ed,2a) { Illegal2();                                       } /// DB   ED
OP(ed,2b) { Illegal2();                                       } /// DB   ED
OP(ed,2c) { Illegal2();                                       } /// DB   ED
OP(ed,2d) { Illegal2();                                       } /// DB   ED
OP(ed,2e) { Illegal2();                                       } /// DB   ED
OP(ed,2f) { Illegal2();                                       } /// DB   ED

OP(ed,30) { Illegal2();                                       } /// DB   ED
OP(ed,31) { Illegal2();                                       } /// DB   ED
OP(ed,32) { Illegal2();                                       } /// DB   ED
OP(ed,33) { Illegal2();                                       } /// DB   ED
OP(ed,34) { Illegal2();                                       } /// DB   ED
OP(ed,35) { Illegal2();                                       } /// DB   ED
OP(ed,36) { Illegal2();                                       } /// DB   ED
OP(ed,37) { Illegal2();                                       } /// DB   ED

OP(ed,38) { Illegal2();                                       } /// DB   ED
OP(ed,39) { Illegal2();                                       } /// DB   ED
OP(ed,3a) { Illegal2();                                       } /// DB   ED
OP(ed,3b) { Illegal2();                                       } /// DB   ED
OP(ed,3c) { Illegal2();                                       } /// DB   ED
OP(ed,3d) { Illegal2();                                       } /// DB   ED
OP(ed,3e) { Illegal2();                                       } /// DB   ED
OP(ed,3f) { Illegal2();                                       } /// DB   ED

OP(ed,40) { B = IN(BC); F = (F & CF) | m_SZP[B];              } /// IN   B,(C)
OP(ed,41) { OUT(BC, B);                                       } /// OUT  (C),B
OP(ed,42) { SBC16(m_bc);                                      } /// SBC  HL,BC
OP(ed,43) { m_EA = ARG16(); WM16(m_EA, &m_bc); WZ = m_EA + 1; } /// LD   (w),BC
OP(ed,44) { NEG();                                            } /// NEG
OP(ed,45) { RETN();                                           } /// RETN
OP(ed,46) { IM = 0;                                           } /// IM   0
OP(ed,47) { LD_I_A();                                         } /// LD   I,A

OP(ed,48) { C = IN(BC); F = (F & CF) | m_SZP[C];              } /// IN   C,(C)
OP(ed,49) { OUT(BC, C);                                       } /// OUT  (C),C
OP(ed,4a) { ADC16(m_bc);                                      } /// ADC  HL,BC
OP(ed,4b) { m_EA = ARG16(); RM16(m_EA, &m_bc); WZ = m_EA + 1; } /// LD   BC,(w)
OP(ed,4c) { NEG();                                            } /// NEG
OP(ed,4d) { RETI();                                           } /// RETI
OP(ed,4e) { IM = 0;                                           } /// IM   0
OP(ed,4f) { LD_R_A();                                         } /// LD   R,A

OP(ed,50) { D = IN(BC); F = (F & CF) | m_SZP[D];              } /// IN   D,(C)
OP(ed,51) { OUT(BC, D);                                       } /// OUT  (C),D
OP(ed,52) { SBC16(m_de);                                      } /// SBC  HL,DE
OP(ed,53) { m_EA = ARG16(); WM16(m_EA, &m_de); WZ = m_EA + 1; } /// LD   (w),DE
OP(ed,54) { NEG();                                            } /// NEG
OP(ed,55) { RETN();                                           } /// RETN
OP(ed,56) { IM = 1;                                           } /// IM   1
OP(ed,57) { LD_A_I();                                         } /// LD   A,I

OP(ed,58) { E = IN(BC); F = (F & CF) | m_SZP[E];              } /// IN   E,(C)
OP(ed,59) { OUT(BC, E);                                       } /// OUT  (C),E
OP(ed,5a) { ADC16(m_de);                                      } /// ADC  HL,DE
OP(ed,5b) { m_EA = ARG16(); RM16(m_EA, &m_de); WZ = m_EA + 1; } /// LD   DE,(w)
OP(ed,5c) { NEG();                                            } /// NEG
OP(ed,5d) { RETI();                                           } /// RETI
OP(ed,5e) { IM = 2;                                           } /// IM   2
OP(ed,5f) { LD_A_R();                                         } /// LD   A,R

OP(ed,60) { H = IN(BC); F = (F & CF) | m_SZP[H];              } /// IN   H,(C)
OP(ed,61) { OUT(BC, H);                                       } /// OUT  (C),H
OP(ed,62) { SBC16(m_hl);                                      } /// SBC  HL,HL
OP(ed,63) { m_EA = ARG16(); WM16(m_EA, &m_hl); WZ = m_EA + 1; } /// LD   (w),HL
OP(ed,64) { NEG();                                            } /// NEG
OP(ed,65) { RETN();                                           } /// RETN
OP(ed,66) { IM = 0;                                           } /// IM   0
OP(ed,67) { RRD();                                            } /// RRD  (HL)

OP(ed,68) { L = IN(BC); F = (F & CF) | m_SZP[L];              } /// IN   L,(C)
OP(ed,69) { OUT(BC, L);                                       } /// OUT  (C),L
OP(ed,6a) { ADC16(m_hl);                                      } /// ADC  HL,HL
OP(ed,6b) { m_EA = ARG16(); RM16(m_EA, &m_hl); WZ = m_EA + 1; } /// LD   HL,(w)
OP(ed,6c) { NEG();                                            } /// NEG
OP(ed,6d) { RETI();                                           } /// RETI
OP(ed,6e) { IM = 0;                                           } /// IM   0
OP(ed,6f) { RLD();                                            } /// RLD  (HL)

OP(ed,70) { u8 res = IN(BC); F = (F & CF) | m_SZP[res];       } /// IN   0,(C)
OP(ed,71) { OUT(BC, 0);                                       } /// OUT  (C),0
OP(ed,72) { SBC16(m_sp);                                      } /// SBC  HL,SP
OP(ed,73) { m_EA = ARG16(); WM16(m_EA, &m_sp); WZ = m_EA + 1; } /// LD   (w),SP
OP(ed,74) { NEG();                                            } /// NEG
OP(ed,75) { RETN();                                           } /// RETN
OP(ed,76) { IM = 1;                                           } /// IM   1
OP(ed,77) { Illegal2();                                       } /// DB   ED,77

OP(ed,78) { A = IN(BC); F = (F & CF) | m_SZP[A]; WZ = BC + 1; } /// IN   E,(C)
OP(ed,79) { OUT(BC, A); WZ = BC + 1;                          } /// OUT  (C),A
OP(ed,7a) { ADC16(m_sp);                                      } /// ADC  HL,SP
OP(ed,7b) { m_EA = ARG16(); RM16(m_EA, &m_sp); WZ = m_EA + 1; } /// LD   SP,(w)
OP(ed,7c) { NEG();                                            } /// NEG
OP(ed,7d) { RETI();                                           } /// RETI
OP(ed,7e) { IM = 2;                                           } /// IM   2
OP(ed,7f) { Illegal2();                                       } /// DB   ED,7F

OP(ed,80) { Illegal2();                                       } /// DB   ED
OP(ed,81) { Illegal2();                                       } /// DB   ED
OP(ed,82) { Illegal2();                                       } /// DB   ED
OP(ed,83) { Illegal2();                                       } /// DB   ED
OP(ed,84) { Illegal2();                                       } /// DB   ED
OP(ed,85) { Illegal2();                                       } /// DB   ED
OP(ed,86) { Illegal2();                                       } /// DB   ED
OP(ed,87) { Illegal2();                                       } /// DB   ED

OP(ed,88) { Illegal2();                                       } /// DB   ED
OP(ed,89) { Illegal2();                                       } /// DB   ED
OP(ed,8a) { Illegal2();                                       } /// DB   ED
OP(ed,8b) { Illegal2();                                       } /// DB   ED
OP(ed,8c) { Illegal2();                                       } /// DB   ED
OP(ed,8d) { Illegal2();                                       } /// DB   ED
OP(ed,8e) { Illegal2();                                       } /// DB   ED
OP(ed,8f) { Illegal2();                                       } /// DB   ED

OP(ed,90) { Illegal2();                                       } /// DB   ED
OP(ed,91) { Illegal2();                                       } /// DB   ED
OP(ed,92) { Illegal2();                                       } /// DB   ED
OP(ed,93) { Illegal2();                                       } /// DB   ED
OP(ed,94) { Illegal2();                                       } /// DB   ED
OP(ed,95) { Illegal2();                                       } /// DB   ED
OP(ed,96) { Illegal2();                                       } /// DB   ED
OP(ed,97) { Illegal2();                                       } /// DB   ED

OP(ed,98) { Illegal2();                                       } /// DB   ED
OP(ed,99) { Illegal2();                                       } /// DB   ED
OP(ed,9a) { Illegal2();                                       } /// DB   ED
OP(ed,9b) { Illegal2();                                       } /// DB   ED
OP(ed,9c) { Illegal2();                                       } /// DB   ED
OP(ed,9d) { Illegal2();                                       } /// DB   ED
OP(ed,9e) { Illegal2();                                       } /// DB   ED
OP(ed,9f) { Illegal2();                                       } /// DB   ED

OP(ed,a0) { LDI();                                            } /// LDI
OP(ed,a1) { CPI();                                            } /// CPI
OP(ed,a2) { INI();                                            } /// INI
OP(ed,a3) { OUTI();                                           } /// OUTI
OP(ed,a4) { Illegal2();                                       } /// DB   ED
OP(ed,a5) { Illegal2();                                       } /// DB   ED
OP(ed,a6) { Illegal2();                                       } /// DB   ED
OP(ed,a7) { Illegal2();                                       } /// DB   ED

OP(ed,a8) { LDD();                                            } /// LDD
OP(ed,a9) { CPD();                                            } /// CPD
OP(ed,aa) { IND();                                            } /// IND
OP(ed,ab) { OUTD();                                           } /// OUTD
OP(ed,ac) { Illegal2();                                       } /// DB   ED
OP(ed,ad) { Illegal2();                                       } /// DB   ED
OP(ed,ae) { Illegal2();                                       } /// DB   ED
OP(ed,af) { Illegal2();                                       } /// DB   ED

OP(ed,b0) { LDIR();                                           } /// LDIR
OP(ed,b1) { CPIR();                                           } /// CPIR
OP(ed,b2) { INIR();                                           } /// INIR
OP(ed,b3) { OTIR();                                           } /// OTIR
OP(ed,b4) { Illegal2();                                       } /// DB   ED
OP(ed,b5) { Illegal2();                                       } /// DB   ED
OP(ed,b6) { Illegal2();                                       } /// DB   ED
OP(ed,b7) { Illegal2();                                       } /// DB   ED

OP(ed,b8) { LDDR();                                           } /// LDDR
OP(ed,b9) { CPDR();                                           } /// CPDR
OP(ed,ba) { INDR();                                           } /// INDR
OP(ed,bb) { OTDR();                                           } /// OTDR
OP(ed,bc) { Illegal2();                                       } /// DB   ED
OP(ed,bd) { Illegal2();                                       } /// DB   ED
OP(ed,be) { Illegal2();                                       } /// DB   ED
OP(ed,bf) { Illegal2();                                       } /// DB   ED

OP(ed,c0) { Illegal2();                                       } /// DB   ED
OP(ed,c1) { Illegal2();                                       } /// DB   ED
OP(ed,c2) { Illegal2();                                       } /// DB   ED
OP(ed,c3) { Illegal2();                                       } /// DB   ED
OP(ed,c4) { Illegal2();                                       } /// DB   ED
OP(ed,c5) { Illegal2();                                       } /// DB   ED
OP(ed,c6) { Illegal2();                                       } /// DB   ED
OP(ed,c7) { Illegal2();                                       } /// DB   ED

OP(ed,c8) { Illegal2();                                       } /// DB   ED
OP(ed,c9) { Illegal2();                                       } /// DB   ED
OP(ed,ca) { Illegal2();                                       } /// DB   ED
OP(ed,cb) { Illegal2();                                       } /// DB   ED
OP(ed,cc) { Illegal2();                                       } /// DB   ED
OP(ed,cd) { Illegal2();                                       } /// DB   ED
OP(ed,ce) { Illegal2();                                       } /// DB   ED
OP(ed,cf) { Illegal2();                                       } /// DB   ED

OP(ed,d0) { Illegal2();                                       } /// DB   ED
OP(ed,d1) { Illegal2();                                       } /// DB   ED
OP(ed,d2) { Illegal2();                                       } /// DB   ED
OP(ed,d3) { Illegal2();                                       } /// DB   ED
OP(ed,d4) { Illegal2();                                       } /// DB   ED
OP(ed,d5) { Illegal2();                                       } /// DB   ED
OP(ed,d6) { Illegal2();                                       } /// DB   ED
OP(ed,d7) { Illegal2();                                       } /// DB   ED

OP(ed,d8) { Illegal2();                                       } /// DB   ED
OP(ed,d9) { Illegal2();                                       } /// DB   ED
OP(ed,da) { Illegal2();                                       } /// DB   ED
OP(ed,db) { Illegal2();                                       } /// DB   ED
OP(ed,dc) { Illegal2();                                       } /// DB   ED
OP(ed,dd) { Illegal2();                                       } /// DB   ED
OP(ed,de) { Illegal2();                                       } /// DB   ED
OP(ed,df) { Illegal2();                                       } /// DB   ED

OP(ed,e0) { Illegal2();                                       } /// DB   ED
OP(ed,e1) { Illegal2();                                       } /// DB   ED
OP(ed,e2) { Illegal2();                                       } /// DB   ED
OP(ed,e3) { Illegal2();                                       } /// DB   ED
OP(ed,e4) { Illegal2();                                       } /// DB   ED
OP(ed,e5) { Illegal2();                                       } /// DB   ED
OP(ed,e6) { Illegal2();                                       } /// DB   ED
OP(ed,e7) { Illegal2();                                       } /// DB   ED

OP(ed,e8) { Illegal2();                                       } /// DB   ED
OP(ed,e9) { Illegal2();                                       } /// DB   ED
OP(ed,ea) { Illegal2();                                       } /// DB   ED
OP(ed,eb) { Illegal2();                                       } /// DB   ED
OP(ed,ec) { Illegal2();                                       } /// DB   ED
OP(ed,ed) { Illegal2();                                       } /// DB   ED
OP(ed,ee) { Illegal2();                                       } /// DB   ED
OP(ed,ef) { Illegal2();                                       } /// DB   ED

OP(ed,f0) { Illegal2();                                       } /// DB   ED
OP(ed,f1) { Illegal2();                                       } /// DB   ED
OP(ed,f2) { Illegal2();                                       } /// DB   ED
OP(ed,f3) { Illegal2();                                       } /// DB   ED
OP(ed,f4) { Illegal2();                                       } /// DB   ED
OP(ed,f5) { Illegal2();                                       } /// DB   ED
OP(ed,f6) { Illegal2();                                       } /// DB   ED
OP(ed,f7) { Illegal2();                                       } /// DB   ED

OP(ed,f8) { Illegal2();                                       } /// DB   ED
OP(ed,f9) { Illegal2();                                       } /// DB   ED
OP(ed,fa) { Illegal2();                                       } /// DB   ED
OP(ed,fb) { Illegal2();                                       } /// DB   ED
OP(ed,fc) { Illegal2();                                       } /// DB   ED
OP(ed,fd) { Illegal2();                                       } /// DB   ED
OP(ed,fe) { Illegal2();                                       } /// DB   ED
OP(ed,ff) { Illegal2();                                       } /// DB   ED

//------------------------------------------------------------------------------
// main opcodes.

OP(op,00) {                                                                                                } /// NOP
OP(op,01) { BC = ARG16();                                                                                  } /// LD   BC,w
OP(op,02) { WM(BC, A); WZ_L = (BC + 1) & 0xFF;  WZ_H = A;                                                  } /// LD   (BC),A
OP(op,03) { BC++;                                                                                          } /// INC  BC
OP(op,04) { B = INC(B);                                                                                    } /// INC  B
OP(op,05) { B = DEC(B);                                                                                    } /// DEC  B
OP(op,06) { B = ARG();                                                                                     } /// LD   B,n
OP(op,07) { RLCA();                                                                                        } /// RLCA

OP(op,08) { EX_AF();                                                                                       } /// EX   AF,AF'
OP(op,09) { ADD16(&m_hl, m_bc);                                                                            } /// ADD  HL,BC
OP(op,0a) { A = RM(BC); WZ = BC + 1;                                                                       } /// LD   A,(BC)
OP(op,0b) { BC--;                                                                                          } /// DEC  BC
OP(op,0c) { C = INC(C);                                                                                    } /// INC  C
OP(op,0d) { C = DEC(C);                                                                                    } /// DEC  C
OP(op,0e) { C = ARG();                                                                                     } /// LD   C,n
OP(op,0f) { RRCA();                                                                                        } /// RRCA

OP(op,10) { B--; JR_COND(B, 0x10);                                                                         } /// DJNZ o
OP(op,11) { DE = ARG16();                                                                                  } /// LD   DE,w
OP(op,12) { WM(DE, A); WZ_L = (DE + 1) & 0xFF;  WZ_H = A;                                                  } /// LD   (DE),A
OP(op,13) { DE++;                                                                                          } /// INC  DE
OP(op,14) { D = INC(D);                                                                                    } /// INC  D
OP(op,15) { D = DEC(D);                                                                                    } /// DEC  D
OP(op,16) { D = ARG();                                                                                     } /// LD   D,n
OP(op,17) { RLA();                                                                                         } /// RLA

OP(op,18) { JR();                                                                                          } /// JR   o
OP(op,19) { ADD16(&m_hl, m_de);                                                                            } /// ADD  HL,DE
OP(op,1a) { A = RM(DE); WZ = DE + 1;                                                                       } /// LD   A,(DE)
OP(op,1b) { DE--;                                                                                          } /// DEC  DE
OP(op,1c) { E = INC(E);                                                                                    } /// INC  E
OP(op,1d) { E = DEC(E);                                                                                    } /// DEC  E
OP(op,1e) { E = ARG();                                                                                     } /// LD   E,n
OP(op,1f) { RRA();                                                                                         } /// RRA

OP(op,20) { JR_COND(!(F & ZF), 0x20);                                                                      } /// JR   NZ,o
OP(op,21) { HL = ARG16();                                                                                  } /// LD   HL,w
OP(op,22) { m_EA = ARG16(); WM16(m_EA, &m_hl); WZ = m_EA + 1;                                              } /// LD   (w),HL
OP(op,23) { HL++;                                                                                          } /// INC  HL
OP(op,24) { H = INC(H);                                                                                    } /// INC  H
OP(op,25) { H = DEC(H);                                                                                    } /// DEC  H
OP(op,26) { H = ARG();                                                                                     } /// LD   H,n
OP(op,27) { DAA();                                                                                         } /// DAA

OP(op,28) { JR_COND(F & ZF, 0x28);                                                                         } /// JR   Z,o
OP(op,29) { ADD16(&m_hl, m_hl);                                                                            } /// ADD  HL,HL
OP(op,2a) { m_EA = ARG16(); RM16(m_EA, &m_hl); WZ = m_EA + 1;                                              } /// LD   HL,(w)
OP(op,2b) { HL--;                                                                                          } /// DEC  HL
OP(op,2c) { L = INC(L);                                                                                    } /// INC  L
OP(op,2d) { L = DEC(L);                                                                                    } /// DEC  L
OP(op,2e) { L = ARG();                                                                                     } /// LD   L,n
OP(op,2f) { A ^= 0xff; F = (F&(SF|ZF|PF|CF))|HF|NF|(A&(YF|XF));                                            } /// CPL

OP(op,30) { JR_COND(!(F & CF), 0x30);                                                                      } /// JR   NC,o
OP(op,31) { SP = ARG16();                                                                                  } /// LD   SP,w
OP(op,32) { m_EA = ARG16(); WM(m_EA, A); WZ_L = (m_EA + 1) & 0xFF; WZ_H = A;                               } /// LD   (w),A
OP(op,33) { SP++;                                                                                          } /// INC  SP
OP(op,34) { WM(HL, INC(RM(HL)));                                                                           } /// INC  (HL)
OP(op,35) { WM(HL, DEC(RM(HL)));                                                                           } /// DEC  (HL)
OP(op,36) { WM(HL, ARG());                                                                                 } /// LD   (HL),n
OP(op,37) { F = (F & (SF|ZF|YF|XF|PF)) | CF | (A & (YF|XF));                                               } /// SCF

OP(op,38) { JR_COND(F & CF, 0x38);                                                                         } /// JR   C,o
OP(op,39) { ADD16(&m_hl, m_sp);                                                                            } /// ADD  HL,SP
OP(op,3a) { m_EA = ARG16(); A = RM(m_EA); WZ = m_EA + 1;                                                   } /// LD   A,(w)
OP(op,3b) { SP--;                                                                                          } /// DEC  SP
OP(op,3c) { A = INC(A);                                                                                    } /// INC  A
OP(op,3d) { A = DEC(A);                                                                                    } /// DEC  A
OP(op,3e) { A = ARG();                                                                                     } /// LD   A,n
OP(op,3f) { F = ((F&(SF|ZF|YF|XF|PF|CF)) | ((F&CF)<<4) | (A&(YF|XF))) ^ CF;                                } /// CCF

OP(op,40) {                                                                                                } /// LD   B,B
OP(op,41) { B = C;                                                                                         } /// LD   B,C
OP(op,42) { B = D;                                                                                         } /// LD   B,D
OP(op,43) { B = E;                                                                                         } /// LD   B,E
OP(op,44) { B = H;                                                                                         } /// LD   B,H
OP(op,45) { B = L;                                                                                         } /// LD   B,L
OP(op,46) { B = RM(HL);                                                                                    } /// LD   B,(HL)
OP(op,47) { B = A;                                                                                         } /// LD   B,A

OP(op,48) { C = B;                                                                                         } /// LD   C,B
OP(op,49) {                                                                                                } /// LD   C,C
OP(op,4a) { C = D;                                                                                         } /// LD   C,D
OP(op,4b) { C = E;                                                                                         } /// LD   C,E
OP(op,4c) { C = H;                                                                                         } /// LD   C,H
OP(op,4d) { C = L;                                                                                         } /// LD   C,L
OP(op,4e) { C = RM(HL);                                                                                    } /// LD   C,(HL)
OP(op,4f) { C = A;                                                                                         } /// LD   C,A

OP(op,50) { D = B;                                                                                         } /// LD   D,B
OP(op,51) { D = C;                                                                                         } /// LD   D,C
OP(op,52) {                                                                                                } /// LD   D,D
OP(op,53) { D = E;                                                                                         } /// LD   D,E
OP(op,54) { D = H;                                                                                         } /// LD   D,H
OP(op,55) { D = L;                                                                                         } /// LD   D,L
OP(op,56) { D = RM(HL);                                                                                    } /// LD   D,(HL)
OP(op,57) { D = A;                                                                                         } /// LD   D,A

OP(op,58) { E = B;                                                                                         } /// LD   E,B
OP(op,59) { E = C;                                                                                         } /// LD   E,C
OP(op,5a) { E = D;                                                                                         } /// LD   E,D
OP(op,5b) {                                                                                                } /// LD   E,E
OP(op,5c) { E = H;                                                                                         } /// LD   E,H
OP(op,5d) { E = L;                                                                                         } /// LD   E,L
OP(op,5e) { E = RM(HL);                                                                                    } /// LD   E,(HL)
OP(op,5f) { E = A;                                                                                         } /// LD   E,A

OP(op,60) { H = B;                                                                                         } /// LD   H,B
OP(op,61) { H = C;                                                                                         } /// LD   H,C
OP(op,62) { H = D;                                                                                         } /// LD   H,D
OP(op,63) { H = E;                                                                                         } /// LD   H,E
OP(op,64) {                                                                                                } /// LD   H,H
OP(op,65) { H = L;                                                                                         } /// LD   H,L
OP(op,66) { H = RM(HL);                                                                                    } /// LD   H,(HL)
OP(op,67) { H = A;                                                                                         } /// LD   H,A

OP(op,68) { L = B;                                                                                         } /// LD   L,B
OP(op,69) { L = C;                                                                                         } /// LD   L,C
OP(op,6a) { L = D;                                                                                         } /// LD   L,D
OP(op,6b) { L = E;                                                                                         } /// LD   L,E
OP(op,6c) { L = H;                                                                                         } /// LD   L,H
OP(op,6d) {                                                                                                } /// LD   L,L
OP(op,6e) { L = RM(HL);                                                                                    } /// LD   L,(HL)
OP(op,6f) { L = A;                                                                                         } /// LD   L,A

OP(op,70) { WM(HL, B);                                                                                     } /// LD   (HL),B
OP(op,71) { WM(HL, C);                                                                                     } /// LD   (HL),C
OP(op,72) { WM(HL, D);                                                                                     } /// LD   (HL),D
OP(op,73) { WM(HL, E);                                                                                     } /// LD   (HL),E
OP(op,74) { WM(HL, H);                                                                                     } /// LD   (HL),H
OP(op,75) { WM(HL, L);                                                                                     } /// LD   (HL),L
OP(op,76) { EnterHalt();                                                                                   } /// HALT
OP(op,77) { WM(HL, A);                                                                                     } /// LD   (HL),A

OP(op,78) { A = B;                                                                                         } /// LD   A,B
OP(op,79) { A = C;                                                                                         } /// LD   A,C
OP(op,7a) { A = D;                                                                                         } /// LD   A,D
OP(op,7b) { A = E;                                                                                         } /// LD   A,E
OP(op,7c) { A = H;                                                                                         } /// LD   A,H
OP(op,7d) { A = L;                                                                                         } /// LD   A,L
OP(op,7e) { A = RM(HL);                                                                                    } /// LD   A,(HL)
OP(op,7f) {                                                                                                } /// LD   A,A

OP(op,80) { ADD(B);                                                                                        } /// ADD  A,B
OP(op,81) { ADD(C);                                                                                        } /// ADD  A,C
OP(op,82) { ADD(D);                                                                                        } /// ADD  A,D
OP(op,83) { ADD(E);                                                                                        } /// ADD  A,E
OP(op,84) { ADD(H);                                                                                        } /// ADD  A,H
OP(op,85) { ADD(L);                                                                                        } /// ADD  A,L
OP(op,86) { ADD(RM(HL));                                                                                   } /// ADD  A,(HL)
OP(op,87) { ADD(A);                                                                                        } /// ADD  A,A

OP(op,88) { ADC(B);                                                                                        } /// ADC  A,B
OP(op,89) { ADC(C);                                                                                        } /// ADC  A,C
OP(op,8a) { ADC(D);                                                                                        } /// ADC  A,D
OP(op,8b) { ADC(E);                                                                                        } /// ADC  A,E
OP(op,8c) { ADC(H);                                                                                        } /// ADC  A,H
OP(op,8d) { ADC(L);                                                                                        } /// ADC  A,L
OP(op,8e) { ADC(RM(HL));                                                                                   } /// ADC  A,(HL)
OP(op,8f) { ADC(A);                                                                                        } /// ADC  A,A

OP(op,90) { SUB(B);                                                                                        } /// SUB  B
OP(op,91) { SUB(C);                                                                                        } /// SUB  C
OP(op,92) { SUB(D);                                                                                        } /// SUB  D
OP(op,93) { SUB(E);                                                                                        } /// SUB  E
OP(op,94) { SUB(H);                                                                                        } /// SUB  H
OP(op,95) { SUB(L);                                                                                        } /// SUB  L
OP(op,96) { SUB(RM(HL));                                                                                   } /// SUB  (HL)
OP(op,97) { SUB(A);                                                                                        } /// SUB  A

OP(op,98) { SBC(B);                                                                                        } /// SBC  A,B
OP(op,99) { SBC(C);                                                                                        } /// SBC  A,C
OP(op,9a) { SBC(D);                                                                                        } /// SBC  A,D
OP(op,9b) { SBC(E);                                                                                        } /// SBC  A,E
OP(op,9c) { SBC(H);                                                                                        } /// SBC  A,H
OP(op,9d) { SBC(L);                                                                                        } /// SBC  A,L
OP(op,9e) { SBC(RM(HL));                                                                                   } /// SBC  A,(HL)
OP(op,9f) { SBC(A);                                                                                        } /// SBC  A,A

OP(op,a0) { AND(B);                                                                                        } /// AND  B
OP(op,a1) { AND(C);                                                                                        } /// AND  C
OP(op,a2) { AND(D);                                                                                        } /// AND  D
OP(op,a3) { AND(E);                                                                                        } /// AND  E
OP(op,a4) { AND(H);                                                                                        } /// AND  H
OP(op,a5) { AND(L);                                                                                        } /// AND  L
OP(op,a6) { AND(RM(HL));                                                                                   } /// AND  (HL)
OP(op,a7) { AND(A);                                                                                        } /// AND  A

OP(op,a8) { XOR(B);                                                                                        } /// XOR  B
OP(op,a9) { XOR(C);                                                                                        } /// XOR  C
OP(op,aa) { XOR(D);                                                                                        } /// XOR  D
OP(op,ab) { XOR(E);                                                                                        } /// XOR  E
OP(op,ac) { XOR(H);                                                                                        } /// XOR  H
OP(op,ad) { XOR(L);                                                                                        } /// XOR  L
OP(op,ae) { XOR(RM(HL));                                                                                   } /// XOR  (HL)
OP(op,af) { XOR(A);                                                                                        } /// XOR  A

OP(op,b0) { OR(B);                                                                                         } /// OR   B
OP(op,b1) { OR(C);                                                                                         } /// OR   C
OP(op,b2) { OR(D);                                                                                         } /// OR   D
OP(op,b3) { OR(E);                                                                                         } /// OR   E
OP(op,b4) { OR(H);                                                                                         } /// OR   H
OP(op,b5) { OR(L);                                                                                         } /// OR   L
OP(op,b6) { OR(RM(HL));                                                                                    } /// OR   (HL)
OP(op,b7) { OR(A);                                                                                         } /// OR   A

OP(op,b8) { CP(B);                                                                                         } /// CP   B
OP(op,b9) { CP(C);                                                                                         } /// CP   C
OP(op,ba) { CP(D);                                                                                         } /// CP   D
OP(op,bb) { CP(E);                                                                                         } /// CP   E
OP(op,bc) { CP(H);                                                                                         } /// CP   H
OP(op,bd) { CP(L);                                                                                         } /// CP   L
OP(op,be) { CP(RM(HL));                                                                                    } /// CP   (HL)
OP(op,bf) { CP(A);                                                                                         } /// CP   A

OP(op,c0) { RET_COND(!(F & ZF), 0xc0);                                                                     } /// RET  NZ
OP(op,c1) { POP(&m_bc);                                                                                    } /// POP  BC
OP(op,c2) { JP_COND(!(F & ZF));                                                                            } /// JP   NZ,a
OP(op,c3) { JP();                                                                                          } /// JP   a
OP(op,c4) { CALL_COND(!(F & ZF), 0xc4);                                                                    } /// CALL NZ,a
OP(op,c5) { PUSH(&m_bc);                                                                                   } /// PUSH BC
OP(op,c6) { ADD(ARG());                                                                                    } /// ADD  A,n
OP(op,c7) { RST(0x00);                                                                                     } /// RST  0

OP(op,c8) { RET_COND(F & ZF, 0xc8);                                                                        } /// RET  Z
OP(op,c9) { POP(&m_pc); WZ = PCD;                                                                          } /// RET
OP(op,ca) { JP_COND(F & ZF);                                                                               } /// JP   Z,a
OP(op,cb) { R++; EXEC(cb, ROP());                                                                          } /// **** CB xx
OP(op,cc) { CALL_COND(F & ZF, 0xcc);                                                                       } /// CALL Z,a
OP(op,cd) { CALL();                                                                                        } /// CALL a
OP(op,ce) { ADC(ARG());                                                                                    } /// ADC  A,n
OP(op,cf) { RST(0x08);                                                                                     } /// RST  1

OP(op,d0) { RET_COND(!(F & CF), 0xd0);                                                                     } /// RET  NC
OP(op,d1) { POP(&m_de);                                                                                    } /// POP  DE
OP(op,d2) { JP_COND(!(F & CF));                                                                            } /// JP   NC,a
OP(op,d3) { u32 n = ARG() | (A << 8); OUT(n, A); WZ_L = ((n & 0xff) + 1) & 0xff; WZ_H = A;                 } /// OUT  (n),A
OP(op,d4) { CALL_COND(!(F & CF), 0xd4);                                                                    } /// CALL NC,a
OP(op,d5) { PUSH(&m_de);                                                                                   } /// PUSH DE
OP(op,d6) { SUB(ARG());                                                                                    } /// SUB  n
OP(op,d7) { RST(0x10);                                                                                     } /// RST  2

OP(op,d8) { RET_COND(F & CF, 0xd8);                                                                        } /// RET  C
OP(op,d9) { EXX();                                                                                         } /// EXX
OP(op,da) { JP_COND(F & CF);                                                                               } /// JP   C,a
OP(op,db) { u32 n = ARG() | (A << 8); A = IN(n); WZ = n + 1;                                               } /// IN   A,(n)
OP(op,dc) { CALL_COND(F & CF, 0xdc);                                                                       } /// CALL C,a
OP(op,dd) { R++; EXEC(dd, ROP());                                                                          } /// **** DD xx
OP(op,de) { SBC(ARG());                                                                                    } /// SBC  A,n
OP(op,df) { RST(0x18);                                                                                     } /// RST  3

OP(op,e0) { RET_COND(!(F & PF), 0xe0);                                                                     } /// RET  PO
OP(op,e1) { POP(&m_hl);                                                                                    } /// POP  HL
OP(op,e2) { JP_COND(!(F & PF));                                                                            } /// JP   PO,a
OP(op,e3) { EXSP(&m_hl);                                                                                   } /// EX   HL,(SP)
OP(op,e4) { CALL_COND(!(F & PF), 0xe4);                                                                    } /// CALL PO,a
OP(op,e5) { PUSH(&m_hl);                                                                                   } /// PUSH HL
OP(op,e6) { AND(ARG());                                                                                    } /// AND  n
OP(op,e7) { RST(0x20);                                                                                     } /// RST  4

OP(op,e8) { RET_COND(F & PF, 0xe8);                                                                        } /// RET  PE
OP(op,e9) { PC = HL;                                                                                       } /// JP   (HL)
OP(op,ea) { JP_COND(F & PF);                                                                               } /// JP   PE,a
OP(op,eb) { EX_DE_HL();                                                                                    } /// EX   DE,HL
OP(op,ec) { CALL_COND(F & PF, 0xec);                                                                       } /// CALL PE,a
OP(op,ed) { R++; EXEC(ed, ROP());                                                                          } /// **** ED xx
OP(op,ee) { XOR(ARG());                                                                                    } /// XOR  n
OP(op,ef) { RST(0x28);                                                                                     } /// RST  5

OP(op,f0) { RET_COND(!(F & SF), 0xf0);                                                                     } /// RET  P
OP(op,f1) { POP(&m_af);                                                                                    } /// POP  AF
OP(op,f2) { JP_COND(!(F & SF));                                                                            } /// JP   P,a
OP(op,f3) { IFF1 = IFF2 = 0;                                                                               } /// DI
OP(op,f4) { CALL_COND(!(F & SF), 0xf4);                                                                    } /// CALL P,a
OP(op,f5) { PUSH(&m_af);                                                                                   } /// PUSH AF
OP(op,f6) { OR(ARG());                                                                                     } /// OR   n
OP(op,f7) { RST(0x30);                                                                                     } /// RST  6

OP(op,f8) { RET_COND(F & SF, 0xf8);                                                                        } /// RET  M
OP(op,f9) { SP = HL;                                                                                       } /// LD   SP,HL
OP(op,fa) { JP_COND(F & SF);                                                                               } /// JP   M,a
OP(op,fb) { EI();                                                                                          } /// EI
OP(op,fc) { CALL_COND(F & SF, 0xfc);                                                                       } /// CALL M,a
OP(op,fd) { R++; EXEC(fd, ROP());                                                                          } /// **** FD xx
OP(op,fe) { CP(ARG());                                                                                     } /// CP   n
OP(op,ff) { RST(0x38);                                                                                     } /// RST  7

} // namespace gpgx::cpu::z80


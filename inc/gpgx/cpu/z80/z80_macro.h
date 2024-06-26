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

#ifndef __GPGX_CPU_Z80_Z80_MACRO_H__
#define __GPGX_CPU_Z80_Z80_MACRO_H__

namespace gpgx::cpu::z80 {

//==============================================================================

//------------------------------------------------------------------------------

#define CF  0x01
#define NF  0x02
#define PF  0x04
#define VF  PF
#define XF  0x08
#define HF  0x10
#define YF  0x20
#define ZF  0x40
#define SF  0x80

//------------------------------------------------------------------------------

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

//------------------------------------------------------------------------------

#define PCD  m_pc.d
#define PC m_pc.w.l

#define SPD m_sp.d
#define SP m_sp.w.l

#define AFD m_af.d
#define AF m_af.w.l
#define A m_af.b.h
#define F m_af.b.l

#define BCD m_bc.d
#define BC m_bc.w.l
#define B m_bc.b.h
#define C m_bc.b.l

#define DED m_de.d
#define DE m_de.w.l
#define D m_de.b.h
#define E m_de.b.l

#define HLD m_hl.d
#define HL m_hl.w.l
#define H m_hl.b.h
#define L m_hl.b.l

#define IXD m_ix.d
#define IX m_ix.w.l
#define HX m_ix.b.h
#define LX m_ix.b.l

#define IYD m_iy.d
#define IY m_iy.w.l
#define HY m_iy.b.h
#define LY m_iy.b.l

#define WZ   m_wz.w.l
#define WZ_H m_wz.b.h
#define WZ_L m_wz.b.l

#define I m_i
#define R m_r
#define R2 m_r2
#define IM m_im
#define IFF1 m_iff1
#define IFF2 m_iff2
#define HALT m_halt

//------------------------------------------------------------------------------

#define EXEC(prefix,opcode) \
{ \
  unsigned op = opcode; \
  AddCycles(kCycles[Z80_TABLE_##prefix][op]); \
  switch(op) \
  { \
  case 0x00: prefix##_##00(); break; case 0x01: prefix##_##01(); break; case 0x02: prefix##_##02(); break; case 0x03: prefix##_##03(); break; \
  case 0x04: prefix##_##04(); break; case 0x05: prefix##_##05(); break; case 0x06: prefix##_##06(); break; case 0x07: prefix##_##07(); break; \
  case 0x08: prefix##_##08(); break; case 0x09: prefix##_##09(); break; case 0x0a: prefix##_##0a(); break; case 0x0b: prefix##_##0b(); break; \
  case 0x0c: prefix##_##0c(); break; case 0x0d: prefix##_##0d(); break; case 0x0e: prefix##_##0e(); break; case 0x0f: prefix##_##0f(); break; \
  case 0x10: prefix##_##10(); break; case 0x11: prefix##_##11(); break; case 0x12: prefix##_##12(); break; case 0x13: prefix##_##13(); break; \
  case 0x14: prefix##_##14(); break; case 0x15: prefix##_##15(); break; case 0x16: prefix##_##16(); break; case 0x17: prefix##_##17(); break; \
  case 0x18: prefix##_##18(); break; case 0x19: prefix##_##19(); break; case 0x1a: prefix##_##1a(); break; case 0x1b: prefix##_##1b(); break; \
  case 0x1c: prefix##_##1c(); break; case 0x1d: prefix##_##1d(); break; case 0x1e: prefix##_##1e(); break; case 0x1f: prefix##_##1f(); break; \
  case 0x20: prefix##_##20(); break; case 0x21: prefix##_##21(); break; case 0x22: prefix##_##22(); break; case 0x23: prefix##_##23(); break; \
  case 0x24: prefix##_##24(); break; case 0x25: prefix##_##25(); break; case 0x26: prefix##_##26(); break; case 0x27: prefix##_##27(); break; \
  case 0x28: prefix##_##28(); break; case 0x29: prefix##_##29(); break; case 0x2a: prefix##_##2a(); break; case 0x2b: prefix##_##2b(); break; \
  case 0x2c: prefix##_##2c(); break; case 0x2d: prefix##_##2d(); break; case 0x2e: prefix##_##2e(); break; case 0x2f: prefix##_##2f(); break; \
  case 0x30: prefix##_##30(); break; case 0x31: prefix##_##31(); break; case 0x32: prefix##_##32(); break; case 0x33: prefix##_##33(); break; \
  case 0x34: prefix##_##34(); break; case 0x35: prefix##_##35(); break; case 0x36: prefix##_##36(); break; case 0x37: prefix##_##37(); break; \
  case 0x38: prefix##_##38(); break; case 0x39: prefix##_##39(); break; case 0x3a: prefix##_##3a(); break; case 0x3b: prefix##_##3b(); break; \
  case 0x3c: prefix##_##3c(); break; case 0x3d: prefix##_##3d(); break; case 0x3e: prefix##_##3e(); break; case 0x3f: prefix##_##3f(); break; \
  case 0x40: prefix##_##40(); break; case 0x41: prefix##_##41(); break; case 0x42: prefix##_##42(); break; case 0x43: prefix##_##43(); break; \
  case 0x44: prefix##_##44(); break; case 0x45: prefix##_##45(); break; case 0x46: prefix##_##46(); break; case 0x47: prefix##_##47(); break; \
  case 0x48: prefix##_##48(); break; case 0x49: prefix##_##49(); break; case 0x4a: prefix##_##4a(); break; case 0x4b: prefix##_##4b(); break; \
  case 0x4c: prefix##_##4c(); break; case 0x4d: prefix##_##4d(); break; case 0x4e: prefix##_##4e(); break; case 0x4f: prefix##_##4f(); break; \
  case 0x50: prefix##_##50(); break; case 0x51: prefix##_##51(); break; case 0x52: prefix##_##52(); break; case 0x53: prefix##_##53(); break; \
  case 0x54: prefix##_##54(); break; case 0x55: prefix##_##55(); break; case 0x56: prefix##_##56(); break; case 0x57: prefix##_##57(); break; \
  case 0x58: prefix##_##58(); break; case 0x59: prefix##_##59(); break; case 0x5a: prefix##_##5a(); break; case 0x5b: prefix##_##5b(); break; \
  case 0x5c: prefix##_##5c(); break; case 0x5d: prefix##_##5d(); break; case 0x5e: prefix##_##5e(); break; case 0x5f: prefix##_##5f(); break; \
  case 0x60: prefix##_##60(); break; case 0x61: prefix##_##61(); break; case 0x62: prefix##_##62(); break; case 0x63: prefix##_##63(); break; \
  case 0x64: prefix##_##64(); break; case 0x65: prefix##_##65(); break; case 0x66: prefix##_##66(); break; case 0x67: prefix##_##67(); break; \
  case 0x68: prefix##_##68(); break; case 0x69: prefix##_##69(); break; case 0x6a: prefix##_##6a(); break; case 0x6b: prefix##_##6b(); break; \
  case 0x6c: prefix##_##6c(); break; case 0x6d: prefix##_##6d(); break; case 0x6e: prefix##_##6e(); break; case 0x6f: prefix##_##6f(); break; \
  case 0x70: prefix##_##70(); break; case 0x71: prefix##_##71(); break; case 0x72: prefix##_##72(); break; case 0x73: prefix##_##73(); break; \
  case 0x74: prefix##_##74(); break; case 0x75: prefix##_##75(); break; case 0x76: prefix##_##76(); break; case 0x77: prefix##_##77(); break; \
  case 0x78: prefix##_##78(); break; case 0x79: prefix##_##79(); break; case 0x7a: prefix##_##7a(); break; case 0x7b: prefix##_##7b(); break; \
  case 0x7c: prefix##_##7c(); break; case 0x7d: prefix##_##7d(); break; case 0x7e: prefix##_##7e(); break; case 0x7f: prefix##_##7f(); break; \
  case 0x80: prefix##_##80(); break; case 0x81: prefix##_##81(); break; case 0x82: prefix##_##82(); break; case 0x83: prefix##_##83(); break; \
  case 0x84: prefix##_##84(); break; case 0x85: prefix##_##85(); break; case 0x86: prefix##_##86(); break; case 0x87: prefix##_##87(); break; \
  case 0x88: prefix##_##88(); break; case 0x89: prefix##_##89(); break; case 0x8a: prefix##_##8a(); break; case 0x8b: prefix##_##8b(); break; \
  case 0x8c: prefix##_##8c(); break; case 0x8d: prefix##_##8d(); break; case 0x8e: prefix##_##8e(); break; case 0x8f: prefix##_##8f(); break; \
  case 0x90: prefix##_##90(); break; case 0x91: prefix##_##91(); break; case 0x92: prefix##_##92(); break; case 0x93: prefix##_##93(); break; \
  case 0x94: prefix##_##94(); break; case 0x95: prefix##_##95(); break; case 0x96: prefix##_##96(); break; case 0x97: prefix##_##97(); break; \
  case 0x98: prefix##_##98(); break; case 0x99: prefix##_##99(); break; case 0x9a: prefix##_##9a(); break; case 0x9b: prefix##_##9b(); break; \
  case 0x9c: prefix##_##9c(); break; case 0x9d: prefix##_##9d(); break; case 0x9e: prefix##_##9e(); break; case 0x9f: prefix##_##9f(); break; \
  case 0xa0: prefix##_##a0(); break; case 0xa1: prefix##_##a1(); break; case 0xa2: prefix##_##a2(); break; case 0xa3: prefix##_##a3(); break; \
  case 0xa4: prefix##_##a4(); break; case 0xa5: prefix##_##a5(); break; case 0xa6: prefix##_##a6(); break; case 0xa7: prefix##_##a7(); break; \
  case 0xa8: prefix##_##a8(); break; case 0xa9: prefix##_##a9(); break; case 0xaa: prefix##_##aa(); break; case 0xab: prefix##_##ab(); break; \
  case 0xac: prefix##_##ac(); break; case 0xad: prefix##_##ad(); break; case 0xae: prefix##_##ae(); break; case 0xaf: prefix##_##af(); break; \
  case 0xb0: prefix##_##b0(); break; case 0xb1: prefix##_##b1(); break; case 0xb2: prefix##_##b2(); break; case 0xb3: prefix##_##b3(); break; \
  case 0xb4: prefix##_##b4(); break; case 0xb5: prefix##_##b5(); break; case 0xb6: prefix##_##b6(); break; case 0xb7: prefix##_##b7(); break; \
  case 0xb8: prefix##_##b8(); break; case 0xb9: prefix##_##b9(); break; case 0xba: prefix##_##ba(); break; case 0xbb: prefix##_##bb(); break; \
  case 0xbc: prefix##_##bc(); break; case 0xbd: prefix##_##bd(); break; case 0xbe: prefix##_##be(); break; case 0xbf: prefix##_##bf(); break; \
  case 0xc0: prefix##_##c0(); break; case 0xc1: prefix##_##c1(); break; case 0xc2: prefix##_##c2(); break; case 0xc3: prefix##_##c3(); break; \
  case 0xc4: prefix##_##c4(); break; case 0xc5: prefix##_##c5(); break; case 0xc6: prefix##_##c6(); break; case 0xc7: prefix##_##c7(); break; \
  case 0xc8: prefix##_##c8(); break; case 0xc9: prefix##_##c9(); break; case 0xca: prefix##_##ca(); break; case 0xcb: prefix##_##cb(); break; \
  case 0xcc: prefix##_##cc(); break; case 0xcd: prefix##_##cd(); break; case 0xce: prefix##_##ce(); break; case 0xcf: prefix##_##cf(); break; \
  case 0xd0: prefix##_##d0(); break; case 0xd1: prefix##_##d1(); break; case 0xd2: prefix##_##d2(); break; case 0xd3: prefix##_##d3(); break; \
  case 0xd4: prefix##_##d4(); break; case 0xd5: prefix##_##d5(); break; case 0xd6: prefix##_##d6(); break; case 0xd7: prefix##_##d7(); break; \
  case 0xd8: prefix##_##d8(); break; case 0xd9: prefix##_##d9(); break; case 0xda: prefix##_##da(); break; case 0xdb: prefix##_##db(); break; \
  case 0xdc: prefix##_##dc(); break; case 0xdd: prefix##_##dd(); break; case 0xde: prefix##_##de(); break; case 0xdf: prefix##_##df(); break; \
  case 0xe0: prefix##_##e0(); break; case 0xe1: prefix##_##e1(); break; case 0xe2: prefix##_##e2(); break; case 0xe3: prefix##_##e3(); break; \
  case 0xe4: prefix##_##e4(); break; case 0xe5: prefix##_##e5(); break; case 0xe6: prefix##_##e6(); break; case 0xe7: prefix##_##e7(); break; \
  case 0xe8: prefix##_##e8(); break; case 0xe9: prefix##_##e9(); break; case 0xea: prefix##_##ea(); break; case 0xeb: prefix##_##eb(); break; \
  case 0xec: prefix##_##ec(); break; case 0xed: prefix##_##ed(); break; case 0xee: prefix##_##ee(); break; case 0xef: prefix##_##ef(); break; \
  case 0xf0: prefix##_##f0(); break; case 0xf1: prefix##_##f1(); break; case 0xf2: prefix##_##f2(); break; case 0xf3: prefix##_##f3(); break; \
  case 0xf4: prefix##_##f4(); break; case 0xf5: prefix##_##f5(); break; case 0xf6: prefix##_##f6(); break; case 0xf7: prefix##_##f7(); break; \
  case 0xf8: prefix##_##f8(); break; case 0xf9: prefix##_##f9(); break; case 0xfa: prefix##_##fa(); break; case 0xfb: prefix##_##fb(); break; \
  case 0xfc: prefix##_##fc(); break; case 0xfd: prefix##_##fd(); break; case 0xfe: prefix##_##fe(); break; case 0xff: prefix##_##ff(); break; \
  } \
}

} // namespace gpgx::cpu::z80

#endif // #ifndef __GPGX_CPU_Z80_Z80_MACRO_H__


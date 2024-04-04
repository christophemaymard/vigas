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

void Z80::EnterHalt()
{
  PC--;
  HALT = 1;
}

//------------------------------------------------------------------------------

void Z80::LeaveHalt()
{
  if (HALT) {
    HALT = 0;
    PC++;
  }
}

//------------------------------------------------------------------------------

void Z80::Illegal1()
{
}

//------------------------------------------------------------------------------

void Z80::Illegal2()
{
}

//------------------------------------------------------------------------------

u8 Z80::INC(u8 value)
{
  u8 res = value + 1;
  F = (F & CF) | m_SZHV_inc[res];

  return (u8)res;
}

//------------------------------------------------------------------------------

u8 Z80::DEC(u8 value)
{
  u8 res = value - 1;
  F = (F & CF) | m_SZHV_dec[res];

  return res;
}

//------------------------------------------------------------------------------

void Z80::RLCA()
{
  A = (A << 1) | (A >> 7);
  F = (F & (SF | ZF | PF)) | (A & (YF | XF | CF));
}

//------------------------------------------------------------------------------

void Z80::RRCA()
{
  F = (F & (SF | ZF | PF)) | (A & CF);
  A = (A >> 1) | (A << 7);
  F |= (A & (YF | XF));
}

//------------------------------------------------------------------------------

void Z80::RLA()
{
  u8 res = (A << 1) | (F & CF);
  u8 c = (A & 0x80) ? CF : 0;
  F = (F & (SF | ZF | PF)) | c | (res & (YF | XF));
  A = res;
}

//------------------------------------------------------------------------------

void Z80::RRA()
{
  u8 res = (A >> 1) | (F << 7);
  u8 c = (A & 0x01) ? CF : 0;
  F = (F & (SF | ZF | PF)) | c | (res & (YF | XF));
  A = res;
}

//------------------------------------------------------------------------------

void Z80::RRD()
{
  u8 n = RM(HL);
  WZ = HL + 1;
  WM(HL, (n >> 4) | (A << 4));
  A = (A & 0xf0) | (n & 0x0f);
  F = (F & CF) | m_SZP[A];
}

//------------------------------------------------------------------------------

void Z80::RLD()
{
  u8 n = RM(HL);
  WZ = HL + 1;
  WM(HL, (n << 4) | (A & 0x0f));
  A = (A & 0xf0) | (n >> 4);
  F = (F & CF) | m_SZP[A];
}

//------------------------------------------------------------------------------

u8 Z80::RLC(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | (res >> 7)) & 0xff;
  F = m_SZP[res] | c;

  return res;

}

//------------------------------------------------------------------------------

u8 Z80::RRC(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (res << 7)) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::RL(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | (F & CF)) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::RR(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (F << 7)) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::SLA(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x80) ? CF : 0;
  res = (res << 1) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::SRA(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (res & 0x80)) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::SLL(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | 0x01) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

u8 Z80::SRL(u8 value)
{
  unsigned res = value;
  unsigned c = (res & 0x01) ? CF : 0;
  res = (res >> 1) & 0xff;
  F = m_SZP[res] | c;

  return res;
}

//------------------------------------------------------------------------------

void Z80::BIT(u8 bit, u8 value)
{
  F = (F & CF) | HF | (m_SZ_BIT[value & (1 << bit)] & ~(YF | XF)) | (value & (YF | XF));
}

//------------------------------------------------------------------------------

void Z80::BIT_HL(u8 bit, u8 value)
{
  F = (F & CF) | HF | (m_SZ_BIT[value & (1 << bit)] & ~(YF | XF)) | (WZ_H & (YF | XF));
}

//------------------------------------------------------------------------------

void Z80::BIT_XY(u8 bit, u8 value)
{
  F = (F & CF) | HF | (m_SZ_BIT[value & (1 << bit)] & ~(YF | XF)) | ((m_EA >> 8) & (YF | XF));
}

//------------------------------------------------------------------------------

u8 Z80::RES(u8 bit, u8 value)
{
  return value & ~(1 << bit);
}

//------------------------------------------------------------------------------

u8 Z80::SET(u8 bit, u8 value)
{
  return value | (1 << bit);
}

//------------------------------------------------------------------------------


void Z80::ADD(u8 value)
{
  u32 ah = AFD & 0xff00;
  u32 res = (u8)((ah >> 8) + value);
  F = m_SZHVC_add[ah | res];
  A = res;
}

//------------------------------------------------------------------------------

void Z80::ADC(u8 value)
{
  u32 ah = AFD & 0xff00;
  u32 c = AFD & 1;
  u32 res = (u8)((ah >> 8) + value + c);
  F = m_SZHVC_add[(c << 16) | ah | res];
  A = res;
}

//------------------------------------------------------------------------------

void Z80::SUB(u8 value)
{
  u32 ah = AFD & 0xff00;
  u32 res = (u8)((ah >> 8) - value);
  F = m_SZHVC_sub[ah | res];
  A = res;
}

//------------------------------------------------------------------------------

void Z80::SBC(u8 value)
{
  u32 ah = AFD & 0xff00;
  u32 c = AFD & 1;
  u32 res = (u8)((ah >> 8) - value - c);
  F = m_SZHVC_sub[(c << 16) | ah | res];
  A = res;
}

//------------------------------------------------------------------------------

void Z80::ADD16(Pair* dest_reg, const Pair& src_reg)
{
  u32 res = dest_reg->d + src_reg.d;
  WZ = dest_reg->d + 1;
  F = (F & (SF | ZF | VF)) |
    (((dest_reg->d ^ res ^ src_reg.d) >> 8) & HF) |
    ((res >> 16) & CF) | ((res >> 8) & (YF | XF));
  dest_reg->w.l = (u16)res;
}

//------------------------------------------------------------------------------

void Z80::ADC16(const Pair& reg)
{
  u32 res = HLD + reg.d + (F & CF);
  WZ = HL + 1;
  F = (((HLD ^ res ^ reg.d) >> 8) & HF) |
    ((res >> 16) & CF) |
    ((res >> 8) & (SF | YF | XF)) |
    ((res & 0xffff) ? 0 : ZF) |
    (((reg.d ^ HLD ^ 0x8000) & (reg.d ^ res) & 0x8000) >> 13);
  HL = (u16)res;
}

//------------------------------------------------------------------------------

void Z80::SBC16(const Pair& reg)
{
  u32 res = HLD - reg.d - (F & CF);
  WZ = HL + 1;
  F = (((HLD ^ res ^ reg.d) >> 8) & HF) | NF | 
    ((res >> 16) & CF) | 
    ((res >> 8) & (SF | YF | XF)) | 
    ((res & 0xffff) ? 0 : ZF) | 
    (((reg.d ^ HLD) & (HLD ^ res) &0x8000) >> 13);
  HL = (u16)res;
}

//------------------------------------------------------------------------------

void Z80::AND(u8 value)
{
  A &= value;
  F = m_SZP[A] | HF;
}

//------------------------------------------------------------------------------

void Z80::OR(u8 value)
{
  A |= value;
  F = m_SZP[A];
}

//------------------------------------------------------------------------------

void Z80::XOR(u8 value)
{
  A ^= value;
  F = m_SZP[A];
}

//------------------------------------------------------------------------------

void Z80::CP(u8 value)
{
  u32 val = value;
  u32 ah = AFD & 0xff00;
  u32 res = (u8)((ah >> 8) - val);
  F = (m_SZHVC_sub[ah | res] & ~(YF | XF)) | (val & (YF | XF));
}

//------------------------------------------------------------------------------

void Z80::NEG()
{
  u8 value = A;
  A = 0;
  SUB(value);
}

//------------------------------------------------------------------------------

void Z80::DAA()
{
  u8 a = A;

  if (F & NF) {
    if ((F & HF) | ((A & 0xf) > 9)) {
      a -= 6;
    }

    if ((F & CF) | (A > 0x99)) {
      a -= 0x60;
    }
  } else {
    if ((F & HF) | ((A & 0xf) > 9)) {
      a += 6;
    }

    if ((F & CF) | (A > 0x99)) {
      a += 0x60;
    }
  }

  F = (F & (CF | NF)) | (A > 0x99) | ((A ^ a) & HF) | m_SZP[a];
  A = a;
}

//------------------------------------------------------------------------------

void Z80::EX_AF()
{
  Pair tmp = m_af;
  m_af = m_af2;
  m_af2 = tmp;
}

//------------------------------------------------------------------------------

void Z80::EX_DE_HL()
{
  Pair tmp = m_de;
  m_de = m_hl;
  m_hl = tmp;
}

//------------------------------------------------------------------------------

void Z80::EXX()
{
  Pair tmp;

  tmp = m_bc;
  m_bc = m_bc2;
  m_bc2 = tmp;

  tmp = m_de;
  m_de = m_de2;
  m_de2 = tmp;

  tmp = m_hl;
  m_hl = m_hl2;
  m_hl2 = tmp;
}

//------------------------------------------------------------------------------

void Z80::EXSP(Pair* reg)
{
  Pair tmp = { { 0, 0, 0, 0 } };
  RM16(SPD, &tmp);
  WM16(SPD, reg);
  *reg = tmp;
  WZ = reg->d;
}

//------------------------------------------------------------------------------

void Z80::CALL()
{
  m_EA = ARG16();
  WZ = m_EA;
  PUSH(&m_pc);
  PCD = m_EA;
}

//------------------------------------------------------------------------------

void Z80::RETN()
{
  POP(&m_pc);
  WZ = PC;
  IFF1 = IFF2;
}

//------------------------------------------------------------------------------

void Z80::RETI()
{
  POP(&m_pc);
  WZ = PC;

  // according to http://www.msxnet.org/tech/z80-documented.pdf.
  IFF1 = IFF2;
}

//------------------------------------------------------------------------------

void Z80::CALL_COND(bool cond, u8 opcode)
{
  if (cond) {
    m_EA = ARG16();
    WZ = m_EA;
    PUSH(&m_pc);
    PCD = m_EA;

    AddCycles(kCycles[Z80_TABLE_ex][opcode]);
  } else {
    // Implicit call PC += 2;
    WZ = ARG16();
  }
}

//------------------------------------------------------------------------------

void Z80::RET_COND(bool cond, u8 opcode)
{
  if (cond) {
    POP(&m_pc);
    WZ = PC;
    AddCycles(kCycles[Z80_TABLE_ex][opcode]);
  }
}

//------------------------------------------------------------------------------

void Z80::LD_R_A()
{
  R = A;

  // keep bit 7 of R.
  R2 = A & 0x80;
}

//------------------------------------------------------------------------------

void Z80::LD_A_R()
{
  A = (R & 0x7f) | R2;
  F = (F & CF) | m_SZ[A] | (IFF2 << 2);
}

//------------------------------------------------------------------------------

void Z80::LD_I_A()
{
  I = A;
}

//------------------------------------------------------------------------------

void Z80::LD_A_I()
{
  A = I;
  F = (F & CF) | m_SZ[A] | (IFF2 << 2);
}

//------------------------------------------------------------------------------

void Z80::LDI()
{
  u8 io = RM(HL);
  WM(DE, io);
  F &= SF | ZF | CF;

  // bit 1 -> flag 5
  if ((A + io) & 0x02) {
    F |= YF;
  }

  // bit 3 -> flag 3
  if ((A + io) & 0x08) {
    F |= XF;
  }

  HL++;
  DE++;
  BC--;

  if (BC) {
    F |= VF;
  }
}

//------------------------------------------------------------------------------

void Z80::CPI()
{
  u8 val = RM(HL);
  u8 res = A - val;
  WZ++;
  HL++;
  BC--;
  F = (F & CF) | (m_SZ[res] & ~(YF | XF)) | ((A ^ val ^ res) & HF) | NF;

  if (F & HF) {
    res -= 1;
  }

  // bit 1 -> flag 5
  if (res & 0x02) {
    F |= YF;
  }

  // bit 3 -> flag 3
  if (res & 0x08) {
    F |= XF;
  }

  if (BC) {
    F |= VF;
  }
}

//------------------------------------------------------------------------------

void Z80::INI()
{
  u8 io = IN(BC);
  WZ = BC + 1;

  AddCycles(kCycles[Z80_TABLE_ex][0xa2]);

  B--;
  WM(HL, io);
  HL++;
  F = m_SZ[B];
  u32 t = (u32)((C + 1) & 0xff) + (u32)io;

  if (io & SF) {
    F |= NF;
  }

  if (t & 0x100) {
    F |= HF | CF;
  }

  F |= m_SZP[(u8)(t & 0x07) ^ B] & PF;
}

//------------------------------------------------------------------------------

void Z80::OUTI()
{
  u8 io = RM(HL);
  B--;
  WZ = BC + 1;
  OUT(BC, io);
  HL++;
  F = m_SZ[B];
  u32 t = (u32)L + (u32)io;

  if (io & SF) {
    F |= NF;
  }

  if (t & 0x100) {
    F |= HF | CF;
  }

  F |= m_SZP[(u8)(t & 0x07) ^ B] & PF;
}

//------------------------------------------------------------------------------

void Z80::LDD()
{
  u8 io = RM(HL);
  WM(DE, io);
  F &= SF | ZF | CF;

  // bit 1 -> flag 5.
  if ((A + io) & 0x02) {
    F |= YF;
  }

  // bit 3 -> flag 3
  if ((A + io) & 0x08) {
    F |= XF;
  }

  HL--;
  DE--;
  BC--;

  if (BC) {
    F |= VF;
  }
}

//------------------------------------------------------------------------------

void Z80::CPD()
{
  u8 val = RM(HL);
  u8 res = A - val;
  WZ--;
  HL--;
  BC--;
  F = (F & CF) | (m_SZ[res] & ~(YF | XF)) | ((A ^ val ^ res) & HF) | NF;

  if (F & HF) {
    res -= 1;
  }

  // bit 1 -> flag 5
  if (res & 0x02) {
    F |= YF;
  }

  // bit 3 -> flag 3
  if (res & 0x08) {
    F |= XF;
  }

  if (BC) {
    F |= VF;
  }
}

//------------------------------------------------------------------------------

void Z80::IND()
{
  u8 io = IN(BC);
  WZ = BC - 1;

  AddCycles(kCycles[Z80_TABLE_ex][0xaa]);

  B--;
  WM(HL, io);
  HL--;
  F = m_SZ[B];
  u32 t = ((u32)(C - 1) & 0xff) + (u32)io;

  if (io & SF) {
    F |= NF;
  }

  if (t & 0x100) {
    F |= HF | CF;
  }

  F |= m_SZP[(u8)(t & 0x07) ^ B] & PF;
}

//------------------------------------------------------------------------------

void Z80::OUTD()
{
  u8 io = RM(HL);
  B--;
  WZ = BC - 1;
  OUT(BC, io);
  HL--;
  F = m_SZ[B];
  u32 t = (u32)L + (u32)io;

  if (io & SF) {
    F |= NF;
  }

  if (t & 0x100) {
    F |= HF | CF;
  }

  F |= m_SZP[(u8)(t & 0x07) ^ B] & PF;
}

//------------------------------------------------------------------------------

void Z80::LDIR()
{
  LDI();

  if (BC) {
    PC -= 2;
    WZ = PC + 1;
    AddCycles(kCycles[Z80_TABLE_ex][0xb0]);
  }
}

//------------------------------------------------------------------------------

void Z80::CPIR()
{
  CPI();

  if (BC && !(F & ZF)) {
    PC -= 2;
    WZ = PC + 1;
    AddCycles(kCycles[Z80_TABLE_ex][0xb1]);
  }
}

//------------------------------------------------------------------------------

void Z80::INIR()
{
  INI();

  if (B) {
    PC -= 2;
    AddCycles(kCycles[Z80_TABLE_ex][0xb2]);
  }
}

//------------------------------------------------------------------------------

void Z80::OTIR()
{
  OUTI();

  if (B) {
    PC -= 2;
    AddCycles(kCycles[Z80_TABLE_ex][0xb3]);
  }
}

//------------------------------------------------------------------------------

void Z80::LDDR()
{
  LDD();

  if (BC) {
    PC -= 2;
    WZ = PC + 1;
    AddCycles(kCycles[Z80_TABLE_ex][0xb8]);
  }
}

//------------------------------------------------------------------------------

void Z80::CPDR()
{
  CPD();

  if (BC && !(F & ZF)) {
    PC -= 2;
    WZ = PC + 1;
    AddCycles(kCycles[Z80_TABLE_ex][0xb9]);
  }
}

//------------------------------------------------------------------------------

void Z80::INDR()
{
  IND();

  if (B) {
    PC -= 2;
    AddCycles(kCycles[Z80_TABLE_ex][0xba]);
  }
}

//------------------------------------------------------------------------------

void Z80::OTDR()
{
  OUTD();

  if (B) {
    PC -= 2;
    AddCycles(kCycles[Z80_TABLE_ex][0xbb]);
  }
}

//------------------------------------------------------------------------------

void Z80::JP()
{
  PCD = ARG16();
  WZ = PCD;
}

//------------------------------------------------------------------------------

void Z80::JP_COND(bool cond)
{
  if (cond) {
    PCD = ARG16();
    WZ = PCD;
  } else {
    // implicit do PC += 2
    WZ = ARG16();
  }
}

//------------------------------------------------------------------------------

void Z80::JR()
{
  // ARG() also increments PC
  s8 arg = (s8)ARG();

  // so don't do PC += ARG()
  PC += arg;

  WZ = PC;
}

//------------------------------------------------------------------------------

void Z80::JR_COND(bool cond, u8 opcode)
{
  if (cond) {
    JR();
    AddCycles(kCycles[Z80_TABLE_ex][opcode]);
  } else {
    PC++;
  }
}

//------------------------------------------------------------------------------

void Z80::EI()
{
  IFF1 = 1;
  IFF2 = 1;
  m_after_ei = 1;
}

//------------------------------------------------------------------------------

void Z80::RST(u32 address)
{
  PUSH(&m_pc);
  PCD = address;
  WZ = PC;
}

} // namespace gpgx::cpu::z80


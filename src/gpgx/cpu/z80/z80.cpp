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

#include "xee/mem/memory.h" // For Memset().

#include "gpgx/cpu/z80/z80_line_state.h"
#include "gpgx/cpu/z80/z80_macro.h"
#include "gpgx/cpu/z80/z80_table_index.h"

namespace gpgx::cpu::z80 {

//==============================================================================
// Z80

//------------------------------------------------------------------------------

u8 Z80::m_SZ[256];
u8 Z80::m_SZ_BIT[256];
u8 Z80::m_SZP[256];
u8 Z80::m_SZHV_inc[256];
u8 Z80::m_SZHV_dec[256];

u8 Z80::m_SZHVC_add[2 * 256 * 256];
u8 Z80::m_SZHVC_sub[2 * 256 * 256];

//------------------------------------------------------------------------------

Z80::Z80()
{
  xee::mem::Memset(m_readmap, 0, sizeof(m_readmap));
  xee::mem::Memset(m_writemap, 0, sizeof(m_writemap));

  m_writemem = nullptr;
  m_readmem = nullptr;

  m_writeport = nullptr;
  m_readport = nullptr;

  m_irq_callback = nullptr;

  m_pc.d = 0;
  m_sp.d = 0;
  m_af.d = 0;
  m_bc.d = 0;
  m_de.d = 0;
  m_hl.d = 0;
  m_ix.d = 0;
  m_iy.d = 0;
  m_wz.d = 0;

  m_af2.d = 0;
  m_bc2.d = 0;
  m_de2.d = 0;
  m_hl2.d = 0;

  m_r = 0;
  m_r2 = 0;
  m_iff1 = 0;
  m_iff2 = 0;
  m_halt = 0;
  m_im = 0;
  m_i = 0;

  m_nmi_state = LineState::kClearLine;
  m_irq_state = LineState::kClearLine;
  m_after_ei = 0;
  m_cycles = 0;

  m_last_fetch = 0;

  m_EA = 0;
}

//------------------------------------------------------------------------------

void Z80::Init(IRQCallback irq_callback)
{
  int i, p;

  int oldval, newval, val;
  u8* padd = &m_SZHVC_add[0 * 256];
  u8* padc = &m_SZHVC_add[256 * 256];
  u8* psub = &m_SZHVC_sub[0 * 256];
  u8* psbc = &m_SZHVC_sub[256 * 256];

  for (oldval = 0; oldval < 256; oldval++) {
    for (newval = 0; newval < 256; newval++) {
      // add or adc w/o carry set.
      val = newval - oldval;
      *padd = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
      *padd |= (newval & (YF | XF));  // undocumented flag bits 5+3.
      if ((newval & 0x0f) < (oldval & 0x0f)) *padd |= HF;
      if (newval < oldval) *padd |= CF;
      if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padd |= VF;
      padd++;

      // adc with carry set.
      val = newval - oldval - 1;
      *padc = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
      *padc |= (newval & (YF | XF));  // undocumented flag bits 5+3.
      if ((newval & 0x0f) <= (oldval & 0x0f)) *padc |= HF;
      if (newval <= oldval) *padc |= CF;
      if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padc |= VF;
      padc++;

      // cp, sub or sbc w/o carry set.
      val = oldval - newval;
      *psub = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
      *psub |= (newval & (YF | XF));  // undocumented flag bits 5+3.
      if ((newval & 0x0f) > (oldval & 0x0f)) *psub |= HF;
      if (newval > oldval) *psub |= CF;
      if ((val ^ oldval) & (oldval ^ newval) & 0x80) *psub |= VF;
      psub++;

      // sbc with carry set.
      val = oldval - newval - 1;
      *psbc = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
      *psbc |= (newval & (YF | XF));  // undocumented flag bits 5+3.
      if ((newval & 0x0f) >= (oldval & 0x0f)) *psbc |= HF;
      if (newval >= oldval) *psbc |= CF;
      if ((val ^ oldval) & (oldval ^ newval) & 0x80) *psbc |= VF;
      psbc++;
    }
  }

  for (i = 0; i < 256; i++) {
    p = 0;
    if (i & 0x01) ++p;
    if (i & 0x02) ++p;
    if (i & 0x04) ++p;
    if (i & 0x08) ++p;
    if (i & 0x10) ++p;
    if (i & 0x20) ++p;
    if (i & 0x40) ++p;
    if (i & 0x80) ++p;
    m_SZ[i] = i ? i & SF : ZF;
    m_SZ[i] |= (i & (YF | XF));    // undocumented flag bits 5+3.
    m_SZ_BIT[i] = i ? i & SF : ZF | PF;
    m_SZ_BIT[i] |= (i & (YF | XF));  // undocumented flag bits 5+3.
    m_SZP[i] = m_SZ[i] | ((p & 1) ? 0 : PF);
    m_SZHV_inc[i] = m_SZ[i];
    if (i == 0x80) m_SZHV_inc[i] |= VF;
    if ((i & 0x0f) == 0x00) m_SZHV_inc[i] |= HF;
    m_SZHV_dec[i] = m_SZ[i] | NF;
    if (i == 0x7f) m_SZHV_dec[i] |= VF;
    if ((i & 0x0f) == 0x0f) m_SZHV_dec[i] |= HF;
  }

  // Initialize Z80 context.

  m_pc.d = 0;
  m_sp.d = 0;
  m_af.d = 0;
  m_bc.d = 0;
  m_de.d = 0;
  m_hl.d = 0;
  m_ix.d = 0;
  m_iy.d = 0;
  m_wz.d = 0;

  m_af2.d = 0;
  m_bc2.d = 0;
  m_de2.d = 0;
  m_hl2.d = 0;

  m_r = 0;
  m_r2 = 0;
  m_iff1 = 0;
  m_iff2 = 0;
  m_halt = 0;
  m_im = 0;
  m_i = 0;

  m_nmi_state = LineState::kClearLine;
  m_irq_state = LineState::kClearLine;
  m_after_ei = 0;
  m_cycles = 0;

  m_irq_callback = irq_callback;

  // Zero flag is set.
  F = ZF;
}

//------------------------------------------------------------------------------

void Z80::Reset()
{
  PC = 0x0000;
  I = 0;
  R = 0;
  R2 = 0;
  IM = 0;
  IFF1 = 0;
  IFF2 = 0;
  HALT = 0;

  m_after_ei = 0;

  WZ = PCD;
}

//------------------------------------------------------------------------------

u16 Z80::GetPCRegister() const
{
  return PC;
}

//------------------------------------------------------------------------------

u32 Z80::GetPCDRegister() const
{
  return PCD;
}

//------------------------------------------------------------------------------

void Z80::SetHLRegister(u16 value)
{
  HL = value;
}

//------------------------------------------------------------------------------

void Z80::SetSPRegister(u16 value)
{
  SP = value;
}

//------------------------------------------------------------------------------

void Z80::SetRRegister(u8 value)
{
  m_r = value;
}

//------------------------------------------------------------------------------

void Z80::SetMemoryHandlers(ReadMemoryHandler read_handler, WriteMemoryHandler write_handler)
{
  m_readmem = read_handler;
  m_writemem = write_handler;
}

//------------------------------------------------------------------------------

void Z80::SetPortHandlers(ReadPortHandler read_handler, WritePortHandler write_handler)
{
  m_readport = read_handler;
  m_writeport = write_handler;
}

//------------------------------------------------------------------------------

void Z80::SetMemoryMapBase(s32 bank, u8* base)
{
  m_readmap[bank] = base;
  m_writemap[bank] = base;
}

//------------------------------------------------------------------------------

u8* Z80::GetReadMemoryMapBase(s32 bank)
{
  return m_readmap[bank];
}

//------------------------------------------------------------------------------

void Z80::SetReadMemoryMapBase(s32 bank, u8* base)
{
  m_readmap[bank] = base;
}

//------------------------------------------------------------------------------

void Z80::MirrorReadMemoryMapBase(s32 bank_dest, s32 bank_src)
{
  m_readmap[bank_dest] = m_readmap[bank_src];
}

//------------------------------------------------------------------------------

void Z80::SetWriteMemoryMapBase(s32 bank, u8* base)
{
  m_writemap[bank] = base;
}

//------------------------------------------------------------------------------

u8 Z80::Read8MemoryMap(u32 address) const
{
  return m_readmap[address >> 10][address & 0x03FF];
}

//------------------------------------------------------------------------------

void Z80::Write8MemoryMap(u32 address, u8 value)
{
  m_writemap[address >> 10][address & 0x03FF] = value;
}

//------------------------------------------------------------------------------

u32 Z80::GetCycles() const
{
  return m_cycles;
}

//------------------------------------------------------------------------------

void Z80::SetCycles(u32 cycles)
{
  m_cycles = cycles;
}

//------------------------------------------------------------------------------

void Z80::AddCycles(u32 cycles)
{
  m_cycles += cycles;
}

//------------------------------------------------------------------------------

void Z80::SubCycles(u32 cycles)
{
  m_cycles -= cycles;
}

//------------------------------------------------------------------------------

u8 Z80::GetLastFetch() const
{
  return m_last_fetch;
}

//------------------------------------------------------------------------------

u8 Z80::GetIRQLine() const
{
  return m_irq_state;
}

//------------------------------------------------------------------------------

void Z80::SetIRQLine(u32 state)
{
  m_irq_state = state;
}

//------------------------------------------------------------------------------

void Z80::SetIRQCallback(IRQCallback callback)
{
  m_irq_callback = callback;
}

//------------------------------------------------------------------------------

void Z80::SetNMILine(u32 state)
{
  // mark an NMI pending on the rising edge.
  if (m_nmi_state == LineState::kClearLine && state != LineState::kClearLine) {

    // Check if processor was halted.
    LeaveHalt();

    IFF1 = 0;
    PUSH(&m_pc);
    PCD = 0x0066;
    WZ = PCD;

    AddCycles(11 * 15);
  }

  m_nmi_state = state;
}

//------------------------------------------------------------------------------

void Z80::Run(u32 cycles)
{
  while (m_cycles < cycles) {
    // Check for IRQs before each instruction.
    if (m_irq_state && IFF1 && !m_after_ei) {
      ProcessInterrupt();

      if (m_cycles >= cycles) {
        return;
      }
    }

    m_after_ei = 0;
    R++;

    EXEC(op, ROP());
  }
}

//------------------------------------------------------------------------------

void Z80::ProcessInterrupt(void)
{
  // Check if processor was halted.
  LeaveHalt();

  // Clear both interrupt flip flops.
  IFF1 = 0;
  IFF2 = 0;

  // Interrupt mode 1. RST 38h.
  if (IM == 1) {
    PUSH(&m_pc);
    PCD = 0x0038;

    // RST $38 + 'interrupt latency' cycles.
    AddCycles(kCycles[Z80_TABLE_op][0xff] + kCycles[Z80_TABLE_ex][0xff]);
  } else {
    // call back the cpu interface to retrieve the vector.
    int irq_vector = (*m_irq_callback)(0);

    // Interrupt mode 2. Call [Z80.i:databyte].
    if (IM == 2) {
      irq_vector = (irq_vector & 0xff) | (I << 8);
      PUSH(&m_pc);
      RM16(irq_vector, &m_pc);

      // CALL $xxxx + 'interrupt latency' cycles.
      AddCycles(kCycles[Z80_TABLE_op][0xcd] + kCycles[Z80_TABLE_ex][0xff]);
    } else {
      // Interrupt mode 0. We check for CALL and JP instructions, 
      // if neither of these were found we assume a 1 byte opcode 
      // was placed on the databus.

      switch (irq_vector & 0xff0000) {
        case 0xcd0000:  // call
          PUSH(&m_pc);
          PCD = irq_vector & 0xffff;

          // CALL $xxxx + 'interrupt latency' cycles.
          AddCycles(kCycles[Z80_TABLE_op][0xcd] + kCycles[Z80_TABLE_ex][0xff]);

          break;
        case 0xc30000:  // jump
          PCD = irq_vector & 0xffff;

          // JP $xxxx + 2 cycles
          AddCycles(kCycles[Z80_TABLE_op][0xc3] + kCycles[Z80_TABLE_ex][0xff]);

          break;
        default:    // rst (or other opcodes?)
          PUSH(&m_pc);
          PCD = irq_vector & 0x0038;

          // RST $xx + 2 cycles
          AddCycles(kCycles[Z80_TABLE_op][0xff] + kCycles[Z80_TABLE_ex][0xff]);

          break;
      }
    }
  }

  WZ = PCD;
}

//------------------------------------------------------------------------------

u8 Z80::ROP()
{
  u32 pc = PCD;
  PC++;
  m_last_fetch = Read8MemoryMap(pc);

  return m_last_fetch;
}

//------------------------------------------------------------------------------

u8 Z80::ARG()
{
  u32 pc = PCD;
  PC++;

  return Read8MemoryMap(pc);
}

//------------------------------------------------------------------------------

u32 Z80::ARG16()
{
  u32 pc = PCD;
  PC += 2;

  return Read8MemoryMap(pc) | (Read8MemoryMap((pc + 1) & 0xffff) << 8);
}

//------------------------------------------------------------------------------

u8 Z80::IN(u32 port)
{
  return m_readport(port);
}

//------------------------------------------------------------------------------

void Z80::OUT(u32 port, u8 value)
{
  m_writeport(port, value);
}

//------------------------------------------------------------------------------

u8 Z80::RM(u32 address)
{
  return m_readmem(address);
}

//------------------------------------------------------------------------------

void Z80::WM(u32 address, u8 data)
{
  m_writemem(address, data);
}

//------------------------------------------------------------------------------

void Z80::RM16(u32 address, Pair* reg)
{
  reg->b.l = RM(address);
  reg->b.h = RM((address + 1) & 0xffff);
}

//------------------------------------------------------------------------------

void Z80::WM16(u32 address, Pair* reg)
{
  WM(address, reg->b.l);
  WM((address + 1) & 0xffff, reg->b.h);
}

//------------------------------------------------------------------------------

void Z80::PUSH(Pair* reg)
{
  SP -= 2; 
  WM16(SPD, reg);
}

//------------------------------------------------------------------------------

void Z80::POP(Pair* reg)
{
  RM16(SPD, reg); 
  SP += 2;
}

//------------------------------------------------------------------------------

void Z80::EAX()
{
  m_EA = (u32)(u16)(IX + (s8)ARG());
  WZ = m_EA;
}

//------------------------------------------------------------------------------

void Z80::EAY()
{
  m_EA = (u32)(u16)(IY + (s8)ARG());
  WZ = m_EA;
}

} // namespace gpgx::cpu::z80


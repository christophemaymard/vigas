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

#ifndef __GPGX_CPU_Z80_Z80_H__
#define __GPGX_CPU_Z80_Z80_H__

#include "xee/fnd/data_type.h"

#include "gpgx/cpu/z80/z80_register_pair.h"

namespace gpgx::cpu::z80 {

//==============================================================================

//------------------------------------------------------------------------------

class Z80
{
public:
  using WriteMemoryHandler = void (*)(u32 address, u8 data);
  using ReadMemoryHandler = u8 (*)(u32 address);

  using WritePortHandler = void (*)(u32 port, u8 data);
  using ReadPortHandler = u8 (*)(u32 port);

  using IRQCallback = int (*)(int irqline);

private:
  static const u16 kCyclesOp[0x100];
  static const u16 kCyclesCb[0x100];
  static const u16 kCyclesEd[0x100];
  static const u16 kCyclesXy[0x100]; /// Illegal combo should return 4 + cc_op[i].
  static const u16 kCyclesXycb[0x100];
  static const u16 kCyclesEx[0x100]; /// Extra cycles if jr/jp/call taken and 'interrupt latency' on rst 0-7.

  static const u16* kCycles[6];

public:
  Z80();

  void Init(IRQCallback irq_callback);
  void Reset();

  // Registers.

  u16 GetPCRegister() const;
  u32 GetPCDRegister() const;
  void SetHLRegister(u16 value);
  void SetSPRegister(u16 value);
  void SetRRegister(u8 value);

  // Memory and port handlers.

  void SetMemoryHandlers(ReadMemoryHandler read_handler, WriteMemoryHandler write_handler);
  void SetPortHandlers(ReadPortHandler read_handler, WritePortHandler write_handler);

  // Memory map.

  void SetMemoryMapBase(s32 bank, u8* base);

  u8* GetReadMemoryMapBase(s32 bank);
  void SetReadMemoryMapBase(s32 bank, u8* base);
  void MirrorReadMemoryMapBase(s32 bank_dest, s32 bank_src);

  void SetWriteMemoryMapBase(s32 bank, u8* base);

  u8 Read8MemoryMap(u32 address) const;
  void Write8MemoryMap(u32 address, u8 value);

  // Clock cycles.

  u32 GetCycles() const;
  void SetCycles(u32 cycles);
  void AddCycles(u32 cycles);
  void SubCycles(u32 cycles);

  u8 GetLastFetch() const;

  u8 GetIRQLine() const;
  void SetIRQLine(u32 state);
  void SetIRQCallback(IRQCallback callback);

  void SetNMILine(u32 state);

  void Run(u32 cycles);

  s32 LoadContext(u8* state);
  s32 SaveContext(u8* state);

private:

  void ProcessInterrupt();

  u8 ROP();
  u8 ARG();
  u32 ARG16();

  u8 IN(u32 port);
  void OUT(u32 port, u8 value);

  u8 RM(u32 address);
  void WM(u32 address, u8 data);

  void RM16(u32 address, Pair* reg);
  void WM16(u32 address, Pair* reg);

  void PUSH(Pair* reg);
  void POP(Pair* reg);

  /// Calculate the effective address EA of an opcode using IX + offset.
  void EAX();

  /// Calculate the effective address EA of an opcode using IY + offset.
  void EAY();

  // Operations.

  /// Enter HALT state.
  void EnterHalt();

  /// Leave HALT state.
  void LeaveHalt();

  void Illegal1();
  void Illegal2();

  u8 INC(u8 value);
  u8 DEC(u8 value);

  void RLCA();
  void RRCA();
  void RLA();
  void RRA();
  void RRD();
  void RLD();
  u8 RLC(u8 value);
  u8 RRC(u8 value);
  u8 RL(u8 value);
  u8 RR(u8 value);
  u8 SLA(u8 value);
  u8 SRA(u8 value);
  u8 SLL(u8 value);
  u8 SRL(u8 value);

  void BIT(u8 bit, u8 value);
  void BIT_HL(u8 bit, u8 value);
  void BIT_XY(u8 bit, u8 value);
  u8 RES(u8 bit, u8 value);
  u8 SET(u8 bit, u8 value);

  void ADD(u8 value);
  void ADC(u8 value);
  void SUB(u8 value);
  void SBC(u8 value);

  void ADD16(Pair* dest_reg, const Pair& src_reg);
  void ADC16(const Pair& reg);
  void SBC16(const Pair& reg);

  void AND(u8 value);
  void OR(u8 value);
  void XOR(u8 value);
  void CP(u8 value);

  void NEG();
  void DAA();

  void EX_AF();
  void EX_DE_HL();
  void EXX();
  void EXSP(Pair* reg);

  void CALL();
  void RETN();
  void RETI();

  void CALL_COND(bool cond, u8 opcode);
  void RET_COND(bool cond, u8 opcode);

  void LD_R_A();
  void LD_A_R();
  void LD_I_A();
  void LD_A_I();

  void LDI();
  void CPI();
  void INI();
  void OUTI();
  void LDD();
  void CPD();
  void IND();
  void OUTD();
  void LDIR();
  void CPIR();
  void INIR();
  void OTIR();
  void LDDR();
  void CPDR();
  void INDR();
  void OTDR();

  void JP();
  void JP_COND(bool cond);
  void JR();
  void JR_COND(bool cond, u8 opcode);

  void EI();
  void RST(u32 address);

  // Opcodes.

#define OPCODE_PROTOTYPES(prefix) \
  void prefix##_00(); void prefix##_01(); void prefix##_02(); void prefix##_03(); \
  void prefix##_04(); void prefix##_05(); void prefix##_06(); void prefix##_07(); \
  void prefix##_08(); void prefix##_09(); void prefix##_0a(); void prefix##_0b(); \
  void prefix##_0c(); void prefix##_0d(); void prefix##_0e(); void prefix##_0f(); \
  void prefix##_10(); void prefix##_11(); void prefix##_12(); void prefix##_13(); \
  void prefix##_14(); void prefix##_15(); void prefix##_16(); void prefix##_17(); \
  void prefix##_18(); void prefix##_19(); void prefix##_1a(); void prefix##_1b(); \
  void prefix##_1c(); void prefix##_1d(); void prefix##_1e(); void prefix##_1f(); \
  void prefix##_20(); void prefix##_21(); void prefix##_22(); void prefix##_23(); \
  void prefix##_24(); void prefix##_25(); void prefix##_26(); void prefix##_27(); \
  void prefix##_28(); void prefix##_29(); void prefix##_2a(); void prefix##_2b(); \
  void prefix##_2c(); void prefix##_2d(); void prefix##_2e(); void prefix##_2f(); \
  void prefix##_30(); void prefix##_31(); void prefix##_32(); void prefix##_33(); \
  void prefix##_34(); void prefix##_35(); void prefix##_36(); void prefix##_37(); \
  void prefix##_38(); void prefix##_39(); void prefix##_3a(); void prefix##_3b(); \
  void prefix##_3c(); void prefix##_3d(); void prefix##_3e(); void prefix##_3f(); \
  void prefix##_40(); void prefix##_41(); void prefix##_42(); void prefix##_43(); \
  void prefix##_44(); void prefix##_45(); void prefix##_46(); void prefix##_47(); \
  void prefix##_48(); void prefix##_49(); void prefix##_4a(); void prefix##_4b(); \
  void prefix##_4c(); void prefix##_4d(); void prefix##_4e(); void prefix##_4f(); \
  void prefix##_50(); void prefix##_51(); void prefix##_52(); void prefix##_53(); \
  void prefix##_54(); void prefix##_55(); void prefix##_56(); void prefix##_57(); \
  void prefix##_58(); void prefix##_59(); void prefix##_5a(); void prefix##_5b(); \
  void prefix##_5c(); void prefix##_5d(); void prefix##_5e(); void prefix##_5f(); \
  void prefix##_60(); void prefix##_61(); void prefix##_62(); void prefix##_63(); \
  void prefix##_64(); void prefix##_65(); void prefix##_66(); void prefix##_67(); \
  void prefix##_68(); void prefix##_69(); void prefix##_6a(); void prefix##_6b(); \
  void prefix##_6c(); void prefix##_6d(); void prefix##_6e(); void prefix##_6f(); \
  void prefix##_70(); void prefix##_71(); void prefix##_72(); void prefix##_73(); \
  void prefix##_74(); void prefix##_75(); void prefix##_76(); void prefix##_77(); \
  void prefix##_78(); void prefix##_79(); void prefix##_7a(); void prefix##_7b(); \
  void prefix##_7c(); void prefix##_7d(); void prefix##_7e(); void prefix##_7f(); \
  void prefix##_80(); void prefix##_81(); void prefix##_82(); void prefix##_83(); \
  void prefix##_84(); void prefix##_85(); void prefix##_86(); void prefix##_87(); \
  void prefix##_88(); void prefix##_89(); void prefix##_8a(); void prefix##_8b(); \
  void prefix##_8c(); void prefix##_8d(); void prefix##_8e(); void prefix##_8f(); \
  void prefix##_90(); void prefix##_91(); void prefix##_92(); void prefix##_93(); \
  void prefix##_94(); void prefix##_95(); void prefix##_96(); void prefix##_97(); \
  void prefix##_98(); void prefix##_99(); void prefix##_9a(); void prefix##_9b(); \
  void prefix##_9c(); void prefix##_9d(); void prefix##_9e(); void prefix##_9f(); \
  void prefix##_a0(); void prefix##_a1(); void prefix##_a2(); void prefix##_a3(); \
  void prefix##_a4(); void prefix##_a5(); void prefix##_a6(); void prefix##_a7(); \
  void prefix##_a8(); void prefix##_a9(); void prefix##_aa(); void prefix##_ab(); \
  void prefix##_ac(); void prefix##_ad(); void prefix##_ae(); void prefix##_af(); \
  void prefix##_b0(); void prefix##_b1(); void prefix##_b2(); void prefix##_b3(); \
  void prefix##_b4(); void prefix##_b5(); void prefix##_b6(); void prefix##_b7(); \
  void prefix##_b8(); void prefix##_b9(); void prefix##_ba(); void prefix##_bb(); \
  void prefix##_bc(); void prefix##_bd(); void prefix##_be(); void prefix##_bf(); \
  void prefix##_c0(); void prefix##_c1(); void prefix##_c2(); void prefix##_c3(); \
  void prefix##_c4(); void prefix##_c5(); void prefix##_c6(); void prefix##_c7(); \
  void prefix##_c8(); void prefix##_c9(); void prefix##_ca(); void prefix##_cb(); \
  void prefix##_cc(); void prefix##_cd(); void prefix##_ce(); void prefix##_cf(); \
  void prefix##_d0(); void prefix##_d1(); void prefix##_d2(); void prefix##_d3(); \
  void prefix##_d4(); void prefix##_d5(); void prefix##_d6(); void prefix##_d7(); \
  void prefix##_d8(); void prefix##_d9(); void prefix##_da(); void prefix##_db(); \
  void prefix##_dc(); void prefix##_dd(); void prefix##_de(); void prefix##_df(); \
  void prefix##_e0(); void prefix##_e1(); void prefix##_e2(); void prefix##_e3(); \
  void prefix##_e4(); void prefix##_e5(); void prefix##_e6(); void prefix##_e7(); \
  void prefix##_e8(); void prefix##_e9(); void prefix##_ea(); void prefix##_eb(); \
  void prefix##_ec(); void prefix##_ed(); void prefix##_ee(); void prefix##_ef(); \
  void prefix##_f0(); void prefix##_f1(); void prefix##_f2(); void prefix##_f3(); \
  void prefix##_f4(); void prefix##_f5(); void prefix##_f6(); void prefix##_f7(); \
  void prefix##_f8(); void prefix##_f9(); void prefix##_fa(); void prefix##_fb(); \
  void prefix##_fc(); void prefix##_fd(); void prefix##_fe(); void prefix##_ff();

  OPCODE_PROTOTYPES(op)
  OPCODE_PROTOTYPES(cb)
  OPCODE_PROTOTYPES(dd)
  OPCODE_PROTOTYPES(ed)
  OPCODE_PROTOTYPES(fd)
  OPCODE_PROTOTYPES(xycb)

#undef OPCODE_PROTOTYPES

private:
  u8* m_readmap[64];
  u8* m_writemap[64];

  WriteMemoryHandler m_writemem;
  ReadMemoryHandler m_readmem;

  WritePortHandler m_writeport;
  ReadPortHandler m_readport;

  IRQCallback m_irq_callback;

  // Z80 context.

  Pair m_pc;
  Pair m_sp;
  Pair m_af;
  Pair m_bc;
  Pair m_de;
  Pair m_hl;
  Pair m_ix;
  Pair m_iy;
  Pair m_wz;

  Pair m_af2;
  Pair m_bc2;
  Pair m_de2;
  Pair m_hl2;

  u8 m_r;
  u8 m_r2;
  u8 m_iff1;
  u8 m_iff2;
  u8 m_halt;
  u8 m_im;
  u8 m_i;

  u8 m_nmi_state;   /// NMI line state.
  u8 m_irq_state;   /// IRQ line state.
  u8 m_after_ei;    /// Are we in the EI shadow?
  u32 m_cycles;     /// Master clock cycles global counter.

  u8 m_last_fetch;

  u32 m_EA;

private:

  static u8 m_SZ[256];       // zero and sign flags.
  static u8 m_SZ_BIT[256];   // zero, sign and parity/overflow (=zero) flags for BIT opcode.
  static u8 m_SZP[256];      // zero, sign and parity flags.
  static u8 m_SZHV_inc[256]; // zero, sign, half carry and overflow flags INC r8.
  static u8 m_SZHV_dec[256]; // zero, sign, half carry and overflow flags DEC r8.

  static u8 m_SZHVC_add[2 * 256 * 256]; // flags for ADD opcode.
  static u8 m_SZHVC_sub[2 * 256 * 256]; // flags for SUB opcode.
};

} // namespace gpgx::cpu::z80

#endif // #ifndef __GPGX_CPU_Z80_Z80_H__


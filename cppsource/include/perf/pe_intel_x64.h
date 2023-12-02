
/*
 * ARM Cortex-A72 performance events
 */

/* 
 * Author:  P.J. Drongowski
 * Site:    sandsoftwaresound.net
 * Version: 1.0
 * Date:    19 November 2020
 *
 * Copyright (c) 2020 Paul J. Drongowski
 */

#ifndef _PE_INTEL_X64_H
#define _PE_INTEL_X64_H

#define X64_SW_INCR                0x00
#define X64_L1I_CACHE_REFILL       0x01
#define X64_L1I_TLB_REFILL         0x02
#define X64_L1D_CACHE_REFILL       0x03
#define X64_L1D_CACHE_ACCESS       0x04
#define X64_L1D_TLB_REFILL         0x05
#define X64_INST_RETIRED           0x08
#define X64_EXC_TAKEN              0x09
#define X64_EXC_EXECUTED           0x0A
#define X64_CID_WRITE              0x0B
#define X64_PC_BRANCH_MIS_PRED     0x10
#define X64_CPU_CYCLES             0x11
#define X64_PC_BRANCH_PRED         0x12
#define X64_MEM_ACCESS             0x13
#define X64_L1I_CACHE_ACCESS       0x14
#define X64_L1D_CACHE_WB           0x15
#define X64_L2_CACHE_ACCESS        0x16
#define X64_L2_CACHE_REFILL        0x17
#define X64_L2_CACHE_WB            0x18
#define X64_BUS_ACCESS             0x19
#define X64_MEMORY_ERROR           0x1A
#define X64_INSTR_SPEC             0x1B
#define X64_TTBR_WRITE             0x1C
#define X64_BUS_CYCLES             0x1D
#define X64_CHAIN                  0x1E
#define X64_L1D_CACHE_RD           0x40
#define X64_L1D_CACHE_WR           0x41
#define X64_L1D_CACHE_REFILL_RD    0x42
#define X64_L1D_CACHE_REFILL_WR    0x43
#define X64_L1D_CACHE_WB_VICTIM    0x46
#define X64_L1D_CACHE_WB_CLEAN     0x47
#define X64_L1D_CACHE_INVAL        0x48
#define X64_L1D_TLB_REFILL_RD      0x4C
#define X64_L1D_TLB_REFILL_WR      0x4D
#define X64_L2D_CACHE_RD           0x50
#define X64_L2D_CACHE_WR           0x51
#define X64_L2D_CACHE_REFILL_RD    0x52
#define X64_L2D_CACHE_REFILL_WR    0x53
#define X64_L2D_CACHE_WB_VICTIM    0x56
#define X64_L2D_CACHE_WB_CLEAN     0x57
#define X64_L2D_CACHE_INVAL        0x58
#define X64_BUS_ACCESS_RD          0x60
#define X64_BUS_ACCESS_WR          0x61
#define X64_BUS_ACCESS_SHARED      0x62
#define X64_BUS_ACCESS_NOT_SHARED  0x63
#define X64_BUS_ACCESS_NORMAL      0x64
#define X64_BUS_ACCESS_PERIPH      0x65
#define X64_MEM_ACCESS_RD          0x66
#define X64_MEM_ACCESS_WR          0x67
#define X64_UNALIGNED_LD_SPEC      0x68
#define X64_UNALIGNED_ST_SPEC      0x69
#define X64_UNALIGNED_LDST_SPEC    0x6A
#define X64_LDREX_SPEC             0x6C
#define X64_STREX_PASS_SPEC        0x6D
#define X64_STREX_FAIL_SPEC        0x6E
#define X64_LD_SPEC                0x70
#define X64_ST_SPEC                0x71
#define X64_LDST_SPEC              0x72
#define X64_DP_SPEC                0x73
#define X64_ASE_SPEC               0x74
#define X64_VFP_SPEC               0x75
#define X64_PC_WRITE_SPEC          0x76
#define X64_CRYPTO_SPEC            0x77
#define X64_BR_IMMED_SPEC          0x78
#define X64_BR_RETURN_SPEC         0x79
#define X64_BR_INDIRECT_SPEC       0x7A
#define X64_ISB_SPEC               0x7C
#define X64_DSB_SPEC               0x7D
#define X64_DMB_SPEC               0x7E
#define X64_EXC_UNDEF              0x81
#define X64_EXC_SVC                0x82
#define X64_EXC_PABORT             0x83
#define X64_EXC_DABORT             0x84
#define X64_EXC_IRQ                0x86
#define X64_EXC_FIQ                0x87
#define X64_EXC_SMC                0x88
#define X64_EXC_HVC                0x8A
#define X64_EXC_TRAP_PABORT        0x8B
#define X64_EXC_TRAP_DABORT        0x8C
#define X64_EXC_TRAP_OTHER         0x8D
#define X64_EXC_TRAP_IRQ           0x8E
#define X64_EXC_TRAP_FIQ           0x8F
#define X64_RC_LD_SPEC             0x90
#define X64_RC_ST_SPEC             0x91

// The following ARMv7 common events are not listed in the ARM Cortex-A72
// Technical Reference Manual (TRM). They may not be supported!
//   X64_MEM_READ
//   X64_MEM_WRITE
//   X64_PC_WRITE
//   X64_PC_IMM_BRANCH
//   X64_PC_PROC_RETURN
//   X64_MEM_UNALIGNED_ACCESS
// Use these events at your own risk!

#define X64_MEM_READ               0x06
#define X64_MEM_WRITE              0x07
#define X64_PC_WRITE               0x0C
#define X64_PC_IMM_BRANCH          0x0D
#define X64_PC_PROC_RETURN         0x0E
#define X64_MEM_UNALIGNED_ACCESS   0x0F

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exported measurement and reporting functions
 */
extern int x64MeasureInstructionEvents() ;
extern int x64PrintInstructionEvents(FILE* stream) ;
extern void x64CollectInstructionEvents(pe_event_collection* c);

extern int x64MeasureDataAccessEvents() ;
extern int x64PrintDataAccessEvents(FILE* stream) ;
extern void x64CollectDataAccessEvents(pe_event_collection* c);

extern int x64MeasureTlbEvents() ;
extern int x64PrintTlbEvents(FILE* stream) ;
extern void x64CollectTblEvents(pe_event_collection* c);

extern int x64MeasureDataCacheEvents() ;
extern int x64PrintDataCacheEvents(FILE* stream) ;
extern void x64CollectDataCacheEvents(pe_event_collection* c);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

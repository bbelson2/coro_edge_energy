
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

#ifndef _PE_CORTEX_A72_H
#define _PE_CORTEX_A72_H

#define A72_SW_INCR                0x00
#define A72_L1I_CACHE_REFILL       0x01
#define A72_L1I_TLB_REFILL         0x02
#define A72_L1D_CACHE_REFILL       0x03
#define A72_L1D_CACHE_ACCESS       0x04
#define A72_L1D_TLB_REFILL         0x05
#define A72_INST_RETIRED           0x08
#define A72_EXC_TAKEN              0x09
#define A72_EXC_EXECUTED           0x0A
#define A72_CID_WRITE              0x0B
#define A72_PC_BRANCH_MIS_PRED     0x10
#define A72_CPU_CYCLES             0x11
#define A72_PC_BRANCH_PRED         0x12
#define A72_MEM_ACCESS             0x13
#define A72_L1I_CACHE_ACCESS       0x14
#define A72_L1D_CACHE_WB           0x15
#define A72_L2_CACHE_ACCESS        0x16
#define A72_L2_CACHE_REFILL        0x17
#define A72_L2_CACHE_WB            0x18
#define A72_BUS_ACCESS             0x19
#define A72_MEMORY_ERROR           0x1A
#define A72_INSTR_SPEC             0x1B
#define A72_TTBR_WRITE             0x1C
#define A72_BUS_CYCLES             0x1D
#define A72_CHAIN                  0x1E
#define A72_L1D_CACHE_RD           0x40
#define A72_L1D_CACHE_WR           0x41
#define A72_L1D_CACHE_REFILL_RD    0x42
#define A72_L1D_CACHE_REFILL_WR    0x43
#define A72_L1D_CACHE_WB_VICTIM    0x46
#define A72_L1D_CACHE_WB_CLEAN     0x47
#define A72_L1D_CACHE_INVAL        0x48
#define A72_L1D_TLB_REFILL_RD      0x4C
#define A72_L1D_TLB_REFILL_WR      0x4D
#define A72_L2D_CACHE_RD           0x50
#define A72_L2D_CACHE_WR           0x51
#define A72_L2D_CACHE_REFILL_RD    0x52
#define A72_L2D_CACHE_REFILL_WR    0x53
#define A72_L2D_CACHE_WB_VICTIM    0x56
#define A72_L2D_CACHE_WB_CLEAN     0x57
#define A72_L2D_CACHE_INVAL        0x58
#define A72_BUS_ACCESS_RD          0x60
#define A72_BUS_ACCESS_WR          0x61
#define A72_BUS_ACCESS_SHARED      0x62
#define A72_BUS_ACCESS_NOT_SHARED  0x63
#define A72_BUS_ACCESS_NORMAL      0x64
#define A72_BUS_ACCESS_PERIPH      0x65
#define A72_MEM_ACCESS_RD          0x66
#define A72_MEM_ACCESS_WR          0x67
#define A72_UNALIGNED_LD_SPEC      0x68
#define A72_UNALIGNED_ST_SPEC      0x69
#define A72_UNALIGNED_LDST_SPEC    0x6A
#define A72_LDREX_SPEC             0x6C
#define A72_STREX_PASS_SPEC        0x6D
#define A72_STREX_FAIL_SPEC        0x6E
#define A72_LD_SPEC                0x70
#define A72_ST_SPEC                0x71
#define A72_LDST_SPEC              0x72
#define A72_DP_SPEC                0x73
#define A72_ASE_SPEC               0x74
#define A72_VFP_SPEC               0x75
#define A72_PC_WRITE_SPEC          0x76
#define A72_CRYPTO_SPEC            0x77
#define A72_BR_IMMED_SPEC          0x78
#define A72_BR_RETURN_SPEC         0x79
#define A72_BR_INDIRECT_SPEC       0x7A
#define A72_ISB_SPEC               0x7C
#define A72_DSB_SPEC               0x7D
#define A72_DMB_SPEC               0x7E
#define A72_EXC_UNDEF              0x81
#define A72_EXC_SVC                0x82
#define A72_EXC_PABORT             0x83
#define A72_EXC_DABORT             0x84
#define A72_EXC_IRQ                0x86
#define A72_EXC_FIQ                0x87
#define A72_EXC_SMC                0x88
#define A72_EXC_HVC                0x8A
#define A72_EXC_TRAP_PABORT        0x8B
#define A72_EXC_TRAP_DABORT        0x8C
#define A72_EXC_TRAP_OTHER         0x8D
#define A72_EXC_TRAP_IRQ           0x8E
#define A72_EXC_TRAP_FIQ           0x8F
#define A72_RC_LD_SPEC             0x90
#define A72_RC_ST_SPEC             0x91

// The following ARMv7 common events are not listed in the ARM Cortex-A72
// Technical Reference Manual (TRM). They may not be supported!
//   A72_MEM_READ
//   A72_MEM_WRITE
//   A72_PC_WRITE
//   A72_PC_IMM_BRANCH
//   A72_PC_PROC_RETURN
//   A72_MEM_UNALIGNED_ACCESS
// Use these events at your own risk!

#define A72_MEM_READ               0x06
#define A72_MEM_WRITE              0x07
#define A72_PC_WRITE               0x0C
#define A72_PC_IMM_BRANCH          0x0D
#define A72_PC_PROC_RETURN         0x0E
#define A72_MEM_UNALIGNED_ACCESS   0x0F

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exported measurement and reporting functions
 */
extern int a72MeasureInstructionEvents() ;
extern int a72PrintInstructionEvents(FILE* stream) ;
extern void a72CollectInstructionEvents(pe_event_collection* c);

extern int a72MeasureDataAccessEvents() ;
extern int a72PrintDataAccessEvents(FILE* stream) ;
extern void a72CollectDataAccessEvents(pe_event_collection* c);

extern int a72MeasureTlbEvents() ;
extern int a72PrintTlbEvents(FILE* stream) ;
extern void a72CollectTblEvents(pe_event_collection* c);

extern int a72MeasureDataCacheEvents() ;
extern int a72PrintDataCacheEvents(FILE* stream) ;
extern void a72CollectDataCacheEvents(pe_event_collection* c);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

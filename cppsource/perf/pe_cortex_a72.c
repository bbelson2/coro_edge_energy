
/*
 * ARM Cortex-A72 performance event assistance functions
 * Use these definitions on the Raspberry Pi 4 (BCM2711 ARM Cortex-A72)
 */

/* 
 * Author:  P.J. Drongowski
 * Site:    sandsoftwaresound.net
 * Version: 1.2
 * Date:    18 February 2021
 *
 * Copyright (c) 2021 Paul J. Drongowski
 */

#include "pe_assist.h"
#include "pe_cortex_a72.h"

static void a72Error(char* fn, char* event)
{
  fprintf(stderr, "Couldn't add event %s (%s)\n", event, fn) ;
}

/*
 * Configure the event counters to measure basic "front-end" instruction
 * events using processor-specific Cortex-72 RAW events.
 *
 */
int a72MeasureInstructionEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr,
	    "Couldn't make event group (a72MeasureInstructionEvents)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  if (peAddLeader(PERF_TYPE_RAW, A72_INST_RETIRED) == 0) {
    a72Error("a72MeasureInstructionEvents", "instructions retired") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x11 (cpu cycles)
  if (peAddEvent(PERF_TYPE_RAW, A72_CPU_CYCLES) == 0) {
    a72Error("a72MeasureInstructionEvents", "CPU cycles") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x76 (PC write spec)
  if (peAddEvent(PERF_TYPE_RAW, A72_PC_WRITE_SPEC) == 0) {
    fprintf(stderr, "Couldn't add event to the group (PC write spec)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x10 (Branch mispredictions)
  if (peAddEvent(PERF_TYPE_RAW, A72_PC_BRANCH_MIS_PRED) == 0) {
    fprintf(stderr, "Couldn't add event to the group (branch mispredict)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x12 (Branch predicted)
  if (peAddEvent(PERF_TYPE_RAW, A72_PC_BRANCH_PRED) == 0) {
    fprintf(stderr, "Couldn't add event to the group (branch predict)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x76 (Instructions speculated)
  if (peAddEvent(PERF_TYPE_RAW, A72_INSTR_SPEC) == 0) {
    fprintf(stderr, "Couldn't add event to the group (Instructions spec'd)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  return( 1 ) ;
}

#ifndef A72_EXCLUDE_PRINTS

/*
 * Print basic "front-end" instruction event counts to the specified stream.
 */
int a72PrintInstructionEvents(FILE* stream)
{
  // Read instruction event counts
  long long instr_retired      = peReadCount(0) ;
  long long cpu_cycles         = peReadCount(1) ;
  long long branch_spec        = peReadCount(2) ;
  long long branch_mispredict  = peReadCount(3) ;
  long long branch_predict     = peReadCount(4) ;
  long long instr_speculated   = peReadCount(5) ;

  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions ret'd:  %'lld\n", instr_retired) ;
  fprintf(stream, "Instructions spec'd: %'lld\n", instr_speculated) ;
  fprintf(stream, "CPU cycles:          %'lld\n", cpu_cycles) ;
  fprintf(stream, "Branch speculated :  %'lld\n", branch_spec) ;
  fprintf(stream, "Branch mispredicted: %'lld\n", branch_mispredict) ;
  fprintf(stream, "Branch predicted     %'lld\n", branch_predict) ;

  fprintf(stream, "Instructions per cycle:  %4.3f\n",
	  peEventRatio(instr_retired, cpu_cycles)) ;
  fprintf(stream, "Retired/spec'd ratio:    %4.3f\n",
	  peEventRatio(instr_retired, instr_speculated)) ;
  fprintf(stream, "Branches per 1000 (PTI): %5.3f\n",
	  peEventPTI(branch_spec, instr_retired)) ;
  fprintf(stream, "Branch mispredict ratio: %4.3f\n",
	  peEventRatio(branch_mispredict, branch_spec)) ;

  return( 1 ) ;
}

#endif

static const char* a72InstructionEventNames[] = {
  "instructions_retired",
  "cpu_cycles",
  "branch_speculated",
  "branch_mispredicted",
  "branch_predicted",
  "instructions_speculated",
};

static const char* a72InstructionEventTitles[] = {
  "Instructions retired",
  "CPU cycles",
  "Branches speculated",
  "Branches mispredicted",
  "Branches predicted",
  "Instructions speculated",
};

static const char* a72InstructionRatioNames[] = {
  "instructions_per_cycle",
  "retired_speculated_ratio",
  "branch_speculated_pti",
  "branch_mispredict_ratio",
};

static const char* a72InstructionRatioTitles[] = {
  "Instructions per cycle",
  "Retired/speculated ratio",
  "Branches speculated (PTI)",
  "Branches mispredicted ratio",
};

extern void a72CollectInstructionEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instr_retired      = peReadCount(0) ;
  long long cpu_cycles         = peReadCount(1) ;
  long long branch_spec        = peReadCount(2) ;
  long long branch_mispredict  = peReadCount(3) ;
  long long branch_predict     = peReadCount(4) ;
  long long instr_speculated   = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, a72InstructionEventNames, a72InstructionEventTitles,
    4, a72InstructionRatioNames, a72InstructionRatioTitles);

  c->values[0].value = instr_retired;
  c->values[1].value = cpu_cycles;
  c->values[2].value = branch_spec;
  c->values[3].value = branch_mispredict;
  c->values[4].value = branch_predict;
  c->values[5].value = instr_speculated;

  c->ratios[0].value = peEventRatio(instr_retired, cpu_cycles);
  c->ratios[1].value = peEventRatio(instr_retired, instr_speculated);
  c->ratios[2].value = peEventPTI(branch_spec, instr_retired);
  c->ratios[3].value = peEventRatio(branch_mispredict, branch_spec);
}

/*
 * Configure performance counters to measure data access events.
 * Use architecture-specific Cortex-A72 RAW events.
 */
int a72MeasureDataAccessEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr, "Couldn't make group (a72MeasureDataAccessEvents)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  if (peAddLeader(PERF_TYPE_RAW, A72_INST_RETIRED) == 0) {
    a72Error("a72MeasureDataAccessEvents", "instructions") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x11 (cpu cycles)
  if (peAddEvent(PERF_TYPE_RAW, A72_CPU_CYCLES) == 0) {
    a72Error("a72MeasureDataAccessEvents", "CPU cycles") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x04 (L1 D-cache access)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_ACCESS) == 0) {
    a72Error("a72MeasureDataAccessEvents", "L1 D-cache access") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x03 (L1 D-cache refill)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_REFILL) == 0) {
    a72Error("a72MeasureDataAccessEvents", "L1 D-cache miss") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x50 (L2 cache access read)
  if (peAddEvent(PERF_TYPE_RAW, A72_L2D_CACHE_RD) == 0) {
    a72Error("a72MeasureDataAccessEvents", "L2 cache read") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x52 (L2 cache refill read)
    if (peAddEvent(PERF_TYPE_RAW, A72_L2D_CACHE_REFILL_RD) == 0) {
    a72Error("a72MeasureDataAccessEvents", "L2 cache read miss") ;
    exit( EXIT_FAILURE ) ;
  }
  return( 1 ) ;
}

#ifndef A72_EXCLUDE_PRINTS

int a72PrintDataAccessEvents(FILE* stream)
{
  // Read instruction event counts
  long long instructions    = peReadCount(0) ;
  long long cpu_cycles      = peReadCount(1) ;
  long long d_cache_reads   = peReadCount(2) ;
  long long d_cache_misses  = peReadCount(3) ;
  long long l2_cache_reads  = peReadCount(4) ;
  long long l2_cache_misses = peReadCount(5) ;
  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions:         %'lld\n", instructions) ;
  fprintf(stream, "CPU cycles:           %'lld\n", cpu_cycles) ;
  fprintf(stream, "L1 D-cache reads:     %'lld\n", d_cache_reads) ;
  fprintf(stream, "L1 D-cache misses:    %'lld\n", d_cache_misses) ;
  fprintf(stream, "L2 cache reads:       %'lld\n", l2_cache_reads) ;
  fprintf(stream, "L2 cache read misses: %'lld\n", l2_cache_misses) ;

  fprintf(stream, "Instructions per cycle:  %4.3f\n",
	  peEventRatio(instructions, cpu_cycles)) ;
  fprintf(stream, "L1 D-cache reads (PTI):  %5.3f\n",
	  peEventPTI(d_cache_reads, instructions)) ;
  fprintf(stream, "L1 D-cache miss ratio:   %4.3f\n",
	  peEventRatio(d_cache_misses, d_cache_reads)) ;
  fprintf(stream, "L2 cache reads (PTI):    %5.3f\n",
	  peEventPTI(l2_cache_reads, instructions)) ;
  fprintf(stream, "L2 cache miss ratio:     %4.3f\n",
	  peEventRatio(l2_cache_misses, l2_cache_reads)) ;
  return( 1 ) ;
}

#endif

static const char* a72DataAccessEventNames[] = {
  "instructions",
  "cpu_cycles",
  "d_cache_reads",
  "d_cache_misses",
  "l2_cache_reads",
  "l2_cache_misses",
};

static const char* a72DataAccessEventTitles[] = {
  "Instructions",
  "CPU cycles",
  "L1 data cache reads",
  "L1 data cache misses",
  "L2 cache reads",
  "L2 cache read misses",
};

static const char* a72DataAccessRatioNames[] = {
  "instructions_per_cycle",
  "l1_dcache_reads_pti",
  "l1_dcache_miss_ratio",
  "l2_cache_reads_pti",
  "l2_cache_miss_ratio"
};

static const char* a72DataAccessRatioTitles[] = {
  "Instructions per cycle",
  "L1 D-cache reads (PTI)",
  "L1 D-cache miss ratio",
  "L2 cache reads (PTI)",
  "L2 cache miss ratio"
};

extern void a72CollectDataAccessEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instructions    = peReadCount(0) ;
  long long cpu_cycles      = peReadCount(1) ;
  long long d_cache_reads   = peReadCount(2) ;
  long long d_cache_misses  = peReadCount(3) ;
  long long l2_cache_reads  = peReadCount(4) ;
  long long l2_cache_misses = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, a72DataAccessEventNames, a72DataAccessEventTitles, 
    5, a72DataAccessRatioNames, a72DataAccessRatioTitles
  );

  c->values[0].value = instructions;
  c->values[1].value = cpu_cycles;
  c->values[2].value = d_cache_reads;
  c->values[3].value = d_cache_misses;
  c->values[4].value = l2_cache_reads;
  c->values[5].value = l2_cache_misses;

  c->ratios[0].value = peEventRatio(instructions, cpu_cycles);
  c->ratios[1].value = peEventPTI(d_cache_reads, instructions);
  c->ratios[2].value = peEventRatio(d_cache_misses, d_cache_reads);
  c->ratios[3].value = peEventPTI(l2_cache_reads, instructions);
  c->ratios[4].value = peEventRatio(l2_cache_misses, l2_cache_reads);
}

/*
 * Configure performance counters to measure translation lookaside
 * buffer (TLB) events. Use architecture-specific Cortex-A72 RAW events.
 */
int a72MeasureTlbEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr, "Couldn't make group (a72MeasureTlbEvents)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  if (peAddLeader(PERF_TYPE_RAW, A72_INST_RETIRED) == 0) {
    a72Error("a72MeasureTlbEvents", "instructions") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x04 (L1 D-cache access)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_ACCESS) == 0) {
    a72Error("a72MeasureTlbEvents", "L1 D-cache access") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x05 (L1 DTLB refill)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_TLB_REFILL) == 0) {
    a72Error("a72MeasureTlbEvents", "L1 DTLB refill") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x4C (L1 DTLB refill LD)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_TLB_REFILL_RD) == 0) {
    a72Error("a72MeasureTlbEvents", "L1 DTLB refill LD") ;
    exit( EXIT_FAILURE ) ;
  }
  
  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x4D (L1 DTLB refill ST)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_TLB_REFILL_WR) == 0) {
    a72Error("a72MeasureTlbEvents", "L1 DTLB refill ST") ;
    exit( EXIT_FAILURE ) ;
  }
  
  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x02 (L1 ITLB refill)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1I_TLB_REFILL) == 0) {
    a72Error("a72MeasureTlbEvents", "L1 ITLB refill") ;
    exit( EXIT_FAILURE ) ;
  }

  return( 1 ) ;
}

#ifndef A72_EXCLUDE_PRINTS

int a72PrintTlbEvents(FILE* stream)
{
  // Read instruction event counts
  long long instructions      = peReadCount(0) ;
  long long d_cache_access    = peReadCount(1) ;
  long long l1_dtlb_refill    = peReadCount(2) ;
  long long l1_dtlb_refill_ld = peReadCount(3) ;
  long long l1_dtlb_refill_st = peReadCount(4) ;
  long long l1_itlb_refill    = peReadCount(5) ;

  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions:      %'lld\n", instructions) ;
  fprintf(stream, "L1 D-cache reads:  %'lld\n", d_cache_access) ;
  fprintf(stream, "L1 DTLB miss:      %'lld\n", l1_dtlb_refill) ;
  fprintf(stream, "L1 DTLB miss LD:   %'lld\n", l1_dtlb_refill_ld) ;
  fprintf(stream, "L1 DTLB miss ST:   %'lld\n", l1_dtlb_refill_st) ;
  fprintf(stream, "L1 ITLB miss:      %'lld\n", l1_itlb_refill) ;

  fprintf(stream, "L1 D-cache reads (PTI):  %5.3f\n",
	  peEventPTI(d_cache_access, instructions)) ;
  fprintf(stream, "L1 DTLB miss ratio:      %4.3f\n",
	  peEventRatio(l1_dtlb_refill, d_cache_access)) ;
  return( 1 ) ;
}

#endif

static const char* a72TlbEventNames[] = {
  "instructions",
  "l1_dcache_reads",
  "l1_dtlb_miss",
  "l1_dtlb_miss_ld",
  "l1_dtlb_miss_st",
  "l1_itlb_miss",
};

static const char* a72TlbEventTitles[] = {
  "Instructions",
  "L1 D-cache reads",
  "L1 DTLB miss",
  "L1 DTLB miss LD",
  "L1 DTLB miss ST",
  "L1 ITLB miss",
};

static const char* a72TlbRatioNames[] = {
  "l1_dcache_reads_pti",
  "l1_dtlb_miss_ratio"
};

static const char* a72TlbRatioTitles[] = {
  "L1 D-cache reads (PTI)",
  "L1 DTLB miss ratio",
};

extern void a72CollectTlbEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instructions      = peReadCount(0) ;
  long long d_cache_access    = peReadCount(1) ;
  long long l1_dtlb_refill    = peReadCount(2) ;
  long long l1_dtlb_refill_ld = peReadCount(3) ;
  long long l1_dtlb_refill_st = peReadCount(4) ;
  long long l1_itlb_refill    = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, a72TlbEventNames, a72TlbEventTitles, 
    2, a72TlbRatioNames, a72TlbRatioTitles
  );

  c->values[0].value = instructions;
  c->values[1].value = d_cache_access;
  c->values[2].value = l1_dtlb_refill;
  c->values[3].value = l1_dtlb_refill_ld;
  c->values[4].value = l1_dtlb_refill_st;
  c->values[5].value = l1_itlb_refill;

  c->ratios[0].value = peEventPTI(d_cache_access, instructions);
  c->ratios[1].value = peEventRatio(l1_dtlb_refill, d_cache_access);
}

/*
 * Configure performance counters to measure level 1 data cache events.
 * Use architecture-specific Cortex-A72 RAW events.
 */
int a72MeasureDataCacheEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr, "Couldn't make group (a72MeasureDataCacheEvents)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  if (peAddLeader(PERF_TYPE_RAW, A72_INST_RETIRED) == 0) {
    a72Error("a72MeasureDataCacheEvents", "instructions") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x11 (cpu cycles)
  if (peAddEvent(PERF_TYPE_RAW, A72_CPU_CYCLES) == 0) {
    a72Error("a72MeasureDataCacheEvents", "CPU cycles") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x04 (L1 D-cache access)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_ACCESS) == 0) {
    a72Error("a72MeasureDataCacheEvents", "L1 D-cache access") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x03 (L1 D-cache refill)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_REFILL) == 0) {
    a72Error("a72MeasureDataCacheEvents", "L1 D-cache miss") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x47 (L1 D-cache write-back clean)
  if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_WB_CLEAN) == 0) {
    a72Error("a72MeasureDataCacheEvents", "L1 D-cache write-back clean") ;
    exit( EXIT_FAILURE ) ;
  }
  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x48 (L1 D-cache invalidate)
    if (peAddEvent(PERF_TYPE_RAW, A72_L1D_CACHE_INVAL) == 0) {
    a72Error("a72MeasureDataCacheEvents", "L1 D-cache invalidate") ;
    exit( EXIT_FAILURE ) ;
  }
  return( 1 ) ;
}

#ifndef A72_EXCLUDE_PRINTS

int a72PrintDataCacheEvents(FILE* stream)
{
  // Read instruction event counts
  long long instructions     = peReadCount(0) ;
  long long cpu_cycles       = peReadCount(1) ;
  long long d_cache_reads    = peReadCount(2) ;
  long long d_cache_misses   = peReadCount(3) ;
  long long d_cache_wb_clean = peReadCount(4) ;
  long long d_cache_invalid  = peReadCount(5) ;

  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions:          %'lld\n", instructions) ;
  fprintf(stream, "CPU cycles:            %'lld\n", cpu_cycles) ;
  fprintf(stream, "L1 D-cache reads:      %'lld\n", d_cache_reads) ;
  fprintf(stream, "L1 D-cache misses:     %'lld\n", d_cache_misses) ;
  fprintf(stream, "L1 D-cache WB clean:   %'lld\n", d_cache_wb_clean) ;
  fprintf(stream, "L1 D-cache invalidate: %'lld\n", d_cache_invalid) ;

  fprintf(stream, "Instructions per cycle:  %4.3f\n",
	  peEventRatio(instructions, cpu_cycles)) ;
  fprintf(stream, "L1 D-cache reads (PTI):  %5.3f\n",
	  peEventPTI(d_cache_reads, instructions)) ;
  fprintf(stream, "L1 D-cache miss ratio:   %4.3f\n",
	  peEventRatio(d_cache_misses, d_cache_reads)) ;
  return( 1 ) ;
}
#endif

static const char* a72DataCacheEventNames[] = {
  "instructions",
  "cpu_cycles",
  "l1_d_cache_reads",
  "l1_d_cache_misses",
  "l1_dcache_wb_clean",
  "l1_dcache_invalidate",
};

static const char* a72DataCacheEventTitles[] = {
  "Instructions",
  "CPU cycles",
  "L1 data cache reads",
  "L1 data cache misses",
  "L1 data cache WB clean",
  "L1 data cache invalidate",
};

static const char* a72DataCacheRatioNames[] = {
  "instructions_per_cycle",
  "l1_dcache_reads_pti",
  "l1_dcache_miss_ratio",
};

static const char* a72DataCacheRatioTitles[] = {
  "Instructions per cycle",
  "L1 D-cache reads (PTI)",
  "L1 D-cache miss ratio",
};

extern void a72CollectDataCacheEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instructions     = peReadCount(0) ;
  long long cpu_cycles       = peReadCount(1) ;
  long long d_cache_reads    = peReadCount(2) ;
  long long d_cache_misses   = peReadCount(3) ;
  long long d_cache_wb_clean = peReadCount(4) ;
  long long d_cache_invalid  = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, a72DataCacheEventNames, a72DataCacheEventTitles, 
    3, a72DataCacheRatioNames, a72DataCacheRatioTitles
  );

  c->values[0].value = instructions;
  c->values[1].value = cpu_cycles;
  c->values[2].value = d_cache_reads;
  c->values[3].value = d_cache_misses;
  c->values[4].value = d_cache_wb_clean;
  c->values[5].value = d_cache_invalid;

  c->ratios[0].value = peEventRatio(instructions, cpu_cycles);
  c->ratios[1].value = peEventPTI(d_cache_reads, instructions);
  c->ratios[2].value = peEventRatio(d_cache_misses, d_cache_reads);
}


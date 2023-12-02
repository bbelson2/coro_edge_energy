
/*
 * Performance event assistance functions
 */

/* 
 * Author:  P.J. Drongowski
 * Site:    sandsoftwaresound.net
 * Version: 1.0
 * Date:    19 November 2020
 *
 * Copyright (c) 2020 Paul J. Drongowski
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "perf/pe_assist.h"

static int           peNumberOfGroups = 0 ;     // For now, one group maximum
static int           peNumberOfEvents = 0 ;     // Number of events in the group
static int           peLeaderDefined  = 0 ;     // TRUE if leader is defined
static pid_t         pePID = 0 ;                // The process to be monitored
static int           peCPU = -1 ;               // The CPU to be monitored
static unsigned long peFlags = 0 ;              // Event monitoring flags
static int           peGroupFD = 0 ;            // Event group file descriptor
static int           peEventFD[PE_MAX_EVENTS] ; // Event file descriptors

static struct perf_event_attr peAttr ;          // Working event attributes
static int           peEventIndex = 0 ;         // Working event array index

static double        starting_clock_time = 0.0; // For CPU time measurement

/*
 * Linux performance event system call
 */
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}

/*
 * Initialize internal data structures. Must be called first.
 */
int peInitialize()
{
  int i ;
  
  peNumberOfGroups = 0 ;
  peNumberOfEvents = 0 ;
  peLeaderDefined = 0 ;
  pePID = 0 ;
  peCPU = -1 ;
  peFlags = 0 ;
  peGroupFD = 0 ;
  for (i = 0 ; i < PE_MAX_EVENTS ; i++) peEventFD[i] = 0 ;

  peEventIndex = 0 ;
  
  return( 1 ) ;
}

/*
 * Make a performance event/monitoring group. Define the number of events in
 * the group including the leader. All events in the group have the same
 * PID, CPU and flag parameter values.
 *    num: Number of events in the group (including the leader)
 *    pid: Process ID parameter
 *    cpu: CPU parameter (-1 is all)
 *    flags: perf_event_open() flags parameter
 */
int peMakeGroup(int num, pid_t pid, int cpu, unsigned long flags)
{
  if (peNumberOfGroups >= PE_MAX_GROUPS) {
    fprintf(stderr, "Too many groups\n") ;
    return( 0 ) ;
  }
  if (num > PE_MAX_EVENTS) {
    fprintf(stderr, "Too many events requested for group") ;
    return( 0 ) ;
  }
  peNumberOfGroups++ ;
  peNumberOfEvents = num ;
  peEventIndex = 0 ;
  peLeaderDefined = 0 ;
  pePID = pid ;
  peCPU = cpu ;
  peFlags = flags ;
  return( 1 ) ;
}

/*
 * Add event leader to an existing event group.
 *    etype: Perf event type
 *    econfig: Perf event identifier
 */
int peAddLeader(long unsigned etype, long unsigned econfig)
{
  if (peNumberOfGroups == 0) {
    fprintf(stderr, "No group defined (peAddLeader)\n") ;
    return( 0 ) ;
  }
  if (peLeaderDefined) {
    fprintf(stderr, "Group event leader already defined") ;
    return( 0 ) ;
  }
  if (peEventIndex >= peNumberOfEvents) {
    fprintf(stderr, "Too many events (peAddLeader)\n") ;
    return( 0 ) ;
  }

  memset(&peAttr, 0, sizeof(struct perf_event_attr)) ;
  peAttr.type = etype ;
  peAttr.size = sizeof(struct perf_event_attr) ;
  peAttr.config = econfig ;
  peAttr.disabled = 1 ;
  peAttr.exclude_kernel = 1 ;
  peAttr.exclude_hv = 1 ;

  peGroupFD = perf_event_open(&peAttr, pePID, peCPU, -1, peFlags) ;
  if (peGroupFD == -1) {
    perror("peAddLeader") ;
    fprintf(stderr, "Error opening leader %llx\n", peAttr.config) ;
    exit(EXIT_FAILURE) ;
  }

  // Save the event file descriptor and advance the index
  peEventFD[peEventIndex] = peGroupFD ;
  peEventIndex++ ;
  peLeaderDefined = 1 ;
  return( 1 ) ;
}

/*
 * Add event to a group with an existing leader
 */
int peAddEvent(long unsigned etype, long unsigned econfig)
{
  int efd = 0 ;

  if (peNumberOfGroups == 0) {
    fprintf(stderr, "No group defined (peAddEvent)\n") ;
    return( 0 ) ;
  }
  if (peLeaderDefined == 0) {
    fprintf(stderr, "Event leader not defined for event group") ;
    return( 0 ) ;
  }
  if (peEventIndex >= peNumberOfEvents) {
    fprintf(stderr, "Too many events (peAddEvent)\n") ;
    return( 0 ) ;
  }

  memset(&peAttr, 0, sizeof(struct perf_event_attr)) ;
  peAttr.type = etype ;
  peAttr.size = sizeof(struct perf_event_attr) ;
  peAttr.config = econfig ;
  peAttr.disabled = 0 ;
  peAttr.exclude_kernel = 1 ;
  peAttr.exclude_hv = 1 ;

  efd = perf_event_open(&peAttr, pePID, peCPU, peGroupFD, peFlags) ;
  if (efd == -1) {
    perror("peAddEvent") ;
    fprintf(stderr, "Error opening event %llx\n", peAttr.config) ;
    exit( EXIT_FAILURE ) ;
  }

  // Save the event file descriptor and advance the index
  peEventFD[peEventIndex] = efd ;
  peEventIndex++ ;
  return( 1 ) ;
}

int peStartCounting()
{
  if (ioctl(peGroupFD, PERF_EVENT_IOC_RESET, 0) < 0) {
    perror("peStartCounting") ;
    fprintf(stderr, "Error when resetting event counters\n") ;
    return( 0 ) ;
  }
  if (ioctl(peGroupFD, PERF_EVENT_IOC_ENABLE, 0) < 0) {
    perror("peStartCounting") ;
    fprintf(stderr, "Error when starting event counters\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

int peStartEvent(int eindex)
{
  if (ioctl(peEventFD[eindex], PERF_EVENT_IOC_RESET, 0) < 0) {
    perror("peStartEvent") ;
    fprintf(stderr, "Error when resetting event counter\n") ;
    return( 0 ) ;
  }
  if (ioctl(peEventFD[eindex], PERF_EVENT_IOC_ENABLE, 0) < 0) {
    perror("peStartEvent") ;
    fprintf(stderr, "Error when starting event counter\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

int peStopCounting()
{
  if (ioctl(peGroupFD, PERF_EVENT_IOC_DISABLE, 0) < 0) {
    perror("peStopCounting") ;
    fprintf(stderr, "Error when stopping event counters\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

int peResetCounters()
{
  if (ioctl(peGroupFD, PERF_EVENT_IOC_RESET, 0) < 0) {
    perror("peResetCounters") ;
    fprintf(stderr, "Error when resetting event counters\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

long long peReadCount(int eindex)
{
  long long count ;
  int fd ;

  if ((eindex < 0) || (eindex >= peNumberOfEvents)) {
    fprintf(stderr, "Invalid event index %d (pePrintCount)\n", eindex) ;
    return( 0 ) ;
  }

  fd = peEventFD[eindex] ;
  if (read(fd, &count, sizeof(long long)) < 0) {
    perror("peReadCount") ;
    fprintf(stderr, "Error when reading event count %d\n", eindex) ;
    return( 0 ) ;
  }
  return( count ) ;
}

#ifndef PE_EXCLUDE_PRINTS

void pePrintCount(FILE* stream, int eindex, char *legend)
{
  long long count ;

  setlocale(LC_NUMERIC, "") ;
  
  count = peReadCount(eindex) ;
  fprintf(stream, "%s%'lld\n", legend, count) ;
}

#endif

/*
 * Compute the ratio of two events.
 */
double peEventRatio(long long num, long long den)
{
  if (den == 0) return( -1.0 ) ;
  
  return( (double)num / (double)den ) ;
}

/*
 * Compute the number of events per 1000 instructions (PTI).
 */
double peEventPTI(long long num, long long instructions)
{
  if (instructions == 0) return( -1.0 ) ;
  
  return( ((double)num / (double)instructions) * 1000.0 ) ;
}

/*
 * Configure the event counters to measure basic "front-end" instruction
 * events using the symbolic built-in events. (Do not use RAW events that
 * are processor-specific).
 */
int peMeasureInstructionEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr, "Couldn't make event group (main)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (Instructions retired)
  if (peAddLeader(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS) == 0) {
    fprintf(stderr, "Couldn't add event group leader (instructions)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x11 (CPU cycles)
  if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES) == 0) {
    fprintf(stderr, "Couldn't add event to the group (cpu cycles)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x0C (PC write retired)
  if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS) == 0) {
    fprintf(stderr, "Couldn't add event to the group (branches)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x10 (Branch mispredictions)
  if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES) == 0) {
    fprintf(stderr, "Couldn't add event to the group (branch misses)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x14 (L1 I-cache read access)
  if (peAddEvent(PERF_TYPE_HW_CACHE,
		 (PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ<<8) |
		  (PERF_COUNT_HW_CACHE_RESULT_ACCESS<<16))) == 0) {
    fprintf(stderr, "Couldn't add event to the group (i-cache reads)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x01 (L1 I-cache read refill)
  if (peAddEvent(PERF_TYPE_HW_CACHE,
		 (PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ<<8) |
		  (PERF_COUNT_HW_CACHE_RESULT_MISS)<<16)) == 0) {
    fprintf(stderr, "Couldn't add event to the group (i-cache misses)\n") ;
    exit( EXIT_FAILURE ) ;
  }
  return( 1 ) ;
}

#ifndef PE_EXCLUDE_PRINTS

/*
 * Print basic "front-end" instruction event counts to the specified stream.
 */
int pePrintInstructionEvents(FILE* stream)
{
  // Read instruction event counts
  long long instructions   = peReadCount(0) ;
  long long cpu_cycles     = peReadCount(1) ;
  long long branches       = peReadCount(2) ;
  long long branch_misses  = peReadCount(3) ;
  long long i_cache_reads  = peReadCount(4) ;
  long long i_cache_misses = peReadCount(5) ;

  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions:   %'lld\n", instructions) ;
  fprintf(stream, "CPU cycles:     %'lld\n", cpu_cycles) ;
  fprintf(stream, "Branches:       %'lld\n", branches) ;
  fprintf(stream, "Branch misses:  %'lld\n", branch_misses) ;
  fprintf(stream, "I-cache reads:  %'lld\n", i_cache_reads) ;
  fprintf(stream, "I-cache misses: %'lld\n", i_cache_misses) ;

  fprintf(stream, "Instructions per cycle:  %4.3f\n",
	  peEventRatio(instructions, cpu_cycles)) ;
  fprintf(stream, "Branches per 1000 (PTI): %5.3f\n",
	  peEventPTI(branches, instructions)) ;
  fprintf(stream, "Branch mispredict ratio: %4.3f\n",
	  peEventRatio(branch_misses, branches)) ;
  fprintf(stream, "L1 I-cache reads (PTI):  %5.3f\n",
	  peEventPTI(i_cache_reads, instructions)) ;
  fprintf(stream, "L1 I-cache miss ratio:   %4.3f\n",
	  peEventRatio(i_cache_misses, i_cache_reads)) ;
  return( 1 ) ;
}

#endif

static const char* peInstructionEventNames[] = {
  "instructions",
  "cpu_cycles",
  "branches",
  "branch_misses",
  "l1_icache_reads",
  "l1_icache_misses",
};

static const char* peInstructionEventTitles[] = {
  "Instructions",
  "CPU cycles",
  "Branches",
  "Branch misses",
  "L1 icache reads",
  "L1 icache read misses",
};

static const char* peInstructionRatioNames[] = {
  "instructions_per_cycle",
  "branches_pti",
  "branch_mispredict",
  "l1_icache_reads_pti",
  "l1_icache_miss_ratio"
};

static const char* peInstructionRatioTitles[] = {
  "Instructions per cycle",
  "Branches (PTI)",
  "Branch mispredict ratio",
  "L1 I-cache reads (PTI)",
  "L1 I-cache miss ratio",
};

extern void peCollectInstructionEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instructions   = peReadCount(0) ;
  long long cpu_cycles     = peReadCount(1) ;
  long long branches       = peReadCount(2) ;
  long long branch_misses  = peReadCount(3) ;
  long long i_cache_reads  = peReadCount(4) ;
  long long i_cache_misses = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, peInstructionEventNames, peInstructionEventTitles, 
    5, peInstructionRatioNames, peInstructionRatioTitles
  );

  c->values[0].value = instructions;
  c->values[1].value = cpu_cycles;
  c->values[2].value = branches;
  c->values[3].value = branch_misses;
  c->values[4].value = i_cache_reads;
  c->values[5].value = i_cache_misses;

  c->ratios[0].value = peEventRatio(instructions, cpu_cycles);
  c->ratios[1].value = peEventPTI(branches, instructions);
  c->ratios[2].value = peEventRatio(branch_misses, branches);
  c->ratios[3].value = peEventPTI(i_cache_reads, instructions);
  c->ratios[4].value = peEventRatio(i_cache_misses, i_cache_reads);
}


/*
 * Configure performance counters to measure data access events using the
 * predefined symbolic events. Do not use architecture-specific RAW events.
 */
int peMeasureDataAccessEvents()
{
  // Initialize the event helper module
  peInitialize() ;

  // Make an event group (two events, monitor this PID, monitor all CPUs)
  // Request 6 events, monitor this PID, monitor all CPUs
  if (peMakeGroup(6, 0, -1, 0) == 0) {
    fprintf(stderr, "Couldn't make event group (main)\n") ;
    exit( EXIT_FAILURE ) ;
  }

#if 0
  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  if (peAddLeader(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS) == 0) {
    fprintf(stderr, "Couldn't add event group leader (instructions)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Linux OS event
  if (peAddEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS) == 0) {
    fprintf(stderr, "Couldn't add event to the group (page faults)\n") ;
    exit( EXIT_FAILURE ) ;
  }
#else
  // Add leader event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x08 (instructions retired)
  // if (peAddLeader(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES) == 0) {
  //   fprintf(stderr, "Couldn't add event to the group (cpu cycles)\n") ;
  //   exit( EXIT_FAILURE ) ;
  // }
  if (peAddLeader(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS) == 0) {
    fprintf(stderr, "Couldn't add event group leader (instructions)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Linux OS event
  if (peAddEvent(PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS) == 0) {
    fprintf(stderr, "Couldn't add event to the group (page faults)\n") ;
    exit( EXIT_FAILURE ) ;
  }
  // if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES) == 0) {
  //   fprintf(stderr, "Couldn't add event to the group (cpu cycles)\n") ;
  //   exit( EXIT_FAILURE ) ;
  // }
#endif

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x04 (L1 D-cache access)
  if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES) == 0) {
    fprintf(stderr, "Couldn't add event to the group (L1 D-cache refs)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x03 (L1 D-cache refill)
  if (peAddEvent(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES) == 0) {
    fprintf(stderr, "Couldn't add event to the group (L1 D-cache misses)\n") ;
    exit( EXIT_FAILURE ) ;
  }

  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x50 (L2 cache access read)
  if (peAddEvent(PERF_TYPE_HW_CACHE,
		 (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ<<8) |
		  (PERF_COUNT_HW_CACHE_RESULT_ACCESS<<16))) == 0) {
    fprintf(stderr, "Couldn't add event to the group (L2 cache read)\n") ;
    exit( EXIT_FAILURE ) ;
  }
  // Add event to the group
  // Raspberry Pi 4 (Cortex-A72) event 0x52 (L2 cache refill read)
  if (peAddEvent(PERF_TYPE_HW_CACHE,
		 (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ<<8) |
		  (PERF_COUNT_HW_CACHE_RESULT_MISS<<16))) == 0) {
    fprintf(stderr, "Couldn't add event to the group (L2 cache miss)\n") ;
    exit( EXIT_FAILURE ) ;
  }
  return( 1 ) ;
}

#ifndef PE_EXCLUDE_PRINTS

int pePrintDataAccessEvents(FILE *stream)
{
  // Read instruction event counts
  long long instructions    = peReadCount(0) ;
  //long long page_faults     = peReadCount(1) ;
  long long cpu_cycles      = peReadCount(1) ;
  long long d_cache_reads   = peReadCount(2) ;
  long long d_cache_misses  = peReadCount(3) ;
  long long l2_cache_reads  = peReadCount(4) ;
  long long l2_cache_misses = peReadCount(5) ;

  setlocale(LC_NUMERIC, "") ;
  
  // Print event counts
  fprintf(stream, "Instructions:      %'lld\n", instructions) ;
  //fprintf(stream, "Page faults:       %'lld\n", page_faults) ;
  fprintf(stream, "CPU cycles:     %'lld\n", cpu_cycles) ;
  fprintf(stream, "L1 D-cache reads:  %'lld\n", d_cache_reads) ;
  fprintf(stream, "L1 D-cache misses: %'lld\n", d_cache_misses) ;
  fprintf(stream, "L2 cache reads:    %'lld\n", l2_cache_reads) ;
  fprintf(stream, "L2 cache misses:   %'lld\n", l2_cache_misses) ;

  fprintf(stream, "Page faults (PTI):       %5.3f\n",
	  peEventPTI(page_faults, instructions)) ;
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

static const char* peDataAccessEventNames[] = {
  "instructions",
  "cpu_cycles",
  "d_cache_reads",
  "d_cache_misses",
  "l2_cache_reads",
  "l2_cache_misses",
};

static const char* peDataAccessEventTitles[] = {
  "Instructions",
  "CPU cycles",
  "L1 data cache reads",
  "L1 data cache misses",
  "L2 cache reads",
  "L2 cache read misses",
};

static const char* peDataAccessRatioNames[] = {
  "instructions_per_cycle",
  "l1_dcache_reads_pti",
  "l1_dcache_miss_ratio",
  "l2_cache_reads_pti",
  "l2_cache_miss_ratio"
};

static const char* peDataAccessRatioTitles[] = {
  "Instructions per cycle",
  "L1 D-cache reads (PTI)",
  "L1 D-cache miss ratio",
  "L2 cache reads (PTI)",
  "L2 cache miss ratio"
};

extern void peCollectDataAccessEvents(pe_event_collection* c) {
  pe_event_collection_clear(c);

  // Read instruction event counts
  long long instructions    = peReadCount(0) ;
  long long cpu_cycles      = peReadCount(1) ;
  long long d_cache_reads   = peReadCount(2) ;
  long long d_cache_misses  = peReadCount(3) ;
  long long l2_cache_reads  = peReadCount(4) ;
  long long l2_cache_misses = peReadCount(5) ;

  pe_event_collection_populate(c, 
    6, peDataAccessEventNames, peDataAccessEventTitles, 
    5, peDataAccessRatioNames, peDataAccessRatioTitles
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


//
// CPU time measurement using clock()
//
void start_clock()
{
  starting_clock_time = ((double)clock() / (double)CLOCKS_PER_SEC) ;
}

double get_clock_time()
{
  return( ((double)clock() / (double)CLOCKS_PER_SEC) - starting_clock_time ) ;
}

void print_clock_time(FILE *stream, double time_in_seconds)
{
  double resolution ;
  resolution = 1.0 / CLOCKS_PER_SEC ;

  fprintf(stream,
	  "Clock() time: %9.3f sec (%8.6f resolution)\n",
	  time_in_seconds, 
	  resolution) ;
}


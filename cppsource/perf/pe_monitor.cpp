#include "perf/pe_monitor.h"

#include <stdio.h>
#if defined(_MSC_VER)
#include <wunistd.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#endif
#include <string.h>

#if (MACHINE == x86_64)  
// All event definitions taken from kernel files 
#else
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
#endif

#define PEMI_MAX PEM_MAX_EVENTS
static int pemi_add_leader(int type, int config);
static int pemi_add_item(int type, int config);
static int pemi_start();
static int pemi_stop();
static long long pemi_read(int eindex);

long long g_totals[PEMI_MAX] = { 0, 0, 0, 0, 0, 0 };

typedef struct tag_pem_descriptor {
  int type, config;
  const char* name;
} pem_descriptor;

pem_descriptor descriptors0[] = {
#if (MACHINE == x86_64)  
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "cpu_cycles" },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "instructions" },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, "d_cache_reads" },
  { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, "d_cache_misses" },
  // { PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES, "cpu_cycles_unscaled" },
  // Note - adding the descriptors below causes failure - all counts return 0.
  // This is probably due to a mixture of types without a corresponding PERF_FORMAT_GROUP flag.
  // See perhaps https://stackoverflow.com/a/42092180
  // { PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL 
  //                     | (PERF_COUNT_HW_CACHE_OP_READ<<8) 
  //                     | (PERF_COUNT_HW_CACHE_RESULT_ACCESS<<16), "ll_cache_reads" },
  // { PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_LL 
  //                     | (PERF_COUNT_HW_CACHE_OP_READ<<8) 
  //                     | (PERF_COUNT_HW_CACHE_RESULT_MISS<<16), "ll_cache_misses" },
#else
  { PERF_TYPE_RAW, A72_CPU_CYCLES, "cpu_cycles" },
  { PERF_TYPE_RAW, A72_INST_RETIRED, "instructions" },
  { PERF_TYPE_RAW, A72_L1D_CACHE_ACCESS, "d_cache_reads" },
  { PERF_TYPE_RAW, A72_L1D_CACHE_REFILL, "d_cache_misses" },
#endif
};
int descriptors0_count = sizeof(descriptors0)/sizeof(pem_descriptor);

int g_pem_descriptor_count = 0;
const pem_descriptor* g_pem_descriptors = 0;

int pem_setup(int which) {
  switch (which) {
    case 0:
      g_pem_descriptor_count = descriptors0_count;
      g_pem_descriptors = descriptors0;
      break;
    default:
      return 0;
  }
  return g_pem_descriptor_count;
}
int pem_before_start() {
  for (int i = 0; i < g_pem_descriptor_count; i++) {
    if (i == 0) {
      if (!pemi_add_leader(g_pem_descriptors[i].type, g_pem_descriptors[i].config)) {
        return 0;
      }
    }
    else {
      if (!pemi_add_item(g_pem_descriptors[i].type, g_pem_descriptors[i].config)) {
        return 0;
      }
    }
  }
  return 1;
}
int pem_get_descriptor(int i, int* type, int* config, const char** name) {
  if (i >= 0 && i < g_pem_descriptor_count) {
    const pem_descriptor* d = g_pem_descriptors + i;
    *type = d->type;
    *config = d->config;
    *name = d->name;
    return 1;
  }
  else {
    return 0;
  }
}

int pem_start(void) { 
  if (!pem_before_start())
    return 0;
  return pemi_start(); 
}
int pem_stop(void) { 
  int ok = pemi_stop(); 
  if (ok) {
    for (int i = 0; i < g_pem_descriptor_count; i++) {
      g_totals[i] = pemi_read(i);
    }
  }
  else {
    for (int i = 0; i < g_pem_descriptor_count; i++) {
      g_totals[i] = -1;
    }
  }
  return ok;
}

int pem_read_totals(int count, long long* buffer) {
  if (count == g_pem_descriptor_count) {
    memcpy(buffer, g_totals, sizeof(long long) * count);
    return 1;
  }
  return 0;
}

///////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}

static int g_group_fd = 0;
static int g_fd[PEMI_MAX] = { 0, 0, 0, 0, 0, 0 }; 
static int g_fd_count = 0;

void pemi_clean_up() {
  if (g_fd_count) {
    for (int i = 0; i < g_fd_count; i++) {
      close(g_fd[i]);
      g_fd[i] = 0;
    }
    g_fd_count = 0;
  }
}

int pemi_add_leader(int type, int config) {
  struct perf_event_attr pe;
  int fd;

  pemi_clean_up();

  memset(&pe, 0, sizeof(pe));
  pe.type = type;
  pe.size = sizeof(pe);
  pe.config = config;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening leader %llx\n", pe.config);
    perror("pemi_add_leader");
    return 0;
  }

  g_group_fd = g_fd[g_fd_count++] = fd;
  return 1;
}

int pemi_add_item(int type, int config) {
  struct perf_event_attr pe;
  int fd;

  memset(&pe, 0, sizeof(pe));
  pe.type = type;
  pe.size = sizeof(pe);
  pe.config = config;
  pe.disabled = 0;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0, -1, g_group_fd, 0);
  if (fd == -1) {
    fprintf(stderr, "Error opening event %llx\n", pe.config);
    perror("pemi_add_item");
    return 0;
  }

  g_fd[g_fd_count++] = fd;
  return 1;
}

int pemi_start()
{
  if (ioctl(g_group_fd, PERF_EVENT_IOC_RESET, 0) < 0) {
    perror("pemi_start") ;
    fprintf(stderr, "Error when resetting event counters\n") ;
    return( 0 ) ;
  }
  if (ioctl(g_group_fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
    perror("pemi_start") ;
    fprintf(stderr, "Error when starting event counters\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

int pemi_stop()
{
  if (ioctl(g_group_fd, PERF_EVENT_IOC_DISABLE, 0) < 0) {
    perror("pemi_stop") ;
    fprintf(stderr, "Error when stopping event counters\n") ;
    return( 0 ) ;
  }
  return( 1 ) ;
}

long long pemi_read(int eindex)
{
  long long count ;
  int fd ;

  if ((eindex < 0) || (eindex >= g_fd_count)) {
    fprintf(stderr, "Invalid event index %d (pemi_read)\n", eindex) ;
    return( 0 ) ;
  }

  fd = g_fd[eindex] ;
  if (read(fd, &count, sizeof(long long)) < 0) {
    perror("pemi_read") ;
    fprintf(stderr, "Error when reading event count %d\n", eindex) ;
    return( 0 ) ;
  }
  return( count ) ;
}

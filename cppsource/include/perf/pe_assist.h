
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

#ifndef _PE_ASSIST_H
#define _PE_ASSIST_H

#include <stdlib.h>
#include <stdio.h>
#if defined(_MSC_VER)
#include "wunistd.h"
#else
#include <unistd.h>
#endif
#include <time.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#if defined(_MSC_VER)
typedef int pid_t;
#else
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#endif

#define  PE_MAX_GROUPS  1
#define  PE_MAX_EVENTS  6

/*
 * Exported functions
 */
extern int peInitialize() ;
extern int peMakeGroup(int num, pid_t pid, int cpu, unsigned long flags) ;
extern int peAddLeader(long unsigned etype, long unsigned econfig) ;
extern int peAddEvent(long unsigned etype, long unsigned econfig) ;
extern int peStartCounting() ;
extern int peStopCounting() ;
extern int peResetCounters() ;
extern long long peReadCount(int eindex) ;
extern double peEventRatio(long long num, long long den) ;
extern double peEventPTI(long long num, long long instructions) ;
extern void pePrintCount(FILE* stream, int eindex, char *legend) ;

extern int peMeasureInstructionEvents() ;
extern int pePrintInstructionEvents() ;

extern int peMeasureDataAccessEvents() ;
extern int pePrintDataAccessEvents() ;

// CPU time measurement using clock()
extern void start_clock() ;
extern double get_clock_time() ;
extern void print_clock_time(FILE *stream, double time_in_seconds) ;

typedef struct pe_event_value_tag {
  const char* name;  
  const char* title;  
  long long value;
} pe_event_value;

typedef struct pe_event_ratio_tag {
  const char* name;  
  const char* title;  
  double value;
} pe_event_ratio;

typedef struct pe_extracted_value_tag {
	long long int v;
	double r;
	int which;
} pe_extracted_value;

#define PE_MAX_EVENT_VALUES  6
#define PE_MAX_EVENT_RATIOS  5

typedef struct pe_event_collection_tag {
  int values_populated, ratios_populated;
  pe_event_value values[PE_MAX_EVENT_VALUES];
  pe_event_ratio ratios[PE_MAX_EVENT_RATIOS];
} pe_event_collection;

typedef struct tag_pe_event_collection_sum {
  const char* name;
  size_t count;
  pe_event_collection data;
} pe_event_collection_sum;

typedef struct tag_pe_event_collection_sum_set {
  size_t count;
  pe_event_collection_sum* items;
} pe_event_collection_sum_set;

extern void pe_event_collection_clear(pe_event_collection* c);
extern void pe_event_collection_populate(pe_event_collection* c,
  size_t values_count, const char* value_names[], const char* value_titles[], 
  size_t ratios_count, const char* ratio_names[], const char* ratio_titles[]);

extern pe_extracted_value pe_extract_summary_value(const pe_event_collection* ec, 
	const char* field_name, size_t count);

extern void pe_event_collection_sum_set_init(pe_event_collection_sum_set* ss, 
  size_t count, const char* sum_names[]);
extern void pe_event_collection_sum_set_term(pe_event_collection_sum_set* ss);
extern int pe_event_collection_sum_set_find_index(pe_event_collection_sum_set* ss, 
  const char* sum_name);
void pe_event_collection_sum_set_append(pe_event_collection_sum_set* ss,  
  const char* sum_name, const pe_event_collection* ec);

extern void peCollectDataAccessEvents(pe_event_collection* c);

#endif // _PE_ASSIST_H

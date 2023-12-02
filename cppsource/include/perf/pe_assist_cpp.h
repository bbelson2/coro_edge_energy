
/*
 * Performance event assistance functions
 */

#ifndef _PE_ASSIST_CPP_H
#define _PE_ASSIST_CPP_H

#ifndef __cplusplus
#error pe_assist_cpp.h requires C++
#endif

#include <perf/pe_assist.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

void pe_report_summaries(std::ostream& os, const pe_event_collection_sum_set& ss, 
  size_t summary_count, const char* summary_names[], const std::vector<std::string>& field_list);

#endif // _PE_ASSIST_CPP_H

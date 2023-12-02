#pragma once
#ifndef __PE_CONTEXT_H__
#define __PE_CONTEXT_H__

#include <iostream>
#include <fstream>
#include "pe_monitor.h"
#include "pe_monitor_coll.h"
#include "perf_rec.h"
#include "pe_summaries.h"

template<typename PARAMS_T>
class context_t {
public:
  typedef PARAMS_T params_t; 
  context_t(const params_t& params) 
  : params(params),
    duration(0), pof(nullptr), pem_statistic_count(0) {

  }
public:
  const params_t& params;
  pem_test pem_summaries[EMI_COUNT];
  intmax_t duration;
  long long pem_data[PEM_MAX_EVENTS];
  
  std::ofstream of_output;
  std::ostream *pof;

  int pem_statistic_count;

public:
  inline void init_output_stream()
  {
    pof = get_output_stream(params, of_output);
  }

  inline void report_line_header() 
  {
    if (!params.omit_header) {
      ::report_line_header(params, pof, pem_summaries[0]);
    }
  }

  inline void report_line_values(size_t control_model, size_t coroutines, bool pf) 
  {
    ::report_line_values(params, control_model, pof, coroutines, pf, duration, 
      pem_statistic_count, pem_data);
  }

  inline void init_telemetry(int which_setup) 
  { 
    pem_statistic_count = pem_setup(which_setup);
    ::pe_summaries_init(pem_statistic_count, pem_summaries);
  }

  template <typename FP_T>
  void perf_record(size_t which_summary, FP_T fp) {
    ::perf_record(params.telemetry, pem_summaries + which_summary, duration, pem_data, fp);
  }

  // Telemetry
  void report_telemetry_summaries(std::ostream& os)
  {
    if (params.telemetry)
    {
      if (params.horizontal_telemetry)
      {
        if (params.group_by_metric) 
        {
          pe_summaries_report_horizontal_group_by_metric(params, 
            pem_statistic_count, pem_summaries, 
            split(params.summaries, ','), params.sep, os);

        }
        else
        {
          pe_summaries_report_horizontal_group_by_test(params, 
            pem_statistic_count, pem_summaries, 
            split(params.summaries, ','), params.sep, os);
        }
      }
      else
      {
        pe_summaries_report_vertical(
          pem_statistic_count, pem_summaries,
          split(params.summaries, ','), params.sep, os);
      }
    }
  }

  void before_test()
  {
    if (params.flood)
    {
      ::clear_cache();
    }
  }

protected:
  // Telemetry
};

#endif // __PE_CONTEXT_H__

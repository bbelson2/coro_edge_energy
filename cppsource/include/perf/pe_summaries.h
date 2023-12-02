#pragma once
#ifndef __PE_SUMMARIES_H__
#define __PE_SUMMARIES_H__

#include "perf/pe_monitor.h"
#include "perf/pe_monitor_coll.h"
#include <cassert>

enum EMI_SUMMARIES
{
  EMI_SEQ_START = 0,
  EMI_CORO,
  EMI_SEQ_END,

  EMI_COUNT
};

const char** pe_summary_get_names() {
  static const char* emi_summary_names[] = {
    "seq_start",
    "coro",
    "seq_end",
    0
  };
  return emi_summary_names;
}

void pe_summaries_init(int pem_statistic_count, pem_test* pem_summaries)
{
  for (int i = 0; i < EMI_COUNT; i++)
  {
    pem_summaries[i].init(pem_statistic_count);
  }
}

#ifdef PE_INCLUDE_REPORTS
void pe_summaries_report_vertical(
  int pem_statistic_count, const pem_test* pem_summaries, 
  const std::vector<std::string> &field_list, const std::string& sep,
  std::ostream &os)
{
  assert(pem_statistic_count == (int)field_list.size());
  if (field_list.size() > 0)
  {
    // Header line 
    os << "test" << sep << join(field_list, sep) << std::endl;
    // One line for each summary
    for (size_t s = 0; s < EMI_COUNT; s++)
    {
      os << summary_names[s];
      for (size_t i = 0; i < field_list.size(); i++)
      {
        os << sep << (long long)pem_summaries[s].extract_value(field_list[i].c_str());
      }
      os << std::endl;
    }
  }
}

template<typename PARAMS_T>
void pe_summaries_report_horizontal_group_by_test(
  const PARAMS_T &params,
  int pem_statistic_count, const pem_test* pem_summaries, 
  const std::vector<std::string> &field_list, const std::string& sep,
  std::ostream &os)
{
  assert(pem_statistic_count == (int)field_list.size());
  // Skip the header line?
  if (params.horizontal_telemetry_header)
  {
    // params headers
    os << join(params.report_summary_headers, sep);
    // summary headers
    for (size_t s = 0; s < EMI_COUNT; s++)
    {
      for (auto f : field_list)
      {
        os << sep << summary_names[s] << "." << f;
      }
    }
    os << std::endl;
  }
  // params values
  params.report_summary_values(os);
  // summary values
  for (size_t s = 0; s < EMI_COUNT; s++)
  {
    for (auto f : field_list)
    {
      os << sep << (long long)pem_summaries[s].extract_value(f.c_str());
    }
  }
  os << std::endl;
}

template<typename PARAMS_T>
void pe_summaries_report_horizontal_group_by_metric(
  const PARAMS_T &params,
  int pem_statistic_count, const pem_test* pem_summaries, 
  const std::vector<std::string> &field_list, const std::string& sep,
  std::ostream &os)
{
  assert(pem_statistic_count == (int)field_list.size());
  // Skip the header line?
  if (params.horizontal_telemetry_header)
  {
    // params headers
    os << join(params.report_summary_headers, sep);
    // summary headers
    for (auto f : field_list)
    {
      for (size_t s = 0; s < EMI_COUNT; s++)
      {
        os << sep << summary_names[s] << "." << f;
      }
    }
    os << std::endl;
  }
  // params values
  params.report_summary_values(os);
  // summary values
  for (auto f : field_list)
  {
    for (size_t s = 0; s < EMI_COUNT; s++)
    {
      os << sep << (long long)pem_summaries[s].extract_value(f.c_str());
    }
  }
  os << std::endl;
}

#endif

#endif // __PE_SUMMARIES_H__

#pragma once

#ifndef __PERF_REC_H__
#define __PERF_REC_H__

#include "perf/pe_monitor_coll.h"


// Timer
#include <chrono>
typedef std::chrono::steady_clock the_timer_t;
inline intmax_t get_duration_us(the_timer_t::time_point t1, the_timer_t::time_point t2)
{
  return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
}

// RAII wrapper class for perf with timer
class perf_recorder
{
public:
  perf_recorder(
      bool use_telemetry,
      pem_test *pem_summary,
      intmax_t &duration,
      long long* pem_data,
      bool auto_start = true)
      : use_telemetry_(use_telemetry), pem_summary_(pem_summary), duration_(duration), pem_data_(pem_data),
        start_time_(), started_(false)
  {
    if (auto_start)
    {
      start();
    }
  }
  ~perf_recorder()
  {
    if (started_)
    {
      stop();
    }
  }
  inline void start()
  {
    start_time_ = the_timer_t::now();
    if (use_telemetry_)
    {
      pem_start();
    }
    started_ = true;
  }
  inline void stop()
  {
    if (use_telemetry_)
    {
      pem_stop();
    }
    auto end_time = the_timer_t::now();
    duration_ = get_duration_us(start_time_, end_time);
    if (use_telemetry_)
    {
      pem_read_totals(pem_summary_->size(), pem_data_);
      pem_summary_->add(pem_summary_->size(), pem_data_);
    }
    started_ = false;
  }

public:
protected:
  bool use_telemetry_;
  pem_test *pem_summary_;
  intmax_t &duration_;
  long long* pem_data_;
  the_timer_t::time_point start_time_;
  bool started_;
};

template <typename FP_T>
void perf_record(bool use_telemetry,
                 pem_test *pem_summary, intmax_t &duration,
                 long long* pem_data,
                 FP_T fp)
{
  perf_recorder recorder(use_telemetry, pem_summary, duration, pem_data);
  fp();
}

#endif // __PERF_REC_H__

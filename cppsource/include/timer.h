
#pragma once

#include <chrono>

////////////////////////////////////////////////////////////////
// Timer
////////////////////////////////////////////////////////////////

class NanoTimer {
public:
  typedef std::chrono::high_resolution_clock Time;
  typedef std::chrono::time_point<Time> timestamp_t;
  typedef unsigned long long timeres_t;
public:
  NanoTimer() {
    reset();
  }
  void reset() {
    start_time = Time::now();
  }
  timeres_t get_timestamp() {
    std::chrono::duration<timeres_t, std::nano> t = Time::now() - start_time;
    return t.count();
  }
  // Busy waits
  static void wait(timeres_t wait_ns) {
    timestamp_t start = Time::now();
    while (true) {
      std::chrono::duration<timeres_t, std::nano> d = Time::now() - start;
      if (d.count() >= wait_ns) {
        break;
      }
    }
  }
  timeres_t wait_until(timeres_t wait_until_ns) {
    timeres_t t;
    while ((t = get_timestamp()) < wait_until_ns) {
    }
    return t;
  }
private:
  timestamp_t start_time;
};


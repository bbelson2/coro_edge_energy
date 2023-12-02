#pragma once

#include <string>
#include <ostream>
#include <sstream>
#include <vector>
#include <fpm/ios.hpp>

template <typename I>
void dump_vector(const I& b, const I& e, std::ostream &os, std::string label = "", size_t max_width = 64U)
{
  int chars_this_line = 0;
  if (!label.empty()) {
    os << label << " = ";
    chars_this_line += label.size() + 3;
  }
  os << "[";
  chars_this_line++;
  for (I i = b; i != e; i++) {
    std::ostringstream ss;
    ss << " " << *i;
    std::string s = ss.str();
    if (s.size() + chars_this_line > max_width) {
      os << std::endl;
      chars_this_line = 0;
    }
    os << s;
    chars_this_line += s.size();
  }
  os << " ]" << std::endl;
}

template <typename T>
inline void dump_vector(const std::vector<T> &v, std::ostream &os, std::string label = "", size_t max_width = 64U)
{
  dump_vector(v.begin(), v.end(), os, label, max_width);
}

#if __linux__
#include <unistd.h>
inline static std::string get_self_path()
{
  char self[PATH_MAX] = { 0 };
  int nchar = readlink("/proc/self/exe", self, sizeof self);
  if (nchar < 0 || (size_t)nchar >= sizeof(self))
    return "";
  return std::string(self);
}
#endif

#if false
// Hangs in WSL2
#include <thread>
#include <chrono>
void wait_us(uint32_t wait) {
  std::this_thread::sleep_for(std::chrono::microseconds(wait));
}
#endif
#if false
// Very slow in WSL2
#include <time.h>
void wait_us(uint32_t wait) {
  struct timespec req = { 0, (__syscall_slong_t)wait * 1000 };
  clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
}
#endif
#if true
//https://stackoverflow.com/a/14391562
#include <chrono>
void wait_us(uint32_t wait) {
  typedef std::chrono::high_resolution_clock Time;
  typedef std::chrono::duration<float> fsec;

  float fwait = (float)wait / 1000000.0;
  auto t0 = Time::now();
  while (true) {
    auto t1 = Time::now();
    fsec fs = t1 - t0;
    if (fs.count() >= fwait) {
      break;
    }
  }
}
#endif

#include <iostream>

template<typename ITEM_T>
inline bool check_results(const std::vector<std::vector<ITEM_T> >& expected, 
    const std::vector<std::vector<ITEM_T> >& actual, const char* data_name,
   [[maybe_unused]] bool verbose = false)
{
  if (actual != expected)
  {
    std::cerr << "inconsistent results for " << data_name << std::endl;
    for (size_t i = 0; i < expected.size(); i++) {
      if (actual[i] != expected[i]) {
        std::cerr << "Error in vector #" << i << std::endl;
        const std::vector<ITEM_T>& a = actual[i];
        const std::vector<ITEM_T>& e = expected[i];
        for (size_t j = 0; j < a.size(); j++) {
          if (a[j] != e[j]) {
              std::cerr << "Error at #" << j << ": " << a[j] << " != " << e[j] << std::endl;
          }
        }
      }
    }
    return false;
  }
  return true;
}


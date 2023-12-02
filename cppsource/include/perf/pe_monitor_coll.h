#pragma once
#ifndef __PE_MONITOR_COLL_H__
#define __PE_MONITOR_COLL_H__

#include "perf/pe_monitor.h"
#include <vector>
#include <string>
#include <exception>
#include <cstring>
#include <stdexcept>

struct pem_desc {
  public:
    int type, config;
    std::string name;
  public:
    pem_desc() {
      type = 0;
      config = 0;
    }
    void populate(int which) {
      const char* pch = 0;
      if (!pem_get_descriptor(which, &type, &config, &pch)) {
        throw std::out_of_range("which");
      }
      name = pch;
    }
};

class pem_test {
  public:
    std::vector<pem_desc> descriptors;
    std::vector<long long> counts;
    size_t sample_count;
  public:
    pem_test() : sample_count(0) {}
    size_t size() const { return descriptors.size(); }
    void init(int count) {
      descriptors.clear();
      for (int which = 0 ; which < count; which++) {
        pem_desc desc;
        desc.populate(which);
        descriptors.push_back(desc);
      }
      clear();
    }
    void add(int count, const long long* data) {
      if (count != (int)size()) {
        throw std::out_of_range("count");        
      }
      for (int i = 0; i < (int)size(); i++) {
        counts[i] += data[i];
      }
      sample_count++;
    }
    void clear() {
      counts.clear();
      counts.resize(descriptors.size(), 0);
      sample_count = 0;
    }
    int index_of(const char* name) const {
      for (size_t i = 0; i < size(); i++) {
        if (!strcmp(name, descriptors[i].name.c_str())) {
          return (int)i;
        }
      }
      return -1;
    }
    long long extract_value(int which) const {
      if (which < 0 || which > (int)size()) {
        throw std::out_of_range("which");
      }
      if (sample_count == 0) {
        //throw std::overflow_error("which");
        return 0;
      }
      return (long long)((double)counts[which] / (double) sample_count);
    }
    long long extract_value(const char* name) const {
      int index = index_of(name);
      if (index < 0) {
        throw std::out_of_range("name");
      }
      return extract_value(index);
    }
};

#endif // __PE_MONITOR_COLL_H__

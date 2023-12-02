#pragma once
#ifndef __BOUNDED_RANDOM_H__
#define __BOUNDED_RANDOM_H__

#include <random>
#include <numeric>

template<typename DATA_T>
DATA_T from_float(float f)
{
  return DATA_T(f);
}

#if USE_FPM==0
template<>
fix_t from_float(float f)
{
  return fix_float_to_int(f);
}
#endif

/**
 * @brief Transforms the outut of a uniform random distribution to be 
 * bounded between min_value and max_value, and capable of taking on 
 * any of granularity values between the two inclusive values.
 * 
 */
struct rnd_bounds {
  float min_value, max_value;
  uint32_t granularity;
  bool is_valid() const {
    return (granularity != 0)
        && (max_value >= min_value);
  }
};

// #ifdef _GLIBCXX_OSTREAM
std::ostream& operator<<(std::ostream& os, const rnd_bounds& item) {
  os << "[" << item.min_value << " to " << item.max_value << " / " << item.granularity << "]";
  return os;
}
// #endif

template<typename DATA_T>
class bounded_distribution {
public:
  bounded_distribution(const rnd_bounds& bounds)
  : distribution_(0, bounds.granularity - 1), bounds_(bounds)
  {
    mult_ = (bounds.max_value - bounds.min_value) / bounds.granularity;
  }
  template<typename ENGINE_T>
  DATA_T operator()(ENGINE_T& engine) {
    return from_float<DATA_T>(bounds_.min_value + ((float)distribution_(engine) * mult_));
  }
private:
  std::uniform_int_distribution<int> distribution_;
  float mult_;
  rnd_bounds bounds_;
};

#endif // __BOUNDED_RANDOM_H__

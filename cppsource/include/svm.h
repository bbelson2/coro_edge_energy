#ifndef SVM_H
#define SVM_H

// #ifndef FPM_FIXED_HPP
// #error svm.h requires fpm/fixed.hpp
// #endif

template<typename T> 
T mult_op(const T& x, const T& y) { return x * y; }

#ifdef __FIXED_POINT_H__
template<> 
fix_t mult_op<fix_t>(const fix_t& x, const fix_t& y) { return fix_mul(x, y); }
#endif // __FIXED_POINT_H__


/**
 * @brief 
 * 
 * @tparam T 
 * @param weights 
 * @param values 
 * @param bias 
 * @param count 
 * @return true 
 * @return false 
 */
template<typename T>
bool svm_infer(const T* weights, const T* values, T bias, size_t count)
{
  T total = T();
  for (size_t i = 0; i < count; i++, values++, weights++)
  {
    // total += *values * *weights;
    total += mult_op(*values, *weights);
  }
  return total > bias;
}

#endif // SVM_H

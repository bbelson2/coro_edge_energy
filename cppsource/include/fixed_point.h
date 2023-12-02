/**
 * @file fixed_point.h
 * @author Bruce Belson (bruce.belson@jcu.edu.au)
 * @brief Conversion and simple arithmetic functions for fixed-point
 * numbers with 13 fractional places. 
 * @version 0.1
 * @date 2022-02-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __FIXED_POINT_H__
#define __FIXED_POINT_H__

#include <math.h>
#include <vector>

#ifndef FIXED_POINT_BITS
#define FIXED_POINT_BITS 16
#endif //  FIXED_POINT_BITS

#if ((FIXED_POINT_BITS != 16) && (FIXED_POINT_BITS != 32))
#error FIXED_POINT_BITS must be 16 or 32
#endif

#if FIXED_POINT_BITS == 16

/**
 * @brief A fixed-point number with 13 fractional places 
 * 
 */
typedef int16_t fix_t;
/**
 * @brief The temporary wide form of fix_t
 * 
 */
typedef int32_t wide_t;

/**
 * @brief The number of fixed point fractional places (13)
 * 
 */
#define FIX_FRAC_PLACES 13
#define FIX_SCALING_FACTOR (1 << FIX_FRAC_PLACES)

#endif

#if FIXED_POINT_BITS == 32

/**
 * @brief A fixed-point number with 13 fractional places 
 * 
 */
typedef int32_t fix_t;
/**
 * @brief The temporary wide form of fix_t
 * 
 */
typedef int64_t wide_t;

/**
 * @brief The number of fixed point fractional places (13)
 * 
 */
#define FIX_FRAC_PLACES 26
#define FIX_SCALING_FACTOR (1 << FIX_FRAC_PLACES)

#endif

/**
 * @brief Converts a 4-byte float to a 2-byte fixed-point number 
 * with 13 fractional places  
 * 
 * @param x is a floating point value
 * @return fix_t x as a fixed point integer with 13 fractional places  
 */
inline fix_t fix_float_to_int(float x)
{
  return (fix_t)round(x * (float)FIX_SCALING_FACTOR);
}

/**
 * @brief Converts a 2-byte fixed-point number with 13 fractional 
 * places to a 4-byte floating-point number  
 * 
 * @param n 2-byte fixed-point number with 13 fractional places
 * @return float n as a floating-point number
 */
inline float fix_int_to_float(fix_t n)
{
  return (float)n / (float)FIX_SCALING_FACTOR;
}

/**
 * @brief Calculates the product of two fixed-point numbers 
 * 
 * @param x is a 2-byte fixed-point number with 13 fractional places
 * @param y is a 2-byte fixed-point number with 13 fractional places
 * @return fix_t the product as a 2-byte fixed-point number with 13 fractional places
 */
inline fix_t fix_mul(fix_t x, fix_t y)
{
  wide_t product = (wide_t)x * y;
  wide_t sh_product = product >> FIX_FRAC_PLACES;
  fix_t narrow_product = (fix_t)sh_product;
  return narrow_product;
}

/**
 * @brief Calculates the quotient of two fixed-point numbers 
 * 
 * @param x is a 2-byte fixed-point number with 13 fractional places
 * @param y is a 2-byte fixed-point number with 13 fractional places
 * @return fix_t the quotient as a 2-byte fixed-point number with 13 fractional places
 */
inline fix_t fix_div(fix_t x, fix_t y)
{
  wide_t dividend = (wide_t)x << FIX_FRAC_PLACES; 
  wide_t quotient = dividend / (wide_t)y;
  fix_t narrow_quotient = (fix_t)quotient;
  return narrow_quotient;
}

inline std::vector<float> fix_int_to_float(const std::vector<fix_t> vin) {
  std::vector<float> vout(vin.size());
  for (size_t i = 0; i < vin.size(); i++) {
    vout[i] = fix_int_to_float(vin[i]);
  }
  return vout;
}

#endif // __FIXED_POINT_H__
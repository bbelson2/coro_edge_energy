#include <fpm/fixed.hpp>
#include "svm.h"
#include <gtest/gtest.h>
#include <vector>


TEST(SVM, Basic_13_u16) {
  using P = fpm::fixed<std::int16_t, std::int32_t, 13>;
  std::vector<P> w{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<P> x{P(1.0f), P(1.0f), P(1.0f)};
  EXPECT_EQ((svm_infer(w.data(), x.data(), P(2.9f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x.data(), P(3.1f), 3)), false);
}

TEST(SVM, Signed_13_u16) {
  using P = fpm::fixed<std::int16_t, std::int32_t, 13>;
  std::vector<P> w{P(-1.0f), P(1.0f), P(-1.0f)};
  std::vector<P> x1{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<P> x2{P(-1.0f), P(-1.0f), P(-1.0f)};
  EXPECT_EQ((svm_infer(w.data(), x1.data(), P(-1.1f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x1.data(), P(-0.9f), 3)), false);
  EXPECT_EQ((svm_infer(w.data(), x2.data(), P(0.9f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x2.data(), P(1.1f), 3)), false);
}

TEST(SVM, Eq_13_u16) {
  using P = fpm::fixed<std::int16_t, std::int32_t, 13>;
  std::vector<P> w{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<P> x{P(1.0f), P(1.0f), P(0.0f)};
  std::vector<P> xp{P(1.0f), P(1.0f), P(0.0001f)};
  std::vector<P> xn{P(1.0f), P(1.0f), P(-0.0001f)};
  EXPECT_EQ((svm_infer(w.data(), x.data(),  P(2.0f), 3)), false); // EQ
  EXPECT_EQ((svm_infer(w.data(), xp.data(), P(2.0f), 3)), true ); // GT
  EXPECT_EQ((svm_infer(w.data(), xn.data(), P(2.0f), 3)), false); // LT
}


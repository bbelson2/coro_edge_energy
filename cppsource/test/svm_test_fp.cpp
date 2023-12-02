#include <fixed_point.h>
#include "svm.h"
#include <gtest/gtest.h>
#include <vector>

#define P fix_float_to_int

TEST(SVM_fp, Basic_fix_t) {
  std::vector<fix_t> w{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<fix_t> x{P(1.0f), P(1.0f), P(1.0f)};
  EXPECT_EQ((svm_infer(w.data(), x.data(), P(2.9f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x.data(), P(3.1f), 3)), false);
}

TEST(SVM_fp, Signed_fix_t) {
  std::vector<fix_t> w{P(-1.0f), P(1.0f), P(-1.0f)};
  std::vector<fix_t> x1{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<fix_t> x2{P(-1.0f), P(-1.0f), P(-1.0f)};
  EXPECT_EQ((svm_infer(w.data(), x1.data(), P(-1.1f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x1.data(), P(-0.9f), 3)), false);
  EXPECT_EQ((svm_infer(w.data(), x2.data(), P(0.9f), 3)), true);
  EXPECT_EQ((svm_infer(w.data(), x2.data(), P(1.1f), 3)), false);
}

TEST(SVM_fp, Eq_fix_t) {
  std::vector<fix_t> w{P(1.0f), P(1.0f), P(1.0f)};
  std::vector<fix_t> x{P(1.0f), P(1.0f), P(0.0f)};
  std::vector<fix_t> xp{P(1.0f), P(1.0f), P(0.0001f)};
  std::vector<fix_t> xn{P(1.0f), P(1.0f), P(-0.0001f)};
  EXPECT_EQ((svm_infer(w.data(), x.data(),  P(2.0f), 3)), false); // EQ
  EXPECT_EQ((svm_infer(w.data(), xp.data(), P(2.0f), 3)), true ); // GT
  EXPECT_EQ((svm_infer(w.data(), xn.data(), P(2.0f), 3)), false); // LT
}


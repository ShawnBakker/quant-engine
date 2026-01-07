#include <cmath>
#include <limits>
#include <vector>

#include "qe/indicators.hpp"
#include "qe/data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

static bool approx(double a, double b, double eps = 1e-12) {
  return std::fabs(a - b) <= eps;
}

static bool is_nan(double x) {
  return std::isnan(x);
}

TEST_CASE("compute_returns: close-to-close simple returns") {
  // basic “happy path” sanity: close-to-close returns for a tiny series
  qe::OhlcvTable t;
  t.push_back({"t0", 0,0,0, 100.0, 0});
  t.push_back({"t1", 0,0,0, 110.0, 0});
  t.push_back({"t2", 0,0,0,  99.0, 0});

  std::vector<double> r = qe::compute_returns(t);
  REQUIRE(r.size() == 2);

  REQUIRE(approx(r[0], 0.10));
  REQUIRE(approx(r[1], (99.0 - 110.0) / 110.0));
}

TEST_CASE("compute_returns: empty or too-short input returns empty") {
  // edge case: with < 2 rows, there are no returns to compute
  qe::OhlcvTable empty;
  qe::OhlcvTable one_row;
  one_row.push_back({"t0", 0,0,0, 100.0, 0});

  REQUIRE(qe::compute_returns(empty).empty());
  REQUIRE(qe::compute_returns(one_row).empty());
}

TEST_CASE("compute_returns: prev close == 0 produces NaN") {
  // edge case: return would divide by zero; we expect NaN as a signal value
  qe::OhlcvTable t;
  t.push_back({"t0", 0,0,0, 0.0, 0});
  t.push_back({"t1", 0,0,0, 10.0, 0});

  std::vector<double> r = qe::compute_returns(t);
  REQUIRE(r.size() == 1);
  REQUIRE(is_nan(r[0]));
}

TEST_CASE("rolling_mean: NaN until window is filled") {
  //  window=3, first two positions cannot have a full window => NaN
  std::vector<double> v{1.0, 2.0, 3.0, 4.0};
  auto m = qe::rolling_mean(v, 3);

  REQUIRE(m.size() == v.size());
  REQUIRE(is_nan(m[0]));
  REQUIRE(is_nan(m[1]));
  REQUIRE(approx(m[2], 2.0)); // (1+2+3)/3
  REQUIRE(approx(m[3], 3.0)); // (2+3+4)/3
}

TEST_CASE("rolling_mean: window=1 should match the original series") {
  // Window=1 is a nice sanity check: each mean is the value itself
  std::vector<double> v{5.0, -2.0, 7.5};
  auto m = qe::rolling_mean(v, 1);

  REQUIRE(m.size() == v.size());
  REQUIRE(approx(m[0], v[0]));
  REQUIRE(approx(m[1], v[1]));
  REQUIRE(approx(m[2], v[2]));
}

TEST_CASE("rolling_mean: window larger than data => all NaNs") {
  // edge case: not enough points to fill even one window
  std::vector<double> v{1.0, 2.0, 3.0};
  auto m = qe::rolling_mean(v, 5);

  REQUIRE(m.size() == v.size());
  REQUIRE(is_nan(m[0]));
  REQUIRE(is_nan(m[1]));
  REQUIRE(is_nan(m[2]));
}

TEST_CASE("rolling_mean: window=0 throws invalid_argument") {
  // edge case: window size of 0 is nonsensical; should hard-fail
  std::vector<double> v{1.0, 2.0};
  REQUIRE_THROWS_AS(qe::rolling_mean(v, 0), std::invalid_argument);
}

TEST_CASE("rolling_std: basic correctness") {
  // basic correctness for a tiny known series.
  std::vector<double> v{1.0, 2.0, 3.0};
  auto s = qe::rolling_std(v, 3);

  REQUIRE(s.size() == v.size());
  REQUIRE(is_nan(s[0]));
  REQUIRE(is_nan(s[1]));

  // mean=2, variance=((1-2)^2+(2-2)^2+(3-2)^2)/3 = 2/3
  const double expected = std::sqrt(2.0 / 3.0);
  REQUIRE(std::fabs(s[2] - expected) < 1e-12);
}

TEST_CASE("rolling_std: constant series has std=0 once window is filled") {
  // extra sanity: constant values -> zero dispersion
  std::vector<double> v{4.0, 4.0, 4.0, 4.0};
  auto s = qe::rolling_std(v, 2);

  REQUIRE(s.size() == v.size());
  REQUIRE(is_nan(s[0]));
  REQUIRE(approx(s[1], 0.0));
  REQUIRE(approx(s[2], 0.0));
  REQUIRE(approx(s[3], 0.0));
}

TEST_CASE("rolling_std: window larger than data => all NaNs") {
  // edge case: not enough points to compute any std dev window
  std::vector<double> v{1.0, 2.0, 3.0};
  auto s = qe::rolling_std(v, 10);

  REQUIRE(s.size() == v.size());
  REQUIRE(is_nan(s[0]));
  REQUIRE(is_nan(s[1]));
  REQUIRE(is_nan(s[2]));
}

TEST_CASE("rolling_std: window=0 throws invalid_argument") {
  // edge case: window size of 0 is invalid
  std::vector<double> v{1.0, 2.0};
  REQUIRE_THROWS_AS(qe::rolling_std(v, 0), std::invalid_argument);
}

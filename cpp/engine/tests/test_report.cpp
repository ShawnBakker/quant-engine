#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "qe/report.hpp"
#include "qe/backtest.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

static bool approx(double a, double b, double eps = 1e-12) {
  return std::fabs(a - b) <= eps;
}

TEST_CASE("compute_win_rate: empty returns 0") {
  std::vector<double> r;
  REQUIRE(qe::compute_win_rate(r) == 0.0);
}

TEST_CASE("compute_win_rate: counts positive returns only") {
  std::vector<double> r{0.01, -0.02, 0.0, 0.03};
  // wins are strictly > 0 => 2 wins out of 4
  REQUIRE(approx(qe::compute_win_rate(r), 0.5));
}

TEST_CASE("compute_win_rate: ignores NaNs") {
  std::vector<double> r{0.01, std::numeric_limits<double>::quiet_NaN(), -0.02};
  // total counted = 2 (ignores NaN), wins = 1 
  REQUIRE(approx(qe::compute_win_rate(r), 0.5));
}

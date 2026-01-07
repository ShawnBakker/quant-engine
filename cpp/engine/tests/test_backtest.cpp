#include <cmath>
#include <stdexcept>

#include "qe/backtest.hpp"
#include "qe/data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// small helper for float compares
static bool approx(double a, double b, double eps = 1e-12) {
  return std::fabs(a - b) <= eps;
}

TEST_CASE("backtest_sma_crossover: validates inputs") {
  qe::OhlcvTable t;
  // need at least slow_window + 1 rows, so make a minimal 4-row table for slow=3 case
  t.push_back({"t0", 0,0,0, 100.0, 0});
  t.push_back({"t1", 0,0,0, 101.0, 0});
  t.push_back({"t2", 0,0,0, 102.0, 0});
  t.push_back({"t3", 0,0,0, 103.0, 0});

  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 0, 2, 1.0), std::invalid_argument);
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 1, 0, 1.0), std::invalid_argument);
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 2, 2, 1.0), std::invalid_argument);
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 3, 2, 1.0), std::invalid_argument);
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 1, 2, 0.0), std::invalid_argument);
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 1, 2, -1.0), std::invalid_argument);

  // not enough data: slow=5 requires at least 6 rows
  REQUIRE_THROWS_AS(qe::backtest_sma_crossover(t, 1, 5, 1.0), std::invalid_argument);
}

TEST_CASE("backtest_sma_crossover: equity curve length and basic metrics") {
  //strictly increasing close series so returns are positive
  qe::OhlcvTable t;
  t.push_back({"t0", 0,0,0, 100.0, 0});
  t.push_back({"t1", 0,0,0, 110.0, 0});
  t.push_back({"t2", 0,0,0, 121.0, 0});
  t.push_back({"t3", 0,0,0, 133.1, 0});
  t.push_back({"t4", 0,0,0, 146.41, 0});

  auto r = qe::backtest_sma_crossover(t, 1, 2, 1.0);

  REQUIRE(r.strat_ret.size() == 4);
  REQUIRE(r.equity.size() == 4);

  // because the series is rising, total return should be >= 0.
  REQUIRE(r.total_return >= 0.0);

  // drawdown should never be negative.
  REQUIRE(r.max_drawdown >= 0.0);

  // final equity should be 1 + total_return (by definition here)
  REQUIRE(approx(r.equity.back(), 1.0 * (1.0 + r.total_return), 1e-12));
}

TEST_CASE("backtest_sma_crossover: produces finite outputs") {
  qe::OhlcvTable t;
  t.push_back({"t0", 0,0,0, 100.0, 0});
  t.push_back({"t1", 0,0,0,  90.0, 0});
  t.push_back({"t2", 0,0,0,  95.0, 0});
  t.push_back({"t3", 0,0,0,  85.0, 0});
  t.push_back({"t4", 0,0,0,  88.0, 0});
  t.push_back({"t5", 0,0,0,  92.0, 0});

  auto r = qe::backtest_sma_crossover(t, 1, 2, 1.0);

  REQUIRE(!r.equity.empty());
  REQUIRE(std::isfinite(r.total_return));
  REQUIRE(std::isfinite(r.max_drawdown));
  REQUIRE(std::isfinite(r.sharpe));
}

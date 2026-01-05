#include <cmath>
#include <limits>

#include "qe/backtest.hpp"
#include "qe/data.hpp"

#include <catch2/catch_test_macros.hpp>

// helper for float compares (tight eps because we use simple arithmetic here)
static bool approx(double a, double b, double eps = 1e-12) {
  return std::fabs(a - b) <= eps;
}

TEST_CASE("backtest_sma_crossover: validates inputs") {
  qe::OhlcvTable t;
  // need at least slow_window + 1 rows for slow=3 -> 4 rows.
  t.push_back({"t0", 0, 0, 0, 100.0, 0});
  t.push_back({"t1", 0, 0, 0, 101.0, 0});
  t.push_back({"t2", 0, 0, 0, 102.0, 0});
  t.push_back({"t3", 0, 0, 0, 103.0, 0});

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
  qe::OhlcvTable t;
  t.push_back({"t0", 0, 0, 0, 100.0, 0});
  t.push_back({"t1", 0, 0, 0, 110.0, 0});
  t.push_back({"t2", 0, 0, 0, 121.0, 0});
  t.push_back({"t3", 0, 0, 0, 133.1, 0});
  t.push_back({"t4", 0, 0, 0, 146.41, 0});

  auto r = qe::backtest_sma_crossover(t, 1, 2, 1.0);

  REQUIRE(r.strat_ret.size() == 4);
  REQUIRE(r.equity.size() == 4);

  // because the series rises -- total return should be >= 0
  REQUIRE(r.total_return >= 0.0);

  // drawdown never negative
  REQUIRE(r.max_drawdown >= 0.0);

  // final equity should be 1 + total_return 
  REQUIRE(approx(r.equity.back(), 1.0 * (1.0 + r.total_return), 1e-12));
}

TEST_CASE("backtest_sma_crossover: produces finite outputs") {
  qe::OhlcvTable t;
  t.push_back({"t0", 0, 0, 0, 100.0, 0});
  t.push_back({"t1", 0, 0, 0,  90.0, 0});
  t.push_back({"t2", 0, 0, 0,  95.0, 0});
  t.push_back({"t3", 0, 0, 0,  85.0, 0});
  t.push_back({"t4", 0, 0, 0,  88.0, 0});
  t.push_back({"t5", 0, 0, 0,  92.0, 0});

  auto r = qe::backtest_sma_crossover(t, 1, 2, 1.0);

  REQUIRE(!r.equity.empty());
  REQUIRE(std::isfinite(r.total_return));
  REQUIRE(std::isfinite(r.max_drawdown));
  REQUIRE(std::isfinite(r.sharpe));
}

TEST_CASE("backtest_sma_crossover: costs reduce final equity and record trade diagnostics") {
  // build a close series that creates alternating returns, which forces SMA(1) vs SMA(2)
  // crossovers on returns, leads to mult. trades
  //
  // close: 100 -> 110 (+0.10)
  //        110 ->  99 (-0.10)
  //         99 -> 108.9 (+0.10)
  //      108.9 ->  98.01 (-0.10)
  //       98.01 -> 107.811 (+0.10)
  //
  // w/ fast=1, slow=2 on RETURNS:
  // slow is ~0 once defined; fast flips sign -> position flips -> trades occur
  qe::OhlcvTable t;
  t.push_back({"t0", 0, 0, 0, 100.0, 0});
  t.push_back({"t1", 0, 0, 0, 110.0, 0});
  t.push_back({"t2", 0, 0, 0,  99.0, 0});
  t.push_back({"t3", 0, 0, 0, 108.9, 0});
  t.push_back({"t4", 0, 0, 0,  98.01, 0});
  t.push_back({"t5", 0, 0, 0, 107.811, 0});

  // no costs
  qe::BacktestResult r0 = qe::backtest_sma_crossover(t, 1, 2, 1.0);

  // with costs: 10 bps fee, 0 bps slippage -> 0.001 per trade
  qe::BacktestCosts c;
  c.fee_bps = 10.0;
  c.slippage_bps = 0.0;

  qe::BacktestResult r1 = qe::backtest_sma_crossover(t, 1, 2, 1.0, c);

  // should reduce final equity
  REQUIRE(r1.equity.back() < r0.equity.back());

  // should have traded at least once, and total_cost should reflect that
  REQUIRE(r1.n_trades > 0);
  REQUIRE(r1.total_cost > 0.0);

  // trade count should be deterministic for this sequence (expected 3 trades)
  REQUIRE(r1.n_trades == 3);
}

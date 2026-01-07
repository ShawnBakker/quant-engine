#pragma once

#include <cstddef>
#include <vector>

#include "qe/data.hpp"

namespace qe {

// Transaction cost model (basis points)
struct BacktestCosts {
  double fee_bps = 0.0;
  double slippage_bps = 0.0;
};

struct BacktestResult {
  std::vector<double> equity;     // equity curve
  std::vector<double> strat_ret;  // strategy returns per step
  double total_return = 0.0;
  double max_drawdown = 0.0;
  double sharpe = 0.0;

  std::size_t n_trades = 0;
  double total_cost = 0.0;
};

BacktestResult backtest_sma_crossover(
  const OhlcvTable& data,
  std::size_t fast_window,
  std::size_t slow_window,
  double initial_equity = 1.0,
  BacktestCosts costs = {}
);

} // 

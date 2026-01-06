#pragma once

#include <vector>
#include "qe/data.hpp"

namespace qe {

struct BacktestResult {
  std::vector<double> equity;     // length = returns len (or prices length-1)
  std::vector<double> strat_ret;  // strategy returns per step
  double total_return = 0.0;
  double max_drawdown = 0.0;
  double sharpe = 0.0;
};

BacktestResult backtest_sma_crossover(
  const OhlcvTable& data,
  std::size_t fast_window,
  std::size_t slow_window,
  double initial_equity = 1.0
);

} // qe

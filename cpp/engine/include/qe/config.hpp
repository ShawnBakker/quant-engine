#pragma once

#include <cstddef>
#include <string>

namespace qe {

struct BacktestConfig {
  std::string strategy = "sma_crossover";
  std::size_t fast = 5;
  std::size_t slow = 20;
  double initial = 1.0;

  // basis points
  double fee_bps = 0.0;
  double slippage_bps = 0.0;
};

BacktestConfig load_backtest_config_json(const std::string& path);

} 

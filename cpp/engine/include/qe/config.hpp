#pragma once

#include <cstddef>
#include <string>

namespace qe {

// config (I10)


struct BacktestConfig {
  std::string strategy = "sma_crossover";
  std::size_t fast = 5;
  std::size_t slow = 20;
  double initial = 1.0;

  // costs
  double fee_bps = 0.0;
  double slippage_bps = 0.0;
};

// loads a JSON config file
// throws on unreadable file or invalid JSON types
BacktestConfig load_backtest_config_json(const std::string& path);

} 

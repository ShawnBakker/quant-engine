#pragma once

#include <string>
#include <vector>

#include "qe/backtest.hpp"

namespace qe {

double compute_win_rate(const std::vector<double>& strat_returns);

//CSV with two columns: index,equity
void write_equity_csv(const std::string& path, const std::vector<double>& equity);

// JSON report (no external deps)
void write_report_json(
  const std::string& path,
  const std::string& strategy_name,
  std::size_t fast_window,
  std::size_t slow_window,
  double initial_equity,
  const BacktestResult& result
);

} // namespace qe

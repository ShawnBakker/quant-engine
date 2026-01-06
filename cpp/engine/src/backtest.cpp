#include "qe/backtest.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include "qe/indicators.hpp"

namespace qe {

static bool is_nan(double x) {
  return std::isnan(x);
}

static double compute_max_drawdown(const std::vector<double>& equity) {
  if (equity.empty()) return 0.0;

  double peak = equity[0];
  double max_dd = 0.0;

  for (double v : equity) {
    peak = std::max(peak, v);
    if (peak > 0.0) {
      double dd = (peak - v) / peak;
      max_dd = std::max(max_dd, dd);
    }
  }
  return max_dd;
}

static double compute_sharpe(const std::vector<double>& r) {
  // Sharpe on per-period returns (no annualization yet)
  if (r.empty()) return 0.0;

  double mean = 0.0;
  for (double x : r) mean += x;
  mean /= static_cast<double>(r.size());

  double var = 0.0;
  for (double x : r) {
    double d = x - mean;
    var += d * d;
  }
  var /= static_cast<double>(r.size());

  double sd = std::sqrt(var);
  if (sd == 0.0) return 0.0;
  return mean / sd;
}

BacktestResult backtest_sma_crossover(
  const OhlcvTable& data,
  std::size_t fast_window,
  std::size_t slow_window,
  double initial_equity,
  BacktestCosts costs
) {
  if (fast_window == 0 || slow_window == 0) {
    throw std::invalid_argument("windows must be > 0");
  }
  if (fast_window >= slow_window) {
    throw std::invalid_argument("fast_window must be < slow_window");
  }
  const std::size_t min_rows = slow_window + 1;
  if (data.size() < min_rows) {
    throw std::invalid_argument(
      "not enough data: need at least " + std::to_string(min_rows) +
      " rows for slow_window=" + std::to_string(slow_window) +
      " (got " + std::to_string(data.size()) + ")"
    );
  }

  if (initial_equity <= 0.0) {
    throw std::invalid_argument("initial_equity must be > 0");
  }

  // close-to-close returns (length n-1)
  std::vector<double> r = compute_returns(data);

  // close vector aligned to returns indices
  // returns[i] corresponds to close[i] -> close[i+1]
  std::vector<double> close;
  close.reserve(data.size());
  for (const auto& row : data) close.push_back(row.close);

  // Compute SMAs on returns index space by using close[1..] (aligned to r indices)
  std::vector<double> close_aligned(close.begin() + 1, close.end()); // length N-1
  std::vector<double> fast = rolling_mean(close_aligned, fast_window);
  std::vector<double> slow = rolling_mean(close_aligned, slow_window);

  BacktestResult out;
  out.strat_ret.assign(r.size(), 0.0);
  out.equity.assign(r.size(), initial_equity);

  double eq = initial_equity;
  int pos = 0; // 0 = flat, 1 = long

  for (std::size_t i = 0; i < r.size(); ++i) {
    // Only trade when both SMAs are defined, throw error if not (?)
    if (!is_nan(fast[i]) && !is_nan(slow[i])) {
      pos = (fast[i] > slow[i]) ? 1 : 0;
    }

    double sr = static_cast<double>(pos) * r[i];
    out.strat_ret[i] = sr;

    eq *= (1.0 + sr);
    out.equity[i] = eq;
  }

  out.total_return = (out.equity.back() / initial_equity) - 1.0;
  out.max_drawdown = compute_max_drawdown(out.equity);
  out.sharpe = compute_sharpe(out.strat_ret);
  return out;
}

} // qe

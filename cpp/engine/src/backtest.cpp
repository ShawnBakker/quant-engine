#include "qe/backtest.hpp"

#include "qe/indicators.hpp"

#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace qe {

static bool is_nan(double x) {
  return std::isnan(x);
}

// compute max drawdown from equity curve
static double compute_max_drawdown(const std::vector<double>& equity) {
  double peak = equity.front();
  double max_dd = 0.0;

  for (double v : equity) {
    if (v > peak) {
      peak = v;
    }
    double dd = (peak - v) / peak;
    if (dd > max_dd) {
      max_dd = dd;
    }
  }

  return max_dd;
}

// simple Sharpe (no annualization)
static double compute_sharpe(const std::vector<double>& returns) {
  if (returns.empty()) {
    return 0.0;
  }

  double mean =
    std::accumulate(returns.begin(), returns.end(), 0.0) /
    static_cast<double>(returns.size());

  double var = 0.0;
  for (double r : returns) {
    double d = r - mean;
    var += d * d;
  }
  var /= static_cast<double>(returns.size());

  double stddev = std::sqrt(var);
  return (stddev > 0.0) ? (mean / stddev) : 0.0;
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
  if (initial_equity <= 0.0) {
    throw std::invalid_argument("initial_equity must be > 0");
  }
  if (data.size() <= slow_window) {
    throw std::invalid_argument("not enough data for slow_window");
  }

  // compute returns
  std::vector<double> ret = compute_returns(data);

  // rolling indicators on returns
  std::vector<double> fast = rolling_mean(ret, fast_window);
  std::vector<double> slow = rolling_mean(ret, slow_window);

  BacktestResult out;
  out.equity.resize(ret.size());
  out.strat_ret.resize(ret.size());

  double eq = initial_equity;
  int pos = 0; // 0 = flat, 1 = long

  const double cost_frac =
    (costs.fee_bps + costs.slippage_bps) / 10000.0;

  // main backtest loop 
  for (std::size_t i = 0; i < ret.size(); ++i) {
    int new_pos = pos;
    bool traded = false;

    if (!is_nan(fast[i]) && !is_nan(slow[i])) {
      new_pos = (fast[i] > slow[i]) ? 1 : 0;
    }

    // apply transaction costs on position change
    if (new_pos != pos) {
      traded = true;

      double cost_amount = eq * cost_frac;
      eq -= cost_amount;

      out.n_trades += 1;
      out.total_cost += cost_amount;

      pos = new_pos;
    }

    // Strategy return INCLUDING cost impact
    double sr = static_cast<double>(pos) * ret[i];
    if (traded) {
      sr -= cost_frac; // apprx cost impact on returns
    }

    out.strat_ret[i] = sr;

    eq *= (1.0 + sr);
    out.equity[i] = eq;
  }

  // metrics
  out.total_return = (out.equity.back() / initial_equity) - 1.0;
  out.max_drawdown = compute_max_drawdown(out.equity);
  out.sharpe = compute_sharpe(out.strat_ret);

  return out;
}

} // qe

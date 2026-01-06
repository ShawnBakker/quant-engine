#include "qe/bench.hpp"

#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"
#include "qe/backtest.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace qe {

static double ms_since(const std::chrono::steady_clock::time_point& start,
                       const std::chrono::steady_clock::time_point& end) {
  return std::chrono::duration<double, std::milli>(end - start).count();
}

static std::size_t clamp_min(std::size_t v, std::size_t lo) {
  return (v < lo) ? lo : v;
}

int run_benchmarks(const std::string& csv_path, std::size_t iters) {
  if (iters == 0) {
    throw std::invalid_argument("--iters must be > 0");
  }

  OhlcvTable table = read_ohlcv_csv(csv_path);
  if (table.size() < 3) {
    throw std::invalid_argument("need at least 3 rows for benchmarks");
  }

  std::vector<double> ret = compute_returns(table);

  // compute_returns
  {
    auto t0 = std::chrono::steady_clock::now();
    volatile double sink = 0.0;

    for (std::size_t i = 0; i < iters; ++i) {
      auto r = compute_returns(table);
      if (!r.empty()) {
        sink += r.back();
      }
    }

    auto t1 = std::chrono::steady_clock::now();
    std::cout << "[bench] compute_returns: " << ms_since(t0, t1)
              << " ms (" << iters << " iters)\n";
  }

  //rolling_mean / rolling_std
  {

    const std::size_t w = clamp_min(std::min<std::size_t>(20, ret.size()), 2);

    auto t0 = std::chrono::steady_clock::now();
    volatile double sink = 0.0;

    for (std::size_t i = 0; i < iters; ++i) {
      auto m = rolling_mean(ret, w);
      auto s = rolling_std(ret, w);
      if (!m.empty()) sink += m.back();
      if (!s.empty()) sink += s.back();
    }

    auto t1 = std::chrono::steady_clock::now();
    std::cout << "[bench] rolling_mean/std (w=" << w << "): " << ms_since(t0, t1)
              << " ms (" << iters << " iters)\n";
  }

  // backtest loop
  {
    // ensure slow_window < table.size()
    // Use slow up to 20, but clamp for fitting
    std::size_t slow = std::min<std::size_t>(20, table.size() - 1);
    slow = clamp_min(slow, 2);

    // ensure fast < slow and fast >= 1
    std::size_t fast = std::min<std::size_t>(5, slow - 1);
    fast = clamp_min(fast, 1);

    auto t0 = std::chrono::steady_clock::now();
    volatile double sink = 0.0;

    for (std::size_t i = 0; i < iters; ++i) {
      BacktestCosts c;
      c.fee_bps = 1.0;
      c.slippage_bps = 1.0;

      auto r = backtest_sma_crossover(table, fast, slow, 1.0, c);
      if (!r.equity.empty()) {
        sink += r.equity.back();
      }
    }

    auto t1 = std::chrono::steady_clock::now();
    std::cout << "[bench] backtest_sma_crossover (fast=" << fast
              << " slow=" << slow << ", 2bps): " << ms_since(t0, t1)
              << " ms (" << iters << " iters)\n";
  }

  std::cout << "[bench] rows=" << table.size()
            << " returns=" << (table.size() > 0 ? table.size() - 1 : 0) << "\n";

  return 0;
}

} // qe

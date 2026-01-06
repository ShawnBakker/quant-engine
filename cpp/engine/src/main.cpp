#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <system_error>

#include "qe/config.hpp"
#include "qe/version.hpp"
#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"
#include "qe/backtest.hpp"
#include "qe/report.hpp"
#include "qe/equity_io.hpp"

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];


    // Version 

    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }


    // CSV sanity check

    if (cmd == "run") {
      std::string data_path;

      for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      try {
        auto table = qe::read_ohlcv_csv(data_path);
        std::cout << "Loaded " << table.size()
                  << " rows from " << data_path << "\n";
      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }


    // Indicators

    if (cmd == "indicators") {
      std::string data_path;
      std::size_t window = 5;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc)
          data_path = argv[++i];
        else if (arg == "--window" && i + 1 < argc)
          window = static_cast<std::size_t>(std::stoul(argv[++i]));
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      try {
        auto table = qe::read_ohlcv_csv(data_path);
        auto returns = qe::compute_returns(table);
        auto mean = qe::rolling_mean(returns, window);
        auto stddev = qe::rolling_std(returns, window);

        std::cout << "rows=" << table.size()
                  << " window=" << window << "\n";

        for (std::size_t i = returns.size() > 5 ? returns.size() - 5 : 0;
             i < returns.size(); ++i) {
          std::cout << "i=" << i
                    << " ret=" << returns[i]
                    << " mean=" << mean[i]
                    << " std=" << stddev[i] << "\n";
        }

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }


// Backtest (with config + stale-output protection)

if (cmd == "backtest") {
  std::string data_path;
  std::string config_path;
  std::string out_dir;

  // Defaults
  qe::BacktestConfig cfg{};
  qe::BacktestCosts costs{};

  bool cli_fast = false;
  bool cli_slow = false;
  bool cli_initial = false;
  bool cli_fee = false;
  bool cli_slip = false;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--data" && i + 1 < argc) {
      data_path = argv[++i];
    } else if (arg == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (arg == "--fast" && i + 1 < argc) {
      cfg.fast = static_cast<std::size_t>(std::stoul(argv[++i]));
      cli_fast = true;
    } else if (arg == "--slow" && i + 1 < argc) {
      cfg.slow = static_cast<std::size_t>(std::stoul(argv[++i]));
      cli_slow = true;
    } else if (arg == "--initial" && i + 1 < argc) {
      cfg.initial = std::stod(argv[++i]);
      cli_initial = true;
    } else if (arg == "--fee-bps" && i + 1 < argc) {
      costs.fee_bps = std::stod(argv[++i]);
      cli_fee = true;
    } else if (arg == "--slip-bps" && i + 1 < argc) {
      costs.slippage_bps = std::stod(argv[++i]);
      cli_slip = true;
    } else if (arg == "--out" && i + 1 < argc) {
      out_dir = argv[++i];
    }
  }

  if (data_path.empty()) {
    std::cerr << "Error: --data <csv_path> is required\n";
    return 1;
  }

  // Load config.json if provided
  if (!config_path.empty()) {
    try {
      cfg = qe::load_backtest_config_json(config_path);
    } catch (const std::exception& ex) {
      std::cerr << "Error: " << ex.what() << "\n";
      return 1;
    }
  }

  // Apply CLI overrides (CLI wins)
  if (cli_fast) cfg.fast = cfg.fast;
  if (cli_slow) cfg.slow = cfg.slow;
  if (cli_initial) cfg.initial = cfg.initial;
  if (cli_fee) costs.fee_bps = costs.fee_bps;
  if (cli_slip) costs.slippage_bps = costs.slippage_bps;

  // Pre-create output dir and remove stale artifacts
  std::string equity_path;
  std::string report_path;
  if (!out_dir.empty()) {
    std::filesystem::create_directories(out_dir);
    equity_path = (std::filesystem::path(out_dir) / "equity.csv").string();
    report_path = (std::filesystem::path(out_dir) / "report.json").string();

    std::error_code ec;
    std::filesystem::remove(equity_path, ec);
    ec.clear();
    std::filesystem::remove(report_path, ec);
  }

  try {
    qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);

    qe::BacktestResult r =
      qe::backtest_sma_crossover(
        table,
        cfg.fast,
        cfg.slow,
        cfg.initial,
        { cfg.fee_bps, cfg.slippage_bps }
      );

    std::cout << "backtest: " << cfg.strategy
              << " fast=" << cfg.fast
              << " slow=" << cfg.slow
              << " initial=" << cfg.initial
              << " fee_bps=" << cfg.fee_bps
              << " slip_bps=" << cfg.slippage_bps << "\n";

    std::cout << "total_return=" << r.total_return
              << " sharpe=" << r.sharpe
              << " max_drawdown=" << r.max_drawdown
              << " win_rate=" << qe::compute_win_rate(r.strat_ret) << "\n";

    std::cout << "trades=" << r.n_trades
              << " total_cost=" << r.total_cost << "\n";

    if (!r.equity.empty()) {
      std::cout << "final_equity=" << r.equity.back() << "\n";
    }

    if (!out_dir.empty()) {
      qe::write_equity_csv(equity_path, r.equity);
      qe::write_report_json(
        report_path,
        cfg.strategy,
        cfg.fast,
        cfg.slow,
        cfg.initial,
        r
      );
      std::cout << "wrote " << equity_path << "\n";
      std::cout << "wrote " << report_path << "\n";
    }

  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}

  // Usage
  std::cout << "qe_cli\n"
            << "Usage:\n"
            << "  qe_cli --version\n"
            << "  qe_cli run --data <csv_path>\n"
            << "  qe_cli indicators --data <csv_path> [--window N]\n"
            << "  qe_cli backtest --data <csv_path> "
               "[--config cfg.json] "
               "[--fast N] [--slow N] [--initial X] "
               "[--fee-bps N] [--slip-bps N] "
               "[--out <dir>]\n";

  return 0;
}
}

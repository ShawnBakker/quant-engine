#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "qe/config.hpp"
#include "qe/version.hpp"
#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"
#include "qe/backtest.hpp"
#include "qe/report.hpp"
#include "qe/equity_io.hpp"
#include "qe/options.hpp"

static void print_usage() {
  std::cout << "qe_cli\n";
  std::cout << "Usage:\n";
  std::cout << "  qe_cli --version\n";
  std::cout << "  qe_cli run --data <csv_path>\n";
  std::cout << "  qe_cli indicators --data <csv_path> [--window N]\n";
  std::cout << "  qe_cli backtest --data <csv_path> "
               "[--config cfg.json] "
               "[--fast N] [--slow N] [--initial X] "
               "[--fee-bps N] [--slip-bps N] "
               "[--out <dir>]\n";
  std::cout << "  qe_cli options --S <spot> --K <strike> --r <rate> --sigma <vol> --T <years>\n";
}

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];

    // Version
    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }

    // Run (CSV ingestion check)
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
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);
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
        if (arg == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        } else if (arg == "--window" && i + 1 < argc) {
          window = static_cast<std::size_t>(std::stoul(argv[++i]));
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      try {
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);

        std::vector<double> returns = qe::compute_returns(table);
        std::vector<double> mean = qe::rolling_mean(returns, window);
        std::vector<double> stddev = qe::rolling_std(returns, window);

        std::cout << "rows=" << table.size()
                  << " returns=" << returns.size()
                  << " window=" << window << "\n";

        std::size_t start = returns.size() > 5 ? returns.size() - 5 : 0;
        for (std::size_t i = start; i < returns.size(); ++i) {
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

    // options pricing (Black-Scholes, no dividends)
    if (cmd == "options") {
      std::optional<double> S;
      std::optional<double> K;
      std::optional<double> r;
      std::optional<double> sigma;
      std::optional<double> T;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--S" && i + 1 < argc) {
          S = std::stod(argv[++i]);
        } else if (arg == "--K" && i + 1 < argc) {
          K = std::stod(argv[++i]);
        } else if (arg == "--r" && i + 1 < argc) {
          r = std::stod(argv[++i]);
        } else if (arg == "--sigma" && i + 1 < argc) {
          sigma = std::stod(argv[++i]);
        } else if (arg == "--T" && i + 1 < argc) {
          T = std::stod(argv[++i]);
        }
      }

      if (!S || !K || !r || !sigma || !T) {
        std::cerr << "Error: options requires --S --K --r --sigma --T\n";
        std::cerr << "Example: qe_cli options --S 100 --K 110 --r 0.05 --sigma 0.2 --T 0.5\n";
        return 1;
      }

      try {
        const double call = qe::black_scholes_call(*S, *K, *r, *sigma, *T);
        const double put  = qe::black_scholes_put(*S, *K, *r, *sigma, *T);

        const double dc = qe::bs_delta_call(*S, *K, *r, *sigma, *T);
        const double dp = qe::bs_delta_put(*S, *K, *r, *sigma, *T);
        const double v  = qe::bs_vega(*S, *K, *r, *sigma, *T);

        std::cout << "options: black_scholes"
                  << " S=" << *S
                  << " K=" << *K
                  << " r=" << *r
                  << " sigma=" << *sigma
                  << " T=" << *T << "\n";

        std::cout << "call=" << call
                  << " put=" << put << "\n";

        std::cout << "delta_call=" << dc
                  << " delta_put=" << dp
                  << " vega=" << v << "\n";

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }

    // Backtest with config + stale-output protection
    if (cmd == "backtest") {
      std::string data_path;
      std::string config_path;
      std::string out_dir;

      std::optional<std::size_t> fast_override;
      std::optional<std::size_t> slow_override;
      std::optional<double> initial_override;
      std::optional<double> fee_override;
      std::optional<double> slip_override;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
          config_path = argv[++i];
        } else if (arg == "--fast" && i + 1 < argc) {
          fast_override = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--slow" && i + 1 < argc) {
          slow_override = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--initial" && i + 1 < argc) {
          initial_override = std::stod(argv[++i]);
        } else if (arg == "--fee-bps" && i + 1 < argc) {
          fee_override = std::stod(argv[++i]);
        } else if (arg == "--slip-bps" && i + 1 < argc) {
          slip_override = std::stod(argv[++i]);
        } else if (arg == "--out" && i + 1 < argc) {
          out_dir = argv[++i];
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      // defaults
      qe::BacktestConfig cfg{};

      // config.json if provided
      if (!config_path.empty()) {
        try {
          cfg = qe::load_backtest_config_json(config_path);
        } catch (const std::exception& ex) {
          std::cerr << "Error: " << ex.what() << "\n";
          return 1;
        }
      }

      // CLI overrides win
      if (fast_override) cfg.fast = *fast_override;
      if (slow_override) cfg.slow = *slow_override;
      if (initial_override) cfg.initial = *initial_override;
      if (fee_override) cfg.fee_bps = *fee_override;
      if (slip_override) cfg.slippage_bps = *slip_override;

      // output dir and remove stale artifacts
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

        qe::BacktestCosts costs{ cfg.fee_bps, cfg.slippage_bps };

        qe::BacktestResult r =
          qe::backtest_sma_crossover(table, cfg.fast, cfg.slow, cfg.initial, costs);

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
          qe::write_report_json(report_path, cfg.strategy, cfg.fast, cfg.slow, cfg.initial, r);
          std::cout << "wrote " << equity_path << "\n";
          std::cout << "wrote " << report_path << "\n";
        }

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }
  }

  print_usage();
  return 0;
}

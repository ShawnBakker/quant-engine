#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <system_error>

#include "qe/version.hpp"
#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"
#include "qe/backtest.hpp"
#include "qe/report.hpp"

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];

    // Version

    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }


    // Run (CSV ingestion(I3))

    if (cmd == "run") {
      std::string data_path;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
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

 
    // Indicators (I4)

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

        // Print last few rows as sanity check
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

 
    // Backtest (I5)
  
    if (cmd == "backtest") {
      std::string data_path;
      std::string out_dir;
      std::size_t fast = 5;
      std::size_t slow = 20;
      double initial = 1.0;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        } else if (arg == "--fast" && i + 1 < argc) {
          fast = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--slow" && i + 1 < argc) {
          slow = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--initial" && i + 1 < argc) {
          initial = std::stod(argv[++i]);
        } else if (arg == "--out" && i + 1 < argc) {
          out_dir = argv[++i];
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }
      // Pre-create output dir and remove stale artifacts up front
      std::string equity_path;
      std::string report_path;
      if (!out_dir.empty()) {
        std::filesystem::create_directories(out_dir);

        equity_path = (std::filesystem::path(out_dir) / "equity.csv").string();
        report_path = (std::filesystem::path(out_dir) / "report.json").string();

        // Remove old outputs so failures can't leave stale files behind
        std::error_code ec;
        std::filesystem::remove(equity_path, ec);
        ec.clear();
        std::filesystem::remove(report_path, ec);
      }

      try {
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);
        qe::BacktestResult r = qe::backtest_sma_crossover(table, fast, slow, initial);

        std::cout << "backtest: sma_crossover fast=" << fast
                  << " slow=" << slow
                  << " initial=" << initial << "\n";

        std::cout << "total_return=" << r.total_return
                  << " sharpe=" << r.sharpe
                  << " max_drawdown=" << r.max_drawdown
                  << " win_rate=" << qe::compute_win_rate(r.strat_ret) << "\n";

        if (!r.equity.empty()) {
          std::cout << "final_equity=" << r.equity.back() << "\n";
        }

        if (!out_dir.empty()) {
          std::filesystem::create_directories(out_dir);

          const std::string equity_path =
            (std::filesystem::path(out_dir) / "equity.csv").string();
          const std::string report_path =
            (std::filesystem::path(out_dir) / "report.json").string();

          qe::write_equity_csv(equity_path, r.equity);
          qe::write_report_json(report_path, "sma_crossover", fast, slow, initial, r);

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

  
  // Usage

  std::cout << "qe_cli\n";
  std::cout << "Usage:\n";
  std::cout << "  qe_cli --version\n";
  std::cout << "  qe_cli run --data <csv_path>\n";
  std::cout << "  qe_cli indicators --data <csv_path> [--window N]\n";
  std::cout << "  qe_cli backtest --data <csv_path> [--fast N] [--slow N] [--initial X] [--out <dir>]\n";

  return 0;
}

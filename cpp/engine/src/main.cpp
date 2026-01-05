#include <iostream>
#include <string>
#include <vector>

#include "qe/version.hpp"
#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];


    // Version

    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }


    // Run (CSV ingestion sanity (I3))

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
        std::size_t start =
          returns.size() > 5 ? returns.size() - 5 : 0;

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
  }


  // Usage

  std::cout << "qe_cli\n";
  std::cout << "Usage:\n";
  std::cout << "  qe_cli --version\n";
  std::cout << "  qe_cli run --data <csv_path>\n";
  std::cout << "  qe_cli indicators --data <csv_path> [--window N]\n";

  return 0;
}

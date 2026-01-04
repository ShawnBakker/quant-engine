#include <iostream>
#include <string>

#include "qe/version.hpp"
#include "qe/csv_reader.hpp"

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];

    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }

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
        std::cout << "Loaded " << table.size() << " rows from " << data_path << "\n";
      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }
  }

  std::cout << "qe_cli\n";
  std::cout << "Usage:\n";
  std::cout << "  qe_cli --version\n";
  std::cout << "  qe_cli run --data <csv_path>\n";
  return 0;
}

#include "qe/csv_reader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace qe {

OhlcvTable read_ohlcv_csv(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open CSV file: " + path);
  }

  OhlcvTable table;
  std::string line;

  // Read header
  if (!std::getline(file, line)) {
    throw std::runtime_error("CSV file is empty: " + path);
  }

  while (std::getline(file, line)) {
    if (line.empty()) continue;

    std::stringstream ss(line);
    std::string cell;

    OhlcvRow row;

    std::getline(ss, row.timestamp, ',');

    std::getline(ss, cell, ',');
    row.open = std::stod(cell);

    std::getline(ss, cell, ',');
    row.high = std::stod(cell);

    std::getline(ss, cell, ',');
    row.low = std::stod(cell);

    std::getline(ss, cell, ',');
    row.close = std::stod(cell);

    std::getline(ss, cell, ',');
    row.volume = std::stod(cell);

    table.push_back(row);
  }

  return table;
}

} 

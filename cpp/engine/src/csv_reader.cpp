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

  // Read header line
  if (!std::getline(file, line)) {
    throw std::runtime_error("CSV file is empty: " + path);
  }

  // Read data lines
  while (std::getline(file, line)) {
    if (line.empty()) continue;

    std::stringstream ss(line);
    std::string cell;

    OhlcvRow row;

    // timestamp
    std::getline(ss, row.timestamp, ',');

    // open
    std::getline(ss, cell, ',');
    row.open = std::stod(cell);

    // high
    std::getline(ss, cell, ',');
    row.high = std::stod(cell);

    // low
    std::getline(ss, cell, ',');
    row.low = std::stod(cell);

    // close
    std::getline(ss, cell, ',');
    row.close = std::stod(cell);

    // volume
    std::getline(ss, cell, ',');
    row.volume = std::stod(cell);

    table.push_back(row);
  }

  return table;
}

} 

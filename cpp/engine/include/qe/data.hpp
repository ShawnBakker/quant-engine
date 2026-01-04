#pragma once

#include <string>
#include <vector>

namespace qe {

struct OhlcvRow {
  std::string timestamp;
  double open;
  double high;
  double low;
  double close;
  double volume;
}; // outlines the struct data for open, high, low, close, and volume

using OhlcvTable = std::vector<OhlcvRow>;

} // namespace defined as qe

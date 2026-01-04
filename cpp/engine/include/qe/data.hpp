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
};

using OhlcvTable = std::vector<OhlcvRow>;

} // namespace qe

#include "qe/report.hpp"

#include <cmath>
#include <vector>

namespace qe {

double compute_win_rate(const std::vector<double>& strat_ret) {
  std::size_t wins = 0;
  std::size_t total = 0;

  for (double r : strat_ret) {
    if (std::isnan(r)) {
      continue;
    }

    ++total;

    // win = positive period return
    if (r > 0.0) {
      ++wins;
    }
  }

  // no returns => 0 win rate
  if (total == 0) {
    return 0.0;
  }

  return static_cast<double>(wins) / static_cast<double>(total);
}

} 

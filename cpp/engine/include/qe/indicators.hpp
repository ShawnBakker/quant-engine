#pragma once

#include <cstddef>
#include <vector>
#include "qe/data.hpp"

namespace qe {

// close-to-close: (close[i] - close[i-1]) / close[i-1]
std::vector<double> compute_returns(const OhlcvTable& data);

// rolling mean: output same length as input, leading values are nil until window fills
std::vector<double> rolling_mean(const std::vector<double>& values, std::size_t window);

// rolling std dev: output same length as input, leading values nil till window fills.
std::vector<double> rolling_std(const std::vector<double>& values, std::size_t window);

} // namespace qe

#include "qe/indicators.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace qe {

static double nanv() {
  return std::numeric_limits<double>::quiet_NaN();
} // need this for a "quiet" nan, avoids polluting the public API and marks undefineds or invalids

std::vector<double> compute_returns(const OhlcvTable& data) {
  if (data.size() < 2) {
    return {};
  }

  std::vector<double> out;
  out.reserve(data.size() - 1);

  for (std::size_t i = 1; i < data.size(); ++i) {
    const double prev = data[i - 1].close;
    const double curr = data[i].close;

    if (prev == 0.0) {
      out.push_back(nanv());
    } else {
      out.push_back((curr - prev) / prev);
    }
  }

  return out;
} // simple returns from a time-ordered OHLCV table, return_i = (close_i - close{i-1} / close_{i-1}), returns empty vector if fewer than 2 data pts provided

std::vector<double> rolling_mean(const std::vector<double>& values, std::size_t window) {
  if (window == 0) {
    throw std::invalid_argument("rolling_mean: window must be > 0");
  } // vals before window is "full" are set as Nan, output vector is the same size as the input.

  std::vector<double> out(values.size(), nanv());
  if (values.size() < window) {
    return out;
  }

  double sum = 0.0;
  for (std::size_t i = 0; i < values.size(); ++i) {
    sum += values[i];
    if (i >= window) {
      sum -= values[i - window];
    }
    if (i + 1 >= window) {
      out[i] = sum / static_cast<double>(window);
    }
  }

  return out;
} // very important to have sliding-window accumulation : add curr, sub the val that falls out the window. this does yield a O(n) time w/ extra space.

std::vector<double> rolling_std(const std::vector<double>& values, std::size_t window) {
  if (window == 0) {
    throw std::invalid_argument("rolling_std: window must be > 0");
  }

  std::vector<double> out(values.size(), nanv());
  if (values.size() < window) {
    return out;
  } // this implementation favors clarity and numerical correctness, it does overperform w/ O(n * window), likely needs optimization
  for (std::size_t i = window - 1; i < values.size(); ++i) {
    double mean = 0.0;
    for (std::size_t j = i + 1 - window; j <= i; ++j) {
      mean += values[j];
    }
    mean /= static_cast<double>(window);

    double var = 0.0;
    for (std::size_t j = i + 1 - window; j <= i; ++j) {
      const double d = values[j] - mean;
      var += d * d;
    }
    var /= static_cast<double>(window);

    out[i] = std::sqrt(var);
  }

  return out;
}  // compute mean and variance per window (O(n*window)).
  // FLAG ::: likely need to optimize this, will assess later
  // use Welford's algo / rolling variance (?)

} // namespace qe

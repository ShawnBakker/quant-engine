#include "qe/report.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace qe {

double compute_win_rate(const std::vector<double>& strat_returns) {
  if (strat_returns.empty()) {
    return 0.0;
  }

  std::size_t wins = 0;
  std::size_t total = 0;

  for (double r : strat_returns) {
    // Ignore NaNs if ever appear
    if (std::isnan(r)) {
      continue;
    }
    ++total;
    if (r > 0.0) {
      ++wins;
    }
  }

  if (total == 0) {
    return 0.0;
  }
  return static_cast<double>(wins) / static_cast<double>(total);
}

void write_equity_csv(const std::string& path, const std::vector<double>& equity) {
  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("failed to open file for writing: " + path);
  }

  out << "index,equity\n";
  out << std::setprecision(17);

  for (std::size_t i = 0; i < equity.size(); ++i) {
    out << i << "," << equity[i] << "\n";
  }
}

static std::string json_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);

  for (char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:   out += c;      break;
    }
  }

  return out;
}

void write_report_json(
  const std::string& path,
  const std::string& strategy_name,
  std::size_t fast_window,
  std::size_t slow_window,
  double initial_equity,
  const BacktestResult& result
) {
  std::ofstream out(path);
  if (!out.is_open()) {
    throw std::runtime_error("failed to open file for writing: " + path);
  }

  const double win_rate = compute_win_rate(result.strat_ret);

  out << std::setprecision(17);
  out << "{\n";
  out << "  \"strategy\": \"" << json_escape(strategy_name) << "\",\n";
  out << "  \"params\": {\n";
  out << "    \"fast_window\": " << fast_window << ",\n";
  out << "    \"slow_window\": " << slow_window << ",\n";
  out << "    \"initial_equity\": " << initial_equity << "\n";
  out << "  },\n";
  out << "  \"metrics\": {\n";
  out << "    \"total_return\": " << result.total_return << ",\n";
  out << "    \"sharpe\": " << result.sharpe << ",\n";
  out << "    \"max_drawdown\": " << result.max_drawdown << ",\n";
  out << "    \"win_rate\": " << win_rate << "\n";
  out << "  },\n";
  out << "  \"series\": {\n";
  out << "    \"n_steps\": " << result.equity.size() << "\n";
  out << "  }\n";
  out << "}\n";
}

} // qe

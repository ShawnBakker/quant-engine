#include "qe/report.hpp"

#include <boost/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace qe {

namespace json = boost::json;

// helpers (I9)

static boost::json::array to_json_array(const std::vector<double>& v) {
  boost::json::array a;
  a.reserve(v.size());

  for (double x : v) {
    // keep NaN as null in JSON
    if (std::isnan(x)) {
      a.emplace_back(nullptr);
    } else {
      a.emplace_back(x);
    }
  }

  return a;
}

// report writer (I6 + 9)

void write_report_json(const std::string& path,
                       const std::string& strategy,
                       std::size_t fast,
                       std::size_t slow,
                       double initial,
                       const BacktestResult& r) {
  boost::json::object root;

  // top-level metadata
  root["strategy"] = strategy;

  boost::json::object params;
  params["fast"] = static_cast<std::int64_t>(fast);
  params["slow"] = static_cast<std::int64_t>(slow);
  params["initial"] = initial;
  root["params"] = std::move(params);

  // summary stats
  boost::json::object stats;
  stats["total_return"] = r.total_return;
  stats["sharpe"] = r.sharpe;
  stats["max_drawdown"] = r.max_drawdown;
  stats["trades"] = static_cast<std::int64_t>(r.n_trades);
  stats["total_cost"] = r.total_cost;
  root["stats"] = std::move(stats);

  // series
  boost::json::object series;

  // keep series optional-ish , some runs might omit
  if (!r.equity.empty()) {
    series["equity"] = to_json_array(r.equity);
    series["final_equity"] = r.equity.back();
  }

  if (!r.strat_ret.empty()) {
    series["strategy_returns"] = to_json_array(r.strat_ret);
  }

  root["series"] = std::move(series);

  // write
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open report path for write: " + path);
  }

  // compact JSON is fine , pretty printing is optional and version sensitive
  out << boost::json::serialize(root) << "\n";
}

} //qe

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>
#include <cstdlib>   // std::getenv
#include <cstdio>    // _popen, _pclose
#include <chrono>    // temp filename uniqueness

#include <boost/json.hpp>

#include "qe/config.hpp"
#include "qe/version.hpp"
#include "qe/csv_reader.hpp"
#include "qe/indicators.hpp"
#include "qe/backtest.hpp"
#include "qe/report.hpp"
#include "qe/equity_io.hpp"
#include "qe/options.hpp"

namespace json = boost::json;

static void print_usage() {
  std::cout << "qe_cli\n";
  std::cout << "Usage:\n";
  std::cout << "  qe_cli --version\n";
  std::cout << "  qe_cli run --data <csv_path>\n";
  std::cout << "  qe_cli indicators --data <csv_path> [--window N]\n";
  std::cout << "  qe_cli backtest --data <csv_path> "
               "[--config cfg.json] "
               "[--fast N] [--slow N] [--initial X] "
               "[--fee-bps N] [--slip-bps N] "
               "[--out <dir>]\n";
  std::cout << "  qe_cli options --S <spot> --K <strike> --r <rate> --sigma <vol> --T <years>\n";
  std::cout << "\n";
  std::cout << "Optional env:\n";
  std::cout << "  QE_API_URL=http://localhost:8787   (default)\n";
}

static std::string get_env_or(const char* key, const std::string& def) {
  if (const char* v = std::getenv(key)) {
    if (*v) return std::string(v);
  }
  return def;
}

static std::string read_all_from_pipe(const std::string& cmdline) {
  std::string out;
  FILE* pipe = _popen(cmdline.c_str(), "r");
  if (!pipe) return out;

  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) {
    out += buf;
  }
  _pclose(pipe);
  return out;
}

static bool write_text_file(const std::filesystem::path& p, const std::string& text) {
  std::ofstream out(p, std::ios::binary);
  if (!out) return false;
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  return static_cast<bool>(out);
}

static std::filesystem::path make_temp_json_path(const std::string& stem) {
  std::error_code ec;
  auto dir = std::filesystem::temp_directory_path(ec);
  if (ec) dir = std::filesystem::current_path();

  const auto now =
    static_cast<unsigned long long>(
      std::chrono::high_resolution_clock::now()
        .time_since_epoch()
        .count()
    );

  const auto filename = stem + "_" + std::to_string(now) + ".json";
  return dir / filename;
}

static std::optional<std::string> api_post_json_file(const std::string& url, const std::filesystem::path& json_path) {
  std::ostringstream cmd;
  cmd << "curl.exe -sS -f -X POST "
      << "\"" << url << "\" "
      << "-H \"Content-Type: application/json\" "
      << "--data-binary \"@" << json_path.string() << "\"";

  const std::string out = read_all_from_pipe(cmd.str());
  if (out.empty()) return std::nullopt;
  return out;
}

static void api_record_run_only(
    const std::string& api_base,
    const std::string& engine_version,
    const std::string& command,
    const std::string& status,
    const json::object& args_json,
    const std::string& data_ref,
    const std::string& out_dir,
    const std::optional<std::string>& error_msg
) {
  json::object run_body;
  run_body["engine_version"] = engine_version;
  run_body["command"] = command;
  run_body["status"] = status;
  run_body["args_json"] = args_json;
  run_body["data_ref"] = data_ref;
  run_body["out_dir"] = out_dir;
  if (error_msg) run_body["error"] = *error_msg;

  const auto run_path = make_temp_json_path("qe_run");
  if (!write_text_file(run_path, json::serialize(run_body))) {
    std::cerr << "[api] warn: failed to write temp run json: " << run_path.string() << "\n";
    return;
  }

  const std::string run_url = api_base + "/runs";
  auto run_resp = api_post_json_file(run_url, run_path);

  std::error_code ec_rm;
  std::filesystem::remove(run_path, ec_rm);

  if (!run_resp) {
    std::cerr << "[api] warn: failed to POST /runs\n";
    return;
  }

  boost::system::error_code ec;
  json::value v = json::parse(*run_resp, ec);
  if (ec || !v.is_object()) {
    std::cerr << "[api] warn: could not parse /runs response\n";
    return;
  }

  const json::object& obj = v.as_object();
  const json::value* idv = obj.if_contains("id");
  if (!idv || !idv->is_string()) {
    std::cerr << "[api] warn: /runs response missing id\n";
    return;
  }

  std::cout << "[api] recorded run_id=" << std::string(idv->as_string()) << "\n";
}

static void api_record_backtest_success(
    const std::string& api_base,
    const std::string& data_ref,
    const std::string& out_dir,
    const qe::BacktestConfig& cfg,
    const qe::BacktestResult& r
) {
  // 1) Create run
  json::object args;
  args["strategy"] = cfg.strategy;
  args["fast"] = static_cast<std::int64_t>(cfg.fast);
  args["slow"] = static_cast<std::int64_t>(cfg.slow);
  args["initial"] = cfg.initial;
  args["fee_bps"] = cfg.fee_bps;
  args["slippage_bps"] = cfg.slippage_bps;

  json::object run_body;
  run_body["engine_version"] = qe::version();
  run_body["command"] = "backtest";
  run_body["status"] = "success";
  run_body["args_json"] = args;
  run_body["data_ref"] = data_ref;
  run_body["out_dir"] = out_dir;

  const auto run_path = make_temp_json_path("qe_run");
  if (!write_text_file(run_path, json::serialize(run_body))) {
    std::cerr << "[api] warn: failed to write temp run json: " << run_path.string() << "\n";
    return;
  }

  const std::string run_url = api_base + "/runs";
  auto run_resp = api_post_json_file(run_url, run_path);
  std::error_code ec_rm;
  std::filesystem::remove(run_path, ec_rm);

  if (!run_resp) {
    std::cerr << "[api] warn: failed to POST /runs\n";
    return;
  }

  boost::system::error_code ec;
  json::value v = json::parse(*run_resp, ec);
  if (ec || !v.is_object()) {
    std::cerr << "[api] warn: could not parse /runs response\n";
    return;
  }

  const json::object& obj = v.as_object();
  const json::value* idv = obj.if_contains("id");
  if (!idv || !idv->is_string()) {
    std::cerr << "[api] warn: /runs response missing id\n";
    return;
  }
  const std::string run_id = std::string(idv->as_string());

  // 2) Upsert metrics
  json::object metrics;
  metrics["total_return"] = r.total_return;
  metrics["sharpe"] = r.sharpe;
  metrics["max_drawdown"] = r.max_drawdown;
  metrics["win_rate"] = qe::compute_win_rate(r.strat_ret);
  metrics["n_trades"] = static_cast<std::int64_t>(r.n_trades);
  metrics["total_cost"] = r.total_cost;
  metrics["final_equity"] = r.equity.empty() ? 0.0 : r.equity.back();

  const auto metrics_path = make_temp_json_path("qe_metrics");
  if (!write_text_file(metrics_path, json::serialize(metrics))) {
    std::cerr << "[api] warn: failed to write temp metrics json: " << metrics_path.string() << "\n";
    return;
  }

  const std::string metrics_url = api_base + "/runs/" + run_id + "/metrics";
  auto metrics_resp = api_post_json_file(metrics_url, metrics_path);
  std::filesystem::remove(metrics_path, ec_rm);

  if (!metrics_resp) {
    std::cerr << "[api] warn: failed to POST /runs/:id/metrics for run_id=" << run_id << "\n";
    return;
  }

  std::cout << "[api] recorded run_id=" << run_id << "\n";
}

static void api_record_backtest_failure(
    const std::string& api_base,
    const std::string& data_ref,
    const std::string& out_dir,
    const qe::BacktestConfig& cfg,
    const std::string& error_msg
) {
  json::object args;
  args["strategy"] = cfg.strategy;
  args["fast"] = static_cast<std::int64_t>(cfg.fast);
  args["slow"] = static_cast<std::int64_t>(cfg.slow);
  args["initial"] = cfg.initial;
  args["fee_bps"] = cfg.fee_bps;
  args["slippage_bps"] = cfg.slippage_bps;

  api_record_run_only(
    api_base,
    qe::version(),
    "backtest",
    "failed",
    args,
    data_ref,
    out_dir,
    error_msg
  );
}

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string cmd = argv[1];

    // Version
    if (cmd == "--version" || cmd == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }

    const std::string api_base = get_env_or("QE_API_URL", "http://localhost:8787");

    // Run (CSV ingestion check)
    if (cmd == "run") {
      std::string data_path;

      for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      try {
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);
        std::cout << "Loaded " << table.size()
                  << " rows from " << data_path << "\n";
      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }

    // Indicators
    if (cmd == "indicators") {
      std::string data_path;
      std::size_t window = 5;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        } else if (arg == "--window" && i + 1 < argc) {
          window = static_cast<std::size_t>(std::stoul(argv[++i]));
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      try {
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);

        std::vector<double> returns = qe::compute_returns(table);
        std::vector<double> mean = qe::rolling_mean(returns, window);
        std::vector<double> stddev = qe::rolling_std(returns, window);

        std::cout << "rows=" << table.size()
                  << " returns=" << returns.size()
                  << " window=" << window << "\n";

        std::size_t start = returns.size() > 5 ? returns.size() - 5 : 0;
        for (std::size_t i = start; i < returns.size(); ++i) {
          std::cout << "i=" << i
                    << " ret=" << returns[i]
                    << " mean=" << mean[i]
                    << " std=" << stddev[i] << "\n";
        }

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
      }

      return 0;
    }

    // options pricing (Black-Scholes, no dividends) + record run
    if (cmd == "options") {
      std::optional<double> S;
      std::optional<double> K;
      std::optional<double> r;
      std::optional<double> sigma;
      std::optional<double> T;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--S" && i + 1 < argc) {
          S = std::stod(argv[++i]);
        } else if (arg == "--K" && i + 1 < argc) {
          K = std::stod(argv[++i]);
        } else if (arg == "--r" && i + 1 < argc) {
          r = std::stod(argv[++i]);
        } else if (arg == "--sigma" && i + 1 < argc) {
          sigma = std::stod(argv[++i]);
        } else if (arg == "--T" && i + 1 < argc) {
          T = std::stod(argv[++i]);
        }
      }

      if (!S || !K || !r || !sigma || !T) {
        std::cerr << "Error: options requires --S --K --r --sigma --T\n";
        std::cerr << "Example: qe_cli options --S 100 --K 110 --r 0.05 --sigma 0.2 --T 0.5\n";
        return 1;
      }

      // record args (plus result)
      json::object args;
      args["S"] = *S;
      args["K"] = *K;
      args["r"] = *r;
      args["sigma"] = *sigma;
      args["T"] = *T;

      try {
        const double call = qe::black_scholes_call(*S, *K, *r, *sigma, *T);
        const double put  = qe::black_scholes_put(*S, *K, *r, *sigma, *T);

        const double dc = qe::bs_delta_call(*S, *K, *r, *sigma, *T);
        const double dp = qe::bs_delta_put(*S, *K, *r, *sigma, *T);
        const double v  = qe::bs_vega(*S, *K, *r, *sigma, *T);

        json::object result;
        result["call"] = call;
        result["put"] = put;
        result["delta_call"] = dc;
        result["delta_put"] = dp;
        result["vega"] = v;

        args["result"] = result;

        std::cout << "options: black_scholes"
                  << " S=" << *S
                  << " K=" << *K
                  << " r=" << *r
                  << " sigma=" << *sigma
                  << " T=" << *T << "\n";

        std::cout << "call=" << call
                  << " put=" << put << "\n";

        std::cout << "delta_call=" << dc
                  << " delta_put=" << dp
                  << " vega=" << v << "\n";

        // record run to API (best-effort)
        api_record_run_only(
          api_base,
          qe::version(),
          "options",
          "success",
          args,
          "",     // data_ref
          "",     // out_dir
          std::nullopt
        );

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";

        // record failure
        api_record_run_only(
          api_base,
          qe::version(),
          "options",
          "failed",
          args,
          "",
          "",
          std::string(ex.what())
        );

        return 1;
      }

      return 0;
    }

    // Backtest with config + stale-output protection + API run recording
    if (cmd == "backtest") {
      std::string data_path;
      std::string config_path;
      std::string out_dir;

      std::optional<std::size_t> fast_override;
      std::optional<std::size_t> slow_override;
      std::optional<double> initial_override;
      std::optional<double> fee_override;
      std::optional<double> slip_override;

      for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
          data_path = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
          config_path = argv[++i];
        } else if (arg == "--fast" && i + 1 < argc) {
          fast_override = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--slow" && i + 1 < argc) {
          slow_override = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--initial" && i + 1 < argc) {
          initial_override = std::stod(argv[++i]);
        } else if (arg == "--fee-bps" && i + 1 < argc) {
          fee_override = std::stod(argv[++i]);
        } else if (arg == "--slip-bps" && i + 1 < argc) {
          slip_override = std::stod(argv[++i]);
        } else if (arg == "--out" && i + 1 < argc) {
          out_dir = argv[++i];
        }
      }

      if (data_path.empty()) {
        std::cerr << "Error: --data <csv_path> is required\n";
        return 1;
      }

      // defaults
      qe::BacktestConfig cfg{};

      // config.json if provided
      if (!config_path.empty()) {
        try {
          cfg = qe::load_backtest_config_json(config_path);
        } catch (const std::exception& ex) {
          std::cerr << "Error: " << ex.what() << "\n";
          api_record_backtest_failure(api_base, data_path, out_dir, cfg, ex.what());
          return 1;
        }
      }

      // CLI overrides win
      if (fast_override) cfg.fast = *fast_override;
      if (slow_override) cfg.slow = *slow_override;
      if (initial_override) cfg.initial = *initial_override;
      if (fee_override) cfg.fee_bps = *fee_override;
      if (slip_override) cfg.slippage_bps = *slip_override;

      // output dir and remove stale artifacts
      std::string equity_path;
      std::string report_path;
      if (!out_dir.empty()) {
        std::filesystem::create_directories(out_dir);
        equity_path = (std::filesystem::path(out_dir) / "equity.csv").string();
        report_path = (std::filesystem::path(out_dir) / "report.json").string();

        std::error_code ec;
        std::filesystem::remove(equity_path, ec);
        ec.clear();
        std::filesystem::remove(report_path, ec);
      }

      try {
        qe::OhlcvTable table = qe::read_ohlcv_csv(data_path);

        qe::BacktestCosts costs{ cfg.fee_bps, cfg.slippage_bps };

        qe::BacktestResult r =
          qe::backtest_sma_crossover(table, cfg.fast, cfg.slow, cfg.initial, costs);

        std::cout << "backtest: " << cfg.strategy
                  << " fast=" << cfg.fast
                  << " slow=" << cfg.slow
                  << " initial=" << cfg.initial
                  << " fee_bps=" << cfg.fee_bps
                  << " slip_bps=" << cfg.slippage_bps << "\n";

        std::cout << "total_return=" << r.total_return
                  << " sharpe=" << r.sharpe
                  << " max_drawdown=" << r.max_drawdown
                  << " win_rate=" << qe::compute_win_rate(r.strat_ret) << "\n";

        std::cout << "trades=" << r.n_trades
                  << " total_cost=" << r.total_cost << "\n";

        if (!r.equity.empty()) {
          std::cout << "final_equity=" << r.equity.back() << "\n";
        }

        if (!out_dir.empty()) {
          qe::write_equity_csv(equity_path, r.equity);
          qe::write_report_json(report_path, cfg.strategy, cfg.fast, cfg.slow, cfg.initial, r);
          std::cout << "wrote " << equity_path << "\n";
          std::cout << "wrote " << report_path << "\n";
        }

        // Record to API/DB (best-effort)
        api_record_backtest_success(api_base, data_path, out_dir, cfg, r);

      } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        api_record_backtest_failure(api_base, data_path, out_dir, cfg, ex.what());
        return 1;
      }

      return 0;
    }
  }

  print_usage();
  return 0;
}

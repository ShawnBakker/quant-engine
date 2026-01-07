#include "qe/config.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

namespace qe {

namespace json = boost::json;

static std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open config path for read: " + path);
  }

  std::string s;
  in.seekg(0, std::ios::end);
  s.resize(static_cast<std::size_t>(in.tellg()));
  in.seekg(0, std::ios::beg);
  in.read(s.data(), static_cast<std::streamsize>(s.size()));
  return s;
}

//tolerate UTF-8 BOM - a powershell Set-Content issue
static void strip_utf8_bom(std::string& s) {
  if (s.size() >= 3 &&
      static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF) {
    s.erase(0, 3);
  }
}

static const json::value* find_key(const json::object& obj, const char* k) {
  return obj.if_contains(k);
}

static std::string get_str(const json::object& obj, const char* k) {
  const json::value* v = find_key(obj, k);
  if (!v || !v->is_string()) {
    throw std::runtime_error(std::string("config key must be string: ") + k);
  }
  return std::string(v->as_string());
}

static std::int64_t get_i64(const json::object& obj, const char* k) {
  const json::value* v = find_key(obj, k);
  if (!v || !v->is_int64()) {
    throw std::runtime_error(std::string("config key must be int: ") + k);
  }
  return v->as_int64();
}

static double get_num(const json::object& obj, const char* k) {
  const json::value* v = find_key(obj, k);
  if (!v || !(v->is_double() || v->is_int64())) {
    throw std::runtime_error(std::string("config key must be number: ") + k);
  }
  return v->is_double() ? v->as_double()
                        : static_cast<double>(v->as_int64());
}

static void maybe_set_size(json::object const& obj, const char* k, std::size_t& out) {
  const json::value* v = find_key(obj, k);
  if (!v) return;
  if (!v->is_int64()) {
    throw std::runtime_error(std::string("config key must be int: ") + k);
  }
  const auto x = v->as_int64();
  if (x <= 0) {
    throw std::runtime_error(std::string("config key must be > 0: ") + k);
  }
  out = static_cast<std::size_t>(x);
}

static void maybe_set_num(json::object const& obj, const char* k, double& out) {
  const json::value* v = find_key(obj, k);
  if (!v) return;
  out = get_num(obj, k);
}

BacktestConfig load_backtest_config_json(const std::string& path) {
  BacktestConfig cfg{};
  std::string text = read_file(path);
  strip_utf8_bom(text);

  boost::system::error_code ec;
  json::value v = json::parse(text, ec);
  if (ec) {
    throw std::runtime_error(
      std::string("config parse failed for '") + path + "': " + ec.message()
    );
  }

  if (!v.is_object()) {
    throw std::runtime_error("config root must be an object");
  }

  const json::object& root = v.as_object();

  // strategy at root
  if (find_key(root, "strategy")) {
    cfg.strategy = get_str(root, "strategy");
  }

  // Support both schemas:
  // A) nested:
  // {
  //   "strategy": "...",
  //   "params": { "fast": 5, "slow": 20, "initial": 1.0, "costs": {...} }
  // }
  // B) flat (legacy):
  // { "strategy": "...", "fast": 5, "slow": 20, "initial": 1.0, "fee_bps": 0.0, ... }
  const json::object* params_obj = &root;
  if (const json::value* pv = find_key(root, "params")) {
    if (!pv->is_object()) {
      throw std::runtime_error("config key must be object: params");
    }
    params_obj = &pv->as_object();
  }

  maybe_set_size(*params_obj, "fast", cfg.fast);
  maybe_set_size(*params_obj, "fast_window", cfg.fast);

  maybe_set_size(*params_obj, "slow", cfg.slow);
  maybe_set_size(*params_obj, "slow_window", cfg.slow);

  if (find_key(*params_obj, "initial"))
    cfg.initial = get_num(*params_obj, "initial");
  if (find_key(*params_obj, "initial_equity"))
    cfg.initial = get_num(*params_obj, "initial_equity");

  // costs: nested or flat
  const json::object* costs_obj = nullptr;
  if (const json::value* cv = find_key(*params_obj, "costs")) {
    if (!cv->is_object()) {
      throw std::runtime_error("config key must be object: costs");
    }
    costs_obj = &cv->as_object();
  } else if (params_obj == &root) {
    costs_obj = &root;
  }

  if (costs_obj) {
    maybe_set_num(*costs_obj, "fee_bps", cfg.fee_bps);
    maybe_set_num(*costs_obj, "slippage_bps", cfg.slippage_bps);
    maybe_set_num(*costs_obj, "slip_bps", cfg.slippage_bps);
  }

  // validation
  if (cfg.fast == 0 || cfg.slow == 0) {
    throw std::runtime_error("fast and slow must be > 0");
  }
  if (cfg.slow <= cfg.fast) {
    throw std::runtime_error("slow must be greater than fast");
  }

  return cfg;
}

} 

#include "qe/config.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

namespace qe {

namespace json = boost::json;

// helpers


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

static bool has_obj_key(const json::object& obj, const char* k) {
  return obj.if_contains(k) != nullptr;
}

static std::int64_t get_i64(const json::object& obj, const char* k) {
  const json::value* v = obj.if_contains(k);
  if (!v || !v->is_int64()) {
    throw std::runtime_error(std::string("config key must be int: ") + k);
  }
  return v->as_int64();
}

static double get_num(const json::object& obj, const char* k) {
  const json::value* v = obj.if_contains(k);
  if (!v || !(v->is_double() || v->is_int64())) {
    throw std::runtime_error(std::string("config key must be number: ") + k);
  }
  return v->is_double() ? v->as_double() : static_cast<double>(v->as_int64());
}

static std::string get_str(const json::object& obj, const char* k) {
  const json::value* v = obj.if_contains(k);
  if (!v || !v->is_string()) {
    throw std::runtime_error(std::string("config key must be string: ") + k);
  }
  return std::string(v->as_string());
}

BacktestConfig load_backtest_config_json(const std::string& path) {
  BacktestConfig cfg;

  const std::string text = read_file(path);

  json::error_code ec;
  json::value v = json::parse(text, ec);
  if (ec) {
    throw std::runtime_error("failed to parse config JSON");
  }

  if (!v.is_object()) {
    throw std::runtime_error("config root must be an object");
  }

  const json::object& root = v.as_object();

  if (has_obj_key(root, "strategy")) {
    cfg.strategy = get_str(root, "strategy");
  }

  if (has_obj_key(root, "fast")) {
    cfg.fast = static_cast<std::size_t>(get_i64(root, "fast"));
  }

  if (has_obj_key(root, "slow")) {
    cfg.slow = static_cast<std::size_t>(get_i64(root, "slow"));
  }

  if (has_obj_key(root, "initial")) {
    cfg.initial = get_num(root, "initial");
  }

  if (has_obj_key(root, "fee_bps")) {
    cfg.fee_bps = get_num(root, "fee_bps");
  }

  if (has_obj_key(root, "slippage_bps")) {
    cfg.slippage_bps = get_num(root, "slippage_bps");
  }

  return cfg;
}

} 

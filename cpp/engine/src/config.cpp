#include "qe/config.hpp"

#include <boost/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

namespace qe {
namespace json = boost::json;


// helpers


static std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("failed to open config path for read: " + path);

  std::string s((std::istreambuf_iterator<char>(in)),
                 std::istreambuf_iterator<char>());

  // Strip UTF-8 BOM 
  if (s.size() >= 3 &&
      static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF) {
    s.erase(0, 3);
  }

  return s;
}

static double get_number(const json::object& obj, const char* k) {
  const auto* v = obj.if_contains(k);
  if (!v || !(v->is_double() || v->is_int64()))
    throw std::runtime_error(std::string("config key must be number: ") + k);
  return v->is_double() ? v->as_double()
                        : static_cast<double>(v->as_int64());
}


// Loader


BacktestConfig load_backtest_config_json(const std::string& path) {
  BacktestConfig cfg;

  const std::string text = read_file(path);

  json::error_code ec;
  json::value v = json::parse(text, ec);
  if (ec)
    throw std::runtime_error("failed to parse config JSON");

  if (!v.is_object())
    throw std::runtime_error("config root must be an object");

  const json::object& root = v.as_object();

  //Flat layout 
  if (auto* p = root.if_contains("fast"))
    cfg.fast = static_cast<std::size_t>(get_number(root, "fast"));

  if (auto* p = root.if_contains("slow"))
    cfg.slow = static_cast<std::size_t>(get_number(root, "slow"));

  if (auto* p = root.if_contains("initial"))
    cfg.initial = get_number(root, "initial");

  if (auto* p = root.if_contains("fee_bps"))
    cfg.fee_bps = get_number(root, "fee_bps");

  if (auto* p = root.if_contains("slippage_bps"))
    cfg.slippage_bps = get_number(root, "slippage_bps");

  //Nested 
  if (auto* params = root.if_contains("params")) {
    if (!params->is_object())
      throw std::runtime_error("'params' must be an object");

    const auto& p = params->as_object();

    if (auto* v = p.if_contains("fast"))
      cfg.fast = static_cast<std::size_t>(get_number(p, "fast"));

    if (auto* v = p.if_contains("slow"))
      cfg.slow = static_cast<std::size_t>(get_number(p, "slow"));

    if (auto* v = p.if_contains("initial"))
      cfg.initial = get_number(p, "initial");

    if (auto* c = p.if_contains("costs")) {
      if (!c->is_object())
        throw std::runtime_error("'costs' must be an object");

      const auto& co = c->as_object();

      if (auto* v = co.if_contains("fee_bps"))
        cfg.fee_bps = get_number(co, "fee_bps");

      if (auto* v = co.if_contains("slippage_bps"))
        cfg.slippage_bps = get_number(co, "slippage_bps");
    }
  }

  return cfg;
}

} 

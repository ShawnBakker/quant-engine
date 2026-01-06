#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "qe/config.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static fs::path write_temp_file(const std::string& contents) {
  fs::path p = fs::temp_directory_path() / "qe_test_config.json";
  std::ofstream out(p, std::ios::binary);
  out << contents;
  out.close();
  return p;
}

TEST_CASE("config: load_backtest_config_json loads flat config", "[config]") {
  const std::string json = R"json(
{
  "strategy": "sma_crossover",
  "fast": 7,
  "slow": 21,
  "initial": 100.0,
  "fee_bps": 1.5,
  "slippage_bps": 2.0
}
)json";

  const fs::path p = write_temp_file(json);
  qe::BacktestConfig cfg = qe::load_backtest_config_json(p.string());

  REQUIRE(cfg.strategy == "sma_crossover");
  REQUIRE(cfg.fast == 7);
  REQUIRE(cfg.slow == 21);
  REQUIRE(cfg.initial == Catch::Approx(100.0));
  REQUIRE(cfg.fee_bps == Catch::Approx(1.5));
  REQUIRE(cfg.slippage_bps == Catch::Approx(2.0));
}

TEST_CASE("config: load_backtest_config_json loads nested config", "[config]") {
  const std::string json = R"json(
{
  "strategy": "sma_crossover",
  "params": {
    "fast": 10,
    "slow": 50,
    "initial": 10000,
    "costs": { "fee_bps": 0.5, "slippage_bps": 1.0 }
  }
}
)json";

  const fs::path p = write_temp_file(json);
  qe::BacktestConfig cfg = qe::load_backtest_config_json(p.string());

  REQUIRE(cfg.strategy == "sma_crossover");
  REQUIRE(cfg.fast == 10);
  REQUIRE(cfg.slow == 50);
  REQUIRE(cfg.initial == Catch::Approx(10000.0));
  REQUIRE(cfg.fee_bps == Catch::Approx(0.5));
  REQUIRE(cfg.slippage_bps == Catch::Approx(1.0));
}

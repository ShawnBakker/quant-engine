#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "qe/options.hpp"

TEST_CASE("options: put-call parity holds approximately", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.05;
  const double sigma = 0.2;
  const double T = 0.5;

  const double C = qe::black_scholes_call(S, K, r, sigma, T);
  const double P = qe::black_scholes_put(S, K, r, sigma, T);

  const double rhs = S - K * std::exp(-r * T);
  REQUIRE((C - P) == Catch::Approx(rhs).margin(1e-10));
}

TEST_CASE("options: call price increases with spot", "[options]") {
  const double K = 100.0;
  const double r = 0.01;
  const double sigma = 0.2;
  const double T = 1.0;

  const double c1 = qe::black_scholes_call(90.0, K, r, sigma, T);
  const double c2 = qe::black_scholes_call(110.0, K, r, sigma, T);

  REQUIRE(c2 > c1);
}

TEST_CASE("options: call price increases with volatility", "[options]") {
  const double S = 100.0;
  const double K = 100.0;
  const double r = 0.01;
  const double T = 1.0;

  const double c1 = qe::black_scholes_call(S, K, r, 0.10, T);
  const double c2 = qe::black_scholes_call(S, K, r, 0.30, T);

  REQUIRE(c2 > c1);
}

TEST_CASE("options: delta is in expected ranges", "[options]") {
  const double S = 100.0;
  const double K = 100.0;
  const double r = 0.01;
  const double sigma = 0.2;
  const double T = 1.0;

  const double dc = qe::bs_delta_call(S, K, r, sigma, T);
  const double dp = qe::bs_delta_put(S, K, r, sigma, T);

  REQUIRE(dc >= 0.0);
  REQUIRE(dc <= 1.0);
  REQUIRE(dp <= 0.0);
  REQUIRE(dp >= -1.0);
}

TEST_CASE("options: vega is positive", "[options]") {
  const double S = 100.0;
  const double K = 100.0;
  const double r = 0.01;
  const double sigma = 0.2;
  const double T = 1.0;

  const double v = qe::bs_vega(S, K, r, sigma, T);
  REQUIRE(v > 0.0);
}

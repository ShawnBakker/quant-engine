#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "qe/options.hpp"

static double fd_d1(double (*f)(double), double x, double h) {
  return (f(x + h) - f(x - h)) / (2.0 * h);
}

static double fd_d2(double (*f)(double), double x, double h) {
  return (f(x + h) - 2.0 * f(x) + f(x - h)) / (h * h);
}

TEST_CASE("options: put-call parity holds approximately", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.05;
  const double sigma = 0.2;
  const double T = 0.5;

  const double C = qe::black_scholes_call(S, K, r, sigma, T);
  const double P = qe::black_scholes_put(S, K, r, sigma, T);

  const double rhs = qe::put_call_parity_rhs(S, K, r, T);
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

TEST_CASE("options: delta ranges and sign", "[options]") {
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

TEST_CASE("options: gamma is positive", "[options]") {
  const double S = 100.0;
  const double K = 100.0;
  const double r = 0.01;
  const double sigma = 0.2;
  const double T = 1.0;

  const double g = qe::bs_gamma(S, K, r, sigma, T);
  REQUIRE(g > 0.0);
}

TEST_CASE("options: rho signs (typical positive rates)", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.05;
  const double sigma = 0.2;
  const double T = 0.5;

  const double rc = qe::bs_rho_call(S, K, r, sigma, T);
  const double rp = qe::bs_rho_put(S, K, r, sigma, T);

  REQUIRE(rc > 0.0);
  REQUIRE(rp < 0.0);
}

TEST_CASE("options: finite-difference checks for delta/gamma/vega", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.03;
  const double sigma = 0.25;
  const double T = 0.75;

  // delta/gamma vs finite differences wrt S
  const double hS = 1e-3 * S;

  auto call_S = [&](double s) { return qe::black_scholes_call(s, K, r, sigma, T); };

  const double dc_fd = (call_S(S + hS) - call_S(S - hS)) / (2.0 * hS);
  const double gc_fd = (call_S(S + hS) - 2.0 * call_S(S) + call_S(S - hS)) / (hS * hS);

  const double dc = qe::bs_delta_call(S, K, r, sigma, T);
  const double gc = qe::bs_gamma(S, K, r, sigma, T);

  REQUIRE(dc == Catch::Approx(dc_fd).epsilon(1e-6));
  REQUIRE(gc == Catch::Approx(gc_fd).epsilon(1e-5));

  // vega vs finite differences wrt sigma
  const double hV = 1e-4;

  auto call_sig = [&](double sig) { return qe::black_scholes_call(S, K, r, sig, T); };

  const double v_fd = (call_sig(sigma + hV) - call_sig(sigma - hV)) / (2.0 * hV);
  const double v = qe::bs_vega(S, K, r, sigma, T);

  REQUIRE(v == Catch::Approx(v_fd).epsilon(1e-6));
}

TEST_CASE("options: implied vol round-trip call", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.05;
  const double sigma = 0.2;
  const double T = 0.5;

  const double C = qe::black_scholes_call(S, K, r, sigma, T);
  const double iv = qe::implied_vol_call(C, S, K, r, T);

  REQUIRE(iv == Catch::Approx(sigma).epsilon(1e-10));
}

TEST_CASE("options: implied vol round-trip put", "[options]") {
  const double S = 100.0;
  const double K = 110.0;
  const double r = 0.05;
  const double sigma = 0.2;
  const double T = 0.5;

  const double P = qe::black_scholes_put(S, K, r, sigma, T);
  const double iv = qe::implied_vol_put(P, S, K, r, T);

  REQUIRE(iv == Catch::Approx(sigma).epsilon(1e-10));
}

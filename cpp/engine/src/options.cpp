#include "qe/options.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace qe {

static void require_finite(const char* name, double x) {
  if (!std::isfinite(x)) {
    throw std::runtime_error(std::string("options: ") + name + " must be finite");
  }
}

static void require_positive(const char* name, double x) {
  require_finite(name, x);
  if (x <= 0.0) {
    throw std::runtime_error(std::string("options: ") + name + " must be > 0");
  }
}

static double norm_pdf(double x) {
  static constexpr double inv_sqrt_2pi = 0.39894228040143267793994605993438; // 1/sqrt(2π)
  return inv_sqrt_2pi * std::exp(-0.5 * x * x);
}

double norm_cdf(double x) {
  require_finite("x", x);
  return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

static void validate_inputs(double S, double K, double r, double sigma, double T) {
  require_positive("S", S);
  require_positive("K", K);
  require_finite("r", r);
  require_positive("sigma", sigma);
  require_positive("T", T);
}

static double d1(double S, double K, double r, double sigma, double T) {
  // d1 = [ln(S/K) + (r + 0.5σ²)T] / (σ√T)
  const double vol_sqrt_t = sigma * std::sqrt(T);
  return (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / vol_sqrt_t;
}

static double d2(double d1v, double sigma, double T) {
  return d1v - sigma * std::sqrt(T);
}

double black_scholes_call(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  const double Nd1 = norm_cdf(d1v);
  const double Nd2 = norm_cdf(d2v);
  const double discK = K * std::exp(-r * T);

  return S * Nd1 - discK * Nd2;
}

double black_scholes_put(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  const double Nmd1 = norm_cdf(-d1v);
  const double Nmd2 = norm_cdf(-d2v);
  const double discK = K * std::exp(-r * T);

  return discK * Nmd2 - S * Nmd1;
}

double bs_delta_call(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);
  const double d1v = d1(S, K, r, sigma, T);
  return norm_cdf(d1v);
}

double bs_delta_put(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);
  const double d1v = d1(S, K, r, sigma, T);
  return norm_cdf(d1v) - 1.0;
}

double bs_vega(double S, double K, double r, double sigma, double T) {
  (void)K;
  (void)r;
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  return S * norm_pdf(d1v) * std::sqrt(T);
}

} 

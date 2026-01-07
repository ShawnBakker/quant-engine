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

static void validate_inputs_no_sigma(double S, double K, double r, double T) {
  require_positive("S", S);
  require_positive("K", K);
  require_finite("r", r);
  require_positive("T", T);
}

static double d1(double S, double K, double r, double sigma, double T) {
  const double vol_sqrt_t = sigma * std::sqrt(T);
  return (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / vol_sqrt_t;
}

static double d2(double d1v, double sigma, double T) {
  return d1v - sigma * std::sqrt(T);
}

double put_call_parity_rhs(double S, double K, double r, double T) {
  validate_inputs_no_sigma(S, K, r, T);
  return S - K * std::exp(-r * T);
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

double bs_gamma(double S, double K, double r, double sigma, double T) {
  (void)K;
  (void)r;
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double denom = S * sigma * std::sqrt(T);
  return norm_pdf(d1v) / denom;
}

double bs_vega(double S, double K, double r, double sigma, double T) {
  (void)K;
  (void)r;
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  return S * norm_pdf(d1v) * std::sqrt(T);
}

// Theta per year (T in years)
double bs_theta_call(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  const double term1 = -(S * norm_pdf(d1v) * sigma) / (2.0 * std::sqrt(T));
  const double term2 = -r * K * std::exp(-r * T) * norm_cdf(d2v);
  return term1 + term2;
}

double bs_theta_put(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  const double term1 = -(S * norm_pdf(d1v) * sigma) / (2.0 * std::sqrt(T));
  const double term2 = +r * K * std::exp(-r * T) * norm_cdf(-d2v);
  return term1 + term2;
}

// Rho per 1.0 rate
double bs_rho_call(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  return K * T * std::exp(-r * T) * norm_cdf(d2v);
}

double bs_rho_put(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  const double d1v = d1(S, K, r, sigma, T);
  const double d2v = d2(d1v, sigma, T);

  return -K * T * std::exp(-r * T) * norm_cdf(-d2v);
}

BsResult black_scholes_all(double S, double K, double r, double sigma, double T) {
  validate_inputs(S, K, r, sigma, T);

  BsResult out{};
  out.call = black_scholes_call(S, K, r, sigma, T);
  out.put  = black_scholes_put(S, K, r, sigma, T);

  out.delta_call = bs_delta_call(S, K, r, sigma, T);
  out.delta_put  = bs_delta_put(S, K, r, sigma, T);

  out.gamma = bs_gamma(S, K, r, sigma, T);
  out.vega  = bs_vega(S, K, r, sigma, T);

  out.theta_call = bs_theta_call(S, K, r, sigma, T);
  out.theta_put  = bs_theta_put(S, K, r, sigma, T);

  out.rho_call = bs_rho_call(S, K, r, sigma, T);
  out.rho_put  = bs_rho_put(S, K, r, sigma, T);

  return out;
}

static double intrinsic_call(double S, double K, double r, double T) {
  const double discK = K * std::exp(-r * T);
  return std::max(0.0, S - discK);
}

static double intrinsic_put(double S, double K, double r, double T) {
  const double discK = K * std::exp(-r * T);
  return std::max(0.0, discK - S);
}

static double implied_vol_bisect(
  double market_price,
  double S, double K, double r, double T,
  bool is_call,
  double sigma_lo, double sigma_hi
) {
  validate_inputs_no_sigma(S, K, r, T);
  require_finite("market_price", market_price);

  if (market_price < 0.0) {
    throw std::runtime_error("options: market_price must be >= 0");
  }

  // Basic no-arbitrage-ish bounds (no dividends, discounted strike)
  const double lower = is_call ? intrinsic_call(S, K, r, T) : intrinsic_put(S, K, r, T);
  // Upper bounds: call <= S, put <= K*exp(-rT) + small epsilon (loose, safe)
  const double upper = is_call ? S : (K * std::exp(-r * T));

  if (market_price < lower - 1e-12) {
    throw std::runtime_error("options: market_price below intrinsic bound");
  }
  if (market_price > upper + 1e-12) {
    throw std::runtime_error("options: market_price above theoretical upper bound");
  }

  sigma_lo = std::max(sigma_lo, 1e-12);
  sigma_hi = std::max(sigma_hi, sigma_lo * 2.0);

  auto price_fn = [&](double sig) {
    return is_call ? black_scholes_call(S, K, r, sig, T)
                   : black_scholes_put(S, K, r, sig, T);
  };

  double lo = sigma_lo;
  double hi = sigma_hi;

  double plo = price_fn(lo);
  double phi = price_fn(hi);

  // Expand upper bracket until it contains the market price (or until hi too big)
  int expand = 0;
  while (phi < market_price && expand < 30) {
    hi *= 2.0;
    phi = price_fn(hi);
    ++expand;
    if (hi > 50.0) break;
  }

  if (!(plo <= market_price && market_price <= phi)) {
    throw std::runtime_error("options: implied vol bracket failed (check inputs / price)");
  }

  // Bisection
  const int max_iter = 200;
  const double tol_price = 1e-12;
  const double tol_sigma = 1e-12;

  for (int i = 0; i < max_iter; ++i) {
    const double mid = 0.5 * (lo + hi);
    const double pmid = price_fn(mid);

    const double err = pmid - market_price;
    if (std::abs(err) < tol_price) return mid;
    if ((hi - lo) < tol_sigma) return mid;

    if (pmid < market_price) {
      lo = mid;
    } else {
      hi = mid;
    }
  }

  return 0.5 * (lo + hi);
}

double implied_vol_call(double market_price, double S, double K, double r, double T,
                        double sigma_lo, double sigma_hi) {
  return implied_vol_bisect(market_price, S, K, r, T, true, sigma_lo, sigma_hi);
}

double implied_vol_put(double market_price, double S, double K, double r, double T,
                       double sigma_lo, double sigma_hi) {
  return implied_vol_bisect(market_price, S, K, r, T, false, sigma_lo, sigma_hi);
}

} 

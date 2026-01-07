#pragma once

#include <cstddef>

namespace qe {
    
double norm_cdf(double x);

// Black–Scholes European option pricing (no dividends).
// S: spot price (>0)
// K: strike price (>0)
// r: continuously compounded risk-free rate (can be negative)
// sigma: volatility (>0)
// T: time to expiry in years (>0)
double black_scholes_call(double S, double K, double r, double sigma, double T);
double black_scholes_put(double S, double K, double r, double sigma, double T);

// Put–call parity helper (no dividends):
// C - P = S - K * exp(-rT)
double put_call_parity_rhs(double S, double K, double r, double T);

// Greeks (no dividends)
// delta
double bs_delta_call(double S, double K, double r, double sigma, double T);
double bs_delta_put(double S, double K, double r, double sigma, double T);

// gamma (same for call/put)
double bs_gamma(double S, double K, double r, double sigma, double T);

// vega (same for call/put), per 1.0 vol (not per 1%)
double bs_vega(double S, double K, double r, double sigma, double T);

// theta (per YEAR, consistent with T in years)
double bs_theta_call(double S, double K, double r, double sigma, double T);
double bs_theta_put(double S, double K, double r, double sigma, double T);

// rho (per 1.0 rate)
double bs_rho_call(double S, double K, double r, double sigma, double T);
double bs_rho_put(double S, double K, double r, double sigma, double T);

// A convenience “one-shot” result
struct BsResult {
  double call = 0.0;
  double put = 0.0;

  // call greeks
  double delta_call = 0.0;
  double gamma = 0.0;
  double vega = 0.0;
  double theta_call = 0.0;
  double rho_call = 0.0;

  // put greeks
  double delta_put = 0.0;
  double theta_put = 0.0;
  double rho_put = 0.0;
};

// Compute prices + greeks in one call 
BsResult black_scholes_all(double S, double K, double r, double sigma, double T);

double implied_vol_call(double market_price, double S, double K, double r, double T,
                        double sigma_lo = 1e-6, double sigma_hi = 5.0);

double implied_vol_put(double market_price, double S, double K, double r, double T,
                       double sigma_lo = 1e-6, double sigma_hi = 5.0);

} 

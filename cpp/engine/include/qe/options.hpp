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

//greeks (no dividends)
double bs_delta_call(double S, double K, double r, double sigma, double T);
double bs_delta_put(double S, double K, double r, double sigma, double T);
double bs_vega(double S, double K, double r, double sigma, double T); // per 1.0 vol (not per 1%)

} 

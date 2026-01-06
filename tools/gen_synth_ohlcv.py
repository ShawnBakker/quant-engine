#!/usr/bin/env python3
"""
Synthetic OHLCV generator for qe_engine demos.

- Deterministic via seed (default: 42)
- Generates daily bars with a simple geometric random walk
- Writes CSV in the format expected by qe::read_ohlcv_csv

Columns:
  timestamp,open,high,low,close,volume
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import math
import random
from pathlib import Path


def clamp(x: float, lo: float, hi: float) -> float:
  return max(lo, min(hi, x))


def gen_ohlcv_rows(
  n: int,
  start_price: float,
  mu: float,
  sigma: float,
  start_date: dt.date,
  seed: int,
) -> list[dict[str, str]]:
  rng = random.Random(seed)

  # simulate close prices using a simple GBM-ish update:
  # close_t = close_{t-1} * exp((mu - 0.5*sigma^2) + sigma*z)

  close_prev = start_price

  rows: list[dict[str, str]] = []

  for i in range(n):
    z = rng.gauss(0.0, 1.0)
    ret = (mu - 0.5 * sigma * sigma) + sigma * z
    close = close_prev * math.exp(ret)

    # reasonable OHLC bar around close using a small intraday range
    # range scale is tied to sigma (vol) but bounded so it wont explode
    range_frac = clamp(abs(rng.gauss(0.0, 1.0)) * (sigma * 1.5), 0.001, 0.08)

    # open drifts a bit from prev close
    open_ = close_prev * (1.0 + clamp(rng.gauss(0.0, 1.0) * (sigma * 0.25), -0.03, 0.03))

    high = max(open_, close) * (1.0 + range_frac)
    low = min(open_, close) * (1.0 - range_frac)

    # vol: lognormal-ish
    vol = int(math.exp(rng.gauss(math.log(1_000_000), 0.35)))

    d = start_date + dt.timedelta(days=i)
    ts = d.isoformat()

    rows.append({
      "timestamp": ts,
      "open": f"{open_:.6f}",
      "high": f"{high:.6f}",
      "low": f"{low:.6f}",
      "close": f"{close:.6f}",
      "volume": str(vol),
    })

    close_prev = close

  return rows


def main() -> int:
  ap = argparse.ArgumentParser()
  ap.add_argument("--out", default="data/sample.csv", help="output CSV path")
  ap.add_argument("--rows", type=int, default=252, help="number of rows")
  ap.add_argument("--start-price", type=float, default=100.0, help="starting close price")
  ap.add_argument("--mu", type=float, default=0.0003, help="drift per bar (daily)")
  ap.add_argument("--sigma", type=float, default=0.01, help="vol per bar (daily)")
  ap.add_argument("--start-date", default="2024-01-01", help="YYYY-MM-DD")
  ap.add_argument("--seed", type=int, default=42, help="random seed")
  args = ap.parse_args()

  out_path = Path(args.out)
  out_path.parent.mkdir(parents=True, exist_ok=True)

  start_date = dt.date.fromisoformat(args.start_date)

  rows = gen_ohlcv_rows(
    n=args.rows,
    start_price=args.start_price,
    mu=args.mu,
    sigma=args.sigma,
    start_date=start_date,
    seed=args.seed,
  )

  with out_path.open("w", newline="", encoding="utf-8") as f:
    w = csv.DictWriter(f, fieldnames=["timestamp", "open", "high", "low", "close", "volume"])
    w.writeheader()
    w.writerows(rows)

  print(f"wrote {out_path} ({len(rows)} rows)")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())

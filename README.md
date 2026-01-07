# Quant Engine

**C++20 quantitative research and backtesting engine** focused on correctness,
performance, and clean system design.

Quant-Engine provides a reusable C++ core, a CLI for experimentation, and a
Docker-backed API for persisting runs and metrics.

The engine supports:
- CSV OHLCV ingestion
- Rolling indicators (SMA, volatility)
- Strategy backtesting with transaction costs
- Performance metrics (Sharpe, drawdown, win rate)
- Black–Scholes options pricing + greeks
- JSON & CSV reporting
- Postgres-backed run persistence
- Deterministic unit-tested core (Catch2)

This project is designed to have: target-based CMake, test coverage, reproducible builds, and incremental feature development.

---

**[`docs/quickstart.md`](docs/quickstart.md)** --- build, run, persist runs
**[`docs/architecture.md`](docs/architecture.md)** --- system design

---

## Repository Layout

```text
quant-engine/
├── cpp/engine/        # C++ engine + CLI
├── api/               # TypeScript API (Express)
├── db/                # SQL migrations
├── data/              # Sample datasets
├── docs/              # Architecture + Quickstart
├── out/               # Local outputs
├── docker-compose.yml
└── README.md
```

## Principles
-Explicit ownership boundaries 

-Deterministic, test-first C++ core

-Windows-first developer experience

-Infrastructure via Docker

-Incremental feature development via issues

## Status (What's next?)

Current capabilities:

    -Backtesting (SMA crossover)

    -Options pricing (Black–Scholes)

    -Run + metric persistence via API

    -Cursor-based querying

Next roadmap:

    -API authentication

    -Performance benchmarking extensions

    -Strategy library expansion
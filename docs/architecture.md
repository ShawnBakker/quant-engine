# Quant-Engine Architecture Overview

Quant-Engine is a modular quantitative research and backtesting system composed of:

- A deterministic C++ computation core
- A thin command-line orchestration layer
- A Docker-backed API 

---

## Repository Layout

```text
├── cpp/
│   └── engine/
│       ├── include/qe/        # Public C++ headers
│       ├── src/               # Engine + CLI implementation
│       ├── tests/             # Catch2 unit tests
│       └── CMakeLists.txt
├── api/                       # TypeScript API (Express)
├── db/
│   ├── migrations/            # SQL schema
│   └── schema.md
├── data/                      # Sample datasets
├── docs/                      # Architecture + Quickstart
├── out/                       # Local outputs
└── docker-compose.yml
```

## Core Components

### `qe_engine` (C++ library)
`qe_engine` is a reusable, deterministic computation library.
It performs no IO and has no external side effects.

Responsibilities include:

-CSV ingestion

-Rolling indicators (returns, SMA, volatility)

-Strategy backtesting (SMA crossover)

-Cost modeling (fees, slippage)

-Reporting (equity curves, summary metrics)

-Options pricing (Black–Scholes + greeks)

-Micro-benchmarks

This separation allows the engine to be reused by:

-CLI tools

-Batch experiments

-Future services

### `qe_cli` (CLI)
The CLI is a thin orchestration layer over `qe_engine`.

Responsibilities:

-Argument parsing

-JSON config loading (Boost.json)

-File IO coordination

-Human-readable output

-Optional recording of runs via the API

-The CLI is currently the only writer to the API.

### API + Persistence layer
A lightweight TypeScript API provides persistence and query access
for experiment runs.

### API Responsibilities
-Store run metadata (`runs` table)

-Store numeric backtest metrics (`run_metrics` table)

-Provide cursor-based querying for recent and individual runs

The API does not execute strategies or pricing logic.

### Storage Model
`runs` stores immutable run metadata:

-`id`

-`command` (e.g. `backtest`, `options`)

-`engine_version`

-JSON arguments

-JSON results (when applicable)

`run_metrics` stores numeric metrics for backtests:

-Sharpe ratio

-Total return

-Drawdown

-Win rate

-Trade count

-Costs

Metrics are normalized to support future analytics.

## Endpoints

```text
GET  /health
POST /runs
GET  /runs?limit=N
GET  /runs/:id
POST /runs/:id/metrics
```

## Infrastructure

-Postgres and API run via Docker Compose

-Database schema is applied automatically on startup

-The system is designed for local-first development on Windows

## Database

Postgres runs via Docker Compose and is initialized from SQL migrations in `db/migrations`.

- `runs`: immutable run metadata (command, engine_version, created_at) plus `args_json` (inputs and, for options runs, `args_json.result`)
- `run_metrics`: numeric backtest metrics keyed by `run_id` for query-friendly analysis

## Build & Testing (Windows)

See `docs/quickstart.md` for the full developer workflow.

C++ builds use CMake + Visual Studio 2022 (x64). Unit tests are written with Catch2 and run via CTest:
```powershell
ctest --test-dir build_x64 -C Release
```

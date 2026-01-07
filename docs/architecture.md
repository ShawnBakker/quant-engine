# Quant-Engine Architecture Overview

Quant-Engine is a C++-based quantitative research and trading engine with a command-line interface (CLI), designed for backtesting, experimentation, and performance analysis.
The repository is structured as a monorepo to support future services such as APIs, databases, and distributed run orchestration.

## Repository Layout

```yaml
├── cpp/
│ └── engine/
│ ├── include/qe/        # Public headers
│ ├── src/               # Engine + CLI implementation
│ ├── tests/             # Catch2 unit tests
│ └── CMakeLists.txt
├── api/                 # TypeScript API (Express + Postgres)
├── db/
│ ├── migrations/        # SQL migrations
│ └── schema.md
├── data/                # Sample datasets (CSV)
├── docs/                # Architecture and quickstart docs
├── tools/               # Helper scripts
├── out/                 # Generated outputs (local)
└── docker-compose.yml
```

## Core Components

### `qe_engine` (C++ library)
The engine is built as a reusable C++ library providing:

-CSV ingestion ('csv_reader')

-Indicators (returns, rolling mean, rolling std)

-Strategy backtesting (SMA crossover)

-Cost modeling (fees + slippage)

-Reporting (equity curves, summary metrics)

-Options pricing (Black–Scholes + greeks)

-Micro-benchmarks (compute vs IO)


This separation allows the engine to be reused by:

-CLI tools

-Automated batch runs

-Future services and APIs

### `qe_cli` (CLI)
The CLI is a thin orchestration layer over ('qe_engine'). It is responsible for:

-Argument parsing

-Config loading (JSON via Boost.JSON)

-File IO coordination

-Human-readable output

-Recording runs to the API (when enabled)

### API + Storage
A lightweight TypeScript API is implemented to persist (and later store) run metadata and metrics.

### API Responsibilities
-Store run metadata (runs table)

-Store backtest metrics (run_metrics table)

-Provide basic querying for recent and individual runs

## Key Endpoints

```text
GET  /health
POST /runs
GET  /runs?limit=N
GET  /runs/:id
POST /runs/:id/metrics
```

## Database

Postgres runs via Docker Compose and is initialized with SQL migrations.

ex:

- ('runs')
run metadata

command name ('backtest, options')

JSON arguments + results

- ('run_metrics')

numeric backtest metrics keyed by ('run_id')

## Build Instructions (Windows)

### Requirements
- Windows 10/11
- Visual Studio 2022 (MSVC)
- CMake ≥ 3.24
- Docker Desktop
- Node.js (for API)

### Build (x64, Release)

```powershell
cmake -S cpp/engine -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release
ctest --test-dir build_x64 -C Release
```

## Demo Commands

Version
```powershell
qe_cli --version
```

CSV Load Check
```powershell
qe_cli run --data data/sample.csv
```

Indicators
```powershell
qe_cli indicators --data data/sample.csv --window 5
```

Backtest
```powershell
qe_cli backtest --data data/sample.csv --out out
```

Backtest (Config-driven)
```powershell
qe_cli backtest --data data/sample.csv --config config.json --out out
```

Options pricing (Black-Scholes)
```powershell
qe_cli options --S 100 --K 110 --r 0.05 --sigma 0.2 --T 0.5
```

Backtest config format:
```json
{
  "strategy": "sma_crossover",
  "params": {
    "fast": 5,
    "slow": 20,
    "initial": 1.0,
    "costs": {
      "fee_bps": 0.0,
      "slippage_bps": 0.0
    }
  }
}
```

## Outputs

```yaml
equity.csv:
  Time-ordered equity curve of the strategy.

report.json:
  Summary statistics including:
    - total return
    - Sharpe ratio
    - max drawdown
    - win rate
    - trade count
    - total cost
```

## Testing

-Unit tests are written via Catch2.
-Tests are automatically utilized via CMake.

To run all tests:
```powershell
ctest --test-dir build_x64 -C Release
```

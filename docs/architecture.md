# Quant-Engine Architecture Overview

Quant-Engine is a C++-based quantitative trading engine with a command-line interface (CLI) designed for research, backtesting, and performance experimentation.
The repository is structured as a monorepo to support future addon services,
APIs, and databases.

## Repository Layout

```yaml
├── cpp/
│ └── engine/
│ ├── include/qe/ # Public headers
│ ├── src/ # Engine + CLI implementation
│ ├── tests/ # Catch2 unit tests
│ └── CMakeLists.txt
├── data/ # Sample datasets (CSV)
├── docs/ # Architecture and usage docs
├── out/ # Generated outputs (ignored by git)
└── README.md
```

## Core Components

### `qe_engine` (C++ library)
The engine is built as a reusable static/shared library that provides:

- CSV ingestion (`csv_reader`)
- Indicators (returns, rolling mean/std)
- Strategy backtesting (SMA crossover)
- Cost modeling (fees + slippage)
- Reporting (equity curves, metrics)

This separation allows the engine to be reused by:
- CLI tools
- Future services (API, batch runs)
- Performance benchmarks

### `qe_cli` (CLI)

The CLI is a thin wrapper over `qe_engine`. It handles:
- Argument parsing
- Config loading
- File IO orchestration
- Human-readable output

## Build Instructions (Windows)

### Requirements
- Windows 10/11
- Visual Studio 2022 (MSVC)
- CMake ≥ 3.24

### Build (x64, Release)

```powershell
cmake -S cpp/engine -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release
ctest --test-dir build_x64 -C Release
```

## Demo Commands

```powershell
qe_cli --version
```

CSV Load Check
```powershell
qe_cli run --data data/sample.csv
```

Indicators
```powershell
qe_cli backtest --data data/sample.csv --out out
```

Backtest (Config-driven)
```powershell
qe_cli backtest --data data/sample.csv --config config.json --out out
```

Example config.json:
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
A time-ordered equity curve of the strategy.

report.json
Contains summary statistics:
-total return
-Sharpe ratio
-max drawdown
-trade count
-cost impact
```

## Testing

```yaml
-Unit tests are written via Catch2.
-Tests are automatically discovered via CMake.
```

To run all tests:
```powershell
ctest --test-dir build_x64 -C Release
```
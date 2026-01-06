## Quickstart for Quant Engine

--a clean checkout to a full backtest run with
synthetic OHLCV data, metrics, and JSON output.

Platform assumptions:
- Windows 10/11
- MSVC Build Tools 2022
- CMake ≥ 3.24
- Python ≥ 3.10

---

## 1. Clone the repository

```powershell
git clone https://github.com/ShawnBakker/quant-engine.git
cd quant-engine
```

## 2. Configure and build the cpp engine
This engine utilizes an out-of-source CMake build targeting MSVC x64.


```powershell
cmake -S cpp/engine -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release
```

# Run unit tests if you'd like:

```
ctest --test-dir build_x64 -C Release --output-on-failure
```

## 3. Generate the synthetic OHLCV data
# The repo includes a Python utility to generate realistic OHLCV time series (using a geometric Brownian motion model)

```powershell
python tools/gen_synth_ohlcv.py `
  --out data/sample.csv `
  --rows 252 `
  --start-price 100 `
  --mu 0.0003 `
  --sigma 0.01 `
  --seed 42
```

# Verify if you'd like:

```powershell
(Import-Csv data/sample.csv).Count
```

## 4. Create a backtest config
# This needs to be done (config.json) at the repo root:

```json
{
  "strategy": "sma_crossover",
  "fast_window": 10,
  "slow_window": 50,
  "initial_equity": 10000,
  "costs": {
    "fee_bps": 0,
    "slippage_bps": 0
  }
}
```

## Run a backtest
# CLI backtest command can be executed via:

```powershell
.\build_x64\Release\qe_cli.exe backtest `
  --data data\sample.csv `
  --config config.json `
  --out out
```

## 6. Inspect the outputs / results
# CSV and JSON artifcats will be present:

``` powershell
type out\equity.csv # Equity curve (CSV)
type out\report.json # Full report (JSON)
```

# Example report structure:

```json
{
  "strategy": "sma_crossover",
  "params": {
    "fast": 10,
    "slow": 50,
    "initial": 10000
  },
  "stats": {
    "total_return": 1.53,
    "sharpe": 0.55,
    "max_drawdown": 0.024,
    "trades": 124,
    "total_cost": 0.0
  },
  "series": {
    "equity": [ ... ],
    "final_equity": 25390.25,
    "strategy_returns": [ ... ]
  }
}
```

## 7. Common errors if you choose to integrate your own dataset or config issues persist:

```powershell
“not enough data for slow_window” : 

Ensure rows > slow_window


"Config not found" : 

Run the CLI from the repo root

Use relative paths as shown above


"Build directory stuck / locked" : 

Close Visual Studio

Rename then delete the build folder if needed
```

## 8. How can this be customized or edited?

# You can : 

```txt
Modify strategy parameters in config.json
Add transaction costs and slippage
Extend reporting metrics
Add parallel indicators with std::execution
Integrate config loading into CI
```
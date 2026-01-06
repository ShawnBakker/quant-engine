# Quant Engine

**C++20 quantitative backtesting engine** -- aims for correctness, performance, and clean system design.

The engine supports:
- CSV OHLCV ingestion
- Rolling indicators (SMA)
- Strategy backtesting with transaction costs
- Performance metrics (Sharpe, drawdown, win rate)
- JSON & CSV reporting
- Deterministic synthetic data generation
- CI-backed unit testing (Catch2)

This project is designed to have: target-based CMake, test coverage, reproducible builds, and incremental feature development.

---

**[`docs/quickstart.md`](docs/quickstart.md)**

---

## Repository Structure

```text
quant-engine/
├── cpp/
│   └── engine/
│       ├── include/qe/        # Public headers
│       ├── src/               # Engine implementation
│       ├── tests/             # Catch2 tests
│       └── CMakeLists.txt
├── data/
│   └── sample.csv             # Example OHLCV dataset
├── tools/
│   └── gen_synth_ohlcv.py     # Synthetic OHLCV generator
├── out/                       # Backtest outputs
├── docs/
│   └── quickstart.md
├── CMakeLists.txt
└── README.md


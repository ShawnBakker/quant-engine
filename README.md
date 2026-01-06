# from repo root
cmake -S . -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release

# generate 252-row synthetic dataset (optional)
python .\tools\gen_synth_ohlcv.py --out .\data\sample.csv --rows 252 --start-price 100 --mu 0.0003 --sigma 0.01 --seed 42

# run backtest
.\build_x64\Release\qe_cli.exe backtest --data .\data\sample.csv --config .\config.json --out .\out

# view outputs
type .\out\report.json
type .\out\equity.csv


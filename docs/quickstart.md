## Quant-Engine Quickstart (Windows)

This guide walks through:

-Building the C++ engine and CLI
-Starting Postgres + API via Docker Compose
-Running CLI commands that persist runs and metrics

All instructions are written for **Windows PowerShell**.

---

## Requirements

- Windows 10 / 11
- Visual Studio 2022 (MSVC toolchain)
- CMake ≥ 3.24
- Docker Desktop
- Node.js (for API development)
- Git

---

## Clone the repo

```powershell
git clone https://github.com/ShawnBakker/quant-engine.git
cd quant-engine
```

## Build the C++ engine and CLI

From the repo root:

```powershell
cmake -S cpp/engine -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release
ctest --test-dir build_x64 -C Release
```

CLI binary can be found at:

```text
build_x64/Release/qe_cli.exe
```

## Start Postgres + API (Docker Compose)

NOTE (POWERSHELL):
`curl` is often an alias for `Invoke-WebRequest`.
Althought it is preference, use `curl.exe` or `irm` (`Invoke-RestMethod`) for JSON APIs.

From the repo root:

```powershell
docker compose up -d --build
docker compose ps
```

Wait until Postgres reports as healthy and the API is running.

```powershell
curl.exe http://127.0.0.1:8787/health
```

## Run the CLI w/ API Recording enabled

The CLI automatically records runs when the API is reachable.

Backtest example:

```powershell
.\build_x64\Release\qe_cli.exe backtest `
  --data .\data\sample.csv `
  --out .\out
```
This will:

-Run the SMA crossover backtest

-Write:

    `-out/equity.csv`

    `-out/report.json`

-Record the run and metrics in Postgres via the API

## Options Pricing Example

```powershell
.\build_x64\Release\qe_cli.exe options `
  --S 100 --K 110 --r 0.05 --sigma 0.2 --T 0.5
```

This will:

-Compute Black–Scholes prices and greeks

-Record the run in Postgres

-Store results in `runs.args_json.result`

## Verify Stored Runs

**VIA API**

```powershell
curl.exe "http://127.0.0.1:8787/runs?limit=5"
```

Fetch a single run:

```powershell
$id = "<run-id>"
curl.exe "http://127.0.0.1:8787/runs/$id"
```

Cursor pagination:

```powershell
$r = curl.exe "http://127.0.0.1:8787/runs?limit=2" | ConvertFrom-Json
$r.next_cursor
curl.exe "http://127.0.0.1:8787/runs?limit=2&cursor=$($r.next_cursor)"
```

**VIA POSTGRES**

```powershell
docker exec -it qe_postgres psql -U qe_user -d qe -c `
"SELECT id, command, status, created_at
 FROM runs
 ORDER BY created_at DESC
 LIMIT 5;"
```

Backtest metrics:

```powershell
docker exec -it qe_postgres psql -U qe_user -d qe -c `
"SELECT *
 FROM run_metrics
 ORDER BY created_at DESC
 LIMIT 5;"
```

## Common Issues

**"API not reachable"**

-Ensure Docker containers are running:

```powershell
docker compose ps
```

-Check API health:

```powershell
curl.exe http://127.0.0.1:8787/health
```

**"Postgres unhealthy"**

```powershell
docker logs qe_postgres --tail 200
```

**JSON config parse errors**

-Ensure config files are UTF-8 without BOM

-PowerShell `Set-Content` may insert BOM by default
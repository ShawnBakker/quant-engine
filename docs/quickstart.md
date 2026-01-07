## Quickstart for Quant Engine

This guide walks through building the C++ engine, starting Postgres via Docker,
running the API, and executing CLI commands that persist runs and metrics.

The instructions are written for Windows (PowerShell).

## Requirements

-Windows 10/11
-Visual Studio 2022 (MSVC toolchain)
-CMake ≥ 3.24
-Docker Desktop
-Node.js (for API)
-Git

```powershell
git clone https://github.com/ShawnBakker/quant-engine.git
cd quant-engine
```

## Build the C++ Engine and CLI

From the repository root:

```powershell
cmake -S cpp/engine -B build_x64 -G "Visual Studio 17 2022" -A x64
cmake --build build_x64 --config Release
ctest --test-dir build_x64 -C Release
```

The CLI binary will be located at:
```text
build_x64/Release/qe_cli.exe
```

## Start Postgres (Docker)

Copy the environment template and start the container:

```powershell
Copy-Item .env.ex .env
docker compose up -d
docker compose ps
```

Wait until the container reports healthy:

```powershell
docker inspect --format='{{json .State.Health.Status}}' qe_postgres
```

## Apply Database Migaration

PowerShell does not support ('< file.sql') redirection like bash.
Use a pipeline instead:

```powershell
Get-Content -Raw .\db\migrations\init.sql | docker exec -i qe_postgres psql -U qe_user -d qe
```

Verify tables exist:

```powershell
docker exec -it qe_postgres psql -U qe_user -d qe -c "\dt"
```

## Run the API Server

From the repo root:

```powershell
cd api
npm install
npm run dev
```

Health check:

```powershell
irm http://localhost:8787/health
```

Expected output:

```json
{ "ok": true }
```

## Run the CLI with API Recording Enabled

Set the API URL:

```powershell
$env:QE_API_URL="http://localhost:8787"
```

## Backtest Example

```powershell
.\build_x64\Release\qe_cli.exe backtest --data .\data\sample.csv --out .\out
```

So:
-Run the backtest
-Write ('out/equity.csv') and ('out/report.json')
-Record the run and metrics in Postgres via the API

## Options Pricing Example

```powershell
.\build_x64\Release\qe_cli.exe options --S 100 --K 110 --r 0.05 --sigma 0.2 --T 0.5
```

This does:
-Compute Black–Scholes prices and greeks
-Record the run in Postgres
-Store results in ('runs.args_json.result')

## Verify Stored Runs

# API:
```powershell
irm "http://localhost:8787/runs?limit=5"
```

Fetch a single run:
```powershell
$id = "<run-id>"
irm "http://localhost:8787/runs/$id" | ConvertTo-Json -Depth 10
```

# Via Postgres
```powershell
docker exec -it qe_postgres psql -U qe_user -d qe -c `
"SELECT id, command, status, created_at FROM runs ORDER BY created_at DESC LIMIT 5;"
```

Backtest metrics:
```powershell
docker exec -it qe_postgres psql -U qe_user -d qe -c `
"SELECT * FROM run_metrics ORDER BY run_id DESC LIMIT 5;"
```

## Common Issues

# Postgres container unhealthy

-Ensure ('.env') exists and contains required variables

-Restart with ('docker compose up -d')

-Inspect logs:

```powershell
docker logs qe_postgres --tail 200
```

# API not reachable

-Confirm the API is running on port ('8787')

-Check:

```powershell
irm http://localhost:8787/health
```

# Config JSON parse errors
-Ensure config files are UTF-8 without BOM

-PowerShell may insert BOM when using ('Set-Content')
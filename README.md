# quant-engine (C++20 - typescript - postgres)
a quantitative research and backtesting platform.
    -C++20 powering high-performance analytics, backtesting, and addon modules (option pricing).
    -TypeScript (node.js) begins runs and exposes an API.
    -Postgres stores runs, metrics, and artifacts.

## repo layout
- `cpp/engine` is a cpp core library and CLI.
- 'apps/api' is a typescript API service.
- 'db' is for the schema and migrations.
- 'docs' holds the architecture and performance notes.

## quickstart (in progress)
```bash
# start database
-> docker compose up -d

# build cpp engine (needs implementation)
-> cmake -S cpp/engine -B build && cmake --build build

# run API (needs implementation)
-> cd apps/api && npm install && npm run dev

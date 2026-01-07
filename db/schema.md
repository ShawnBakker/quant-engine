# Quant-Engine Database Schema (v1)

Database name: `qe`

This schema supports persistent storage of engine runs (Issue #14) and enables a
TypeScript API for runs and storage (Issue #15).

---

## Tables

### `runs`
Tracks a single engine invocation (e.g., `backtest`, `options`).

**Columns**
- `id` (uuid, pk): unique run id
- `created_at` (timestamptz): time run record was created
- `status` (text): `queued | running | success | failed`
- `engine_version` (text): engine version string (e.g., `qe_cli --version`)
- `command` (text): logical command name (e.g., `backtest`, `options`)
- `args_json` (jsonb): normalized args/config used for the run
- `data_ref` (text): dataset reference (path for now)
- `out_dir` (text): output directory (path for now)
- `error` (text, nullable): failure details

**Notes**
- `args_json` is indexed with a GIN index for later querying.
- For v1, `data_ref` and `out_dir` are strings. Later they can become URIs or ids.

---

### `run_metrics`
Stores backtest metrics. One row per run (1:1 with `runs`).

**Columns**
- `run_id` (uuid, pk, fk -> runs.id)
- `total_return`, `sharpe`, `max_drawdown`, `win_rate`
- `n_trades`, `total_cost`, `final_equity`

**Notes**
- Nullable columns allow `options` runs (or other commands) to skip metrics.

---

## Migration

- `db/migrations/001_init.sql` initializes the schema and indexes.

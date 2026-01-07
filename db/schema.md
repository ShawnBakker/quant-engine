# Quant-Engine Database Schema (v1)

Database name: `qe`

This schema supports persistent storage of engine runs and enables a
TypeScript API for runs and storage.

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
- `data_ref` (text): dataset reference (path for now; later can be URI or id)
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
- `total_return` (double precision, nullable)
- `sharpe` (double precision, nullable)
- `max_drawdown` (double precision, nullable)
- `win_rate` (double precision, nullable)
- `n_trades` (integer, nullable)
- `total_cost` (double precision, nullable)
- `final_equity` (double precision, nullable)

**Notes**
- Nullable columns allow non-backtest commands (like `options`) to skip metrics.
- `run_metrics.run_id` cascades on delete from `runs`.

---

## Migration

- `db/migrations/init.sql` initializes the schema and indexes.

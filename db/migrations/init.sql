-- Quant-Engine: initial schema (v1)
-- This schema is intentionally minimal

BEGIN;

-- UUID generation (gen_random_uuid)
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Runs: tracks any engine invocation 
CREATE TABLE IF NOT EXISTS runs (
  id            uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  created_at    timestamptz NOT NULL DEFAULT now(),
  status        text NOT NULL DEFAULT 'success' CHECK (status IN ('queued','running','success','failed')),

  engine_version text NOT NULL DEFAULT '',

  -- Logical command 
  command       text NOT NULL,

  -- Normalized args/config blob used for the run (CLI / config.json)
  args_json     jsonb NOT NULL DEFAULT '{}'::jsonb,

  -- What data was used (path for now; later can become dataset_id or storage URI)
  data_ref      text NOT NULL DEFAULT '',

  -- Where outputs were written (path for now)
  out_dir       text NOT NULL DEFAULT '',

  -- Error details on failure (if any)
  error         text
);

-- Basic metrics for backtests (nullable for non-backtest commands like "options")
CREATE TABLE IF NOT EXISTS run_metrics (
  run_id        uuid PRIMARY KEY REFERENCES runs(id) ON DELETE CASCADE,

  total_return  double precision,
  sharpe        double precision,
  max_drawdown  double precision,
  win_rate      double precision,
  n_trades      integer,
  total_cost    double precision,
  final_equity  double precision
);

-- Helpful indexes
CREATE INDEX IF NOT EXISTS idx_runs_created_at ON runs(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_runs_command ON runs(command);
CREATE INDEX IF NOT EXISTS idx_runs_status ON runs(status);
CREATE INDEX IF NOT EXISTS idx_runs_args_json_gin ON runs USING GIN (args_json);

COMMIT;

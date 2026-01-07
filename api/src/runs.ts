import { z } from "zod";
import { pool } from "./db.js";

export const RunStatus = z.enum(["queued", "running", "success", "failed"]);

export const CreateRunBody = z.object({
  engine_version: z.string().default(""),
  command: z.string().min(1),
  status: RunStatus.default("success"),
  args_json: z.record(z.any()).default({}),
  data_ref: z.string().default(""),
  out_dir: z.string().default(""),
  error: z.string().optional().nullable()
});

export type CreateRunBody = z.infer<typeof CreateRunBody>;

export const UpsertMetricsBody = z.object({
  total_return: z.number().optional().nullable(),
  sharpe: z.number().optional().nullable(),
  max_drawdown: z.number().optional().nullable(),
  win_rate: z.number().optional().nullable(),
  n_trades: z.number().int().optional().nullable(),
  total_cost: z.number().optional().nullable(),
  final_equity: z.number().optional().nullable()
});

export type UpsertMetricsBody = z.infer<typeof UpsertMetricsBody>;

export async function createRun(input: CreateRunBody) {
  const q = `
    INSERT INTO runs (engine_version, command, status, args_json, data_ref, out_dir, error)
    VALUES ($1, $2, $3, $4::jsonb, $5, $6, $7)
    RETURNING id, created_at, engine_version, command, status, args_json, data_ref, out_dir, error
  `;
  const values = [
    input.engine_version,
    input.command,
    input.status,
    JSON.stringify(input.args_json ?? {}),
    input.data_ref,
    input.out_dir,
    input.error ?? null,
  ];
  const res = await pool.query(q, values);
  return res.rows[0];
}

export async function listRuns(limit: number) {
  const q = `
    SELECT id, created_at, engine_version, command, status, args_json, data_ref, out_dir, error
    FROM runs
    ORDER BY created_at DESC
    LIMIT $1
  `;
  const res = await pool.query(q, [limit]);
  return res.rows;
}

export async function getRunById(id: string) {
  const q = `
    SELECT
      r.id, r.created_at, r.engine_version, r.command, r.status, r.args_json, r.data_ref, r.out_dir, r.error,
      m.total_return, m.sharpe, m.max_drawdown, m.win_rate, m.n_trades, m.total_cost, m.final_equity
    FROM runs r
    LEFT JOIN run_metrics m ON m.run_id = r.id
    WHERE r.id = $1
  `;
  const res = await pool.query(q, [id]);
  return res.rows[0] ?? null;
}

export async function upsertMetrics(runId: string, m: UpsertMetricsBody) {
  const q = `
    INSERT INTO run_metrics (
      run_id, total_return, sharpe, max_drawdown, win_rate, n_trades, total_cost, final_equity
    ) VALUES (
      $1, $2, $3, $4, $5, $6, $7, $8
    )
    ON CONFLICT (run_id) DO UPDATE SET
      total_return = EXCLUDED.total_return,
      sharpe = EXCLUDED.sharpe,
      max_drawdown = EXCLUDED.max_drawdown,
      win_rate = EXCLUDED.win_rate,
      n_trades = EXCLUDED.n_trades,
      total_cost = EXCLUDED.total_cost,
      final_equity = EXCLUDED.final_equity
    RETURNING run_id, total_return, sharpe, max_drawdown, win_rate, n_trades, total_cost, final_equity
  `;
  const values = [
    runId,
    m.total_return ?? null,
    m.sharpe ?? null,
    m.max_drawdown ?? null,
    m.win_rate ?? null,
    m.n_trades ?? null,
    m.total_cost ?? null,
    m.final_equity ?? null,
  ];
  const res = await pool.query(q, values);
  return res.rows[0];
}

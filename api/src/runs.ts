import { pool } from "./db.js";
import { CreateRunBody, UpsertMetricsBody } from "./schema.js";

type RunRow = {
  id: string;
  created_at: string;
  engine_version: string;
  command: string;
  status: string;
  args_json: any;
  data_ref: string;
  out_dir: string;
  error: string | null;
};

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
  return res.rows[0] as RunRow;
}

/**
 * Cursor pagination:
 * - Order: created_at DESC, id DESC
 * - cursor is a run id
 * - We page "older than cursor" using (created_at, id) < (cursor_created_at, cursor_id)
 */
export async function listRuns(limit: number, cursor?: string) {
  if (!cursor) {
    const q = `
      SELECT id, created_at, engine_version, command, status, args_json, data_ref, out_dir, error
      FROM runs
      ORDER BY created_at DESC, id DESC
      LIMIT $1
    `;
    const res = await pool.query(q, [limit]);
    const items = res.rows as RunRow[];
    return {
      items,
      next_cursor: items.length === limit ? items[items.length - 1].id : null,
    };
  }

  // Look up cursor anchor (created_at + id)
  const anchorQ = `SELECT id, created_at FROM runs WHERE id = $1`;
  const anchorRes = await pool.query(anchorQ, [cursor]);
  const anchor = anchorRes.rows[0] as { id: string; created_at: string } | undefined;
  if (!anchor) {
    const err: any = new Error("cursor not found");
    err.status = 400;
    throw err;
  }

  const q = `
    SELECT id, created_at, engine_version, command, status, args_json, data_ref, out_dir, error
    FROM runs
    WHERE (created_at, id) < ($2::timestamptz, $3::uuid)
    ORDER BY created_at DESC, id DESC
    LIMIT $1
  `;
  const res = await pool.query(q, [limit, anchor.created_at, anchor.id]);
  const items = res.rows as RunRow[];
  return {
    items,
    next_cursor: items.length === limit ? items[items.length - 1].id : null,
  };
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

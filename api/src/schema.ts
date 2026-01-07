import { z } from "zod";

export const RunStatus = z.enum(["queued", "running", "success", "failed"]);

export const CreateRunBody = z.object({
  engine_version: z.string().default(""),
  command: z.string().min(1),
  status: RunStatus.default("success"),
  args_json: z.record(z.unknown()).default({}),
  data_ref: z.string().default(""),
  out_dir: z.string().default(""),
  error: z.string().optional().nullable(),
});

export type CreateRunBody = z.infer<typeof CreateRunBody>;

export const UpsertMetricsBody = z.object({
  total_return: z.number().optional().nullable(),
  sharpe: z.number().optional().nullable(),
  max_drawdown: z.number().optional().nullable(),
  win_rate: z.number().optional().nullable(),
  n_trades: z.number().int().optional().nullable(),
  total_cost: z.number().optional().nullable(),
  final_equity: z.number().optional().nullable(),
});

export type UpsertMetricsBody = z.infer<typeof UpsertMetricsBody>;

export const ListRunsQuery = z.object({
  limit: z.coerce.number().int().min(1).max(200).default(50),
  cursor: z.string().uuid().optional(),
});

export type ListRunsQuery = z.infer<typeof ListRunsQuery>;

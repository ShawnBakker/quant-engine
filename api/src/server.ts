import "dotenv/config";
import express from "express";
import { z } from "zod";
import { dbHealthcheck } from "./db.js";
import { CreateRunBody, ListRunsQuery, UpsertMetricsBody } from "./schema.js";
import { createRun, getRunById, listRuns, upsertMetrics } from "./runs.js";

const app = express();
app.use(express.json({ limit: "1mb" }));

function errMsg(err: unknown): string {
  if (err instanceof z.ZodError) {
    return err.issues.map(i => `${i.path.join(".") || "(root)"}: ${i.message}`).join("; ");
  }
  if (err && typeof err === "object" && "message" in err) {
    return String((err as any).message);
  }
  return String(err);
}

function sendError(res: any, status: number, err: unknown) {
  res.status(status).json({ error: errMsg(err) });
}

app.get("/health", async (_req, res) => {
  try {
    await dbHealthcheck();
    res.json({ ok: true });
  } catch (err) {
    res.status(500).json({ ok: false, error: errMsg(err) });
  }
});

app.post("/runs", async (req, res) => {
  try {
    const body = CreateRunBody.parse(req.body);
    const row = await createRun(body);
    res.status(201).json(row);
  } catch (err) {
    // Zod or bad client input
    sendError(res, 400, err);
  }
});

app.get("/runs", async (req, res) => {
  try {
    const { limit, cursor } = ListRunsQuery.parse(req.query);
    const page = await listRuns(limit, cursor);
    res.json(page); // { items: [...], next_cursor: string|null }
  } catch (err: any) {
    const status = typeof err?.status === "number" ? err.status : 400;
    sendError(res, status, err);
  }
});

app.get("/runs/:id", async (req, res) => {
  try {
    const id = z.string().uuid().parse(req.params.id);
    const row = await getRunById(id);
    if (!row) return res.status(404).json({ error: "run not found" });
    res.json(row);
  } catch (err) {
    sendError(res, 400, err);
  }
});

app.post("/runs/:id/metrics", async (req, res) => {
  try {
    const id = z.string().uuid().parse(req.params.id);
    const body = UpsertMetricsBody.parse(req.body);
    const row = await upsertMetrics(id, body);
    res.status(201).json(row);
  } catch (err) {
    sendError(res, 400, err);
  }
});

const port = Number(process.env.PORT ?? "8787");
app.listen(port, "0.0.0.0", () => {
  console.log(`qe-api listening on http://0.0.0.0:${port}`);
});


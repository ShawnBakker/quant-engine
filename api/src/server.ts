import "dotenv/config";
import express from "express";
import { z } from "zod";
import { dbHealthcheck } from "./db.js";
import { CreateRunBody, UpsertMetricsBody, createRun, getRunById, listRuns, upsertMetrics } from "./runs.js";

const app = express();
app.use(express.json({ limit: "1mb" }));

app.get("/health", async (_req, res) => {
  try {
    await dbHealthcheck();
    res.json({ ok: true });
  } catch (err: any) {
    res.status(500).json({ ok: false, error: err?.message ?? String(err) });
  }
});

app.post("/runs", async (req, res) => {
  try {
    const body = CreateRunBody.parse(req.body);
    const row = await createRun(body);
    res.status(201).json(row);
  } catch (err: any) {
    res.status(400).json({ error: err?.message ?? String(err) });
  }
});

app.get("/runs", async (req, res) => {
  try {
    const limit = z.coerce.number().int().min(1).max(200).default(50).parse(req.query.limit);
    const rows = await listRuns(limit);
    res.json(rows);
  } catch (err: any) {
    res.status(400).json({ error: err?.message ?? String(err) });
  }
});

app.get("/runs/:id", async (req, res) => {
  try {
    const id = z.string().uuid().parse(req.params.id);
    const row = await getRunById(id);
    if (!row) return res.status(404).json({ error: "run not found" });
    res.json(row);
  } catch (err: any) {
    res.status(400).json({ error: err?.message ?? String(err) });
  }
});

app.post("/runs/:id/metrics", async (req, res) => {
  try {
    const id = z.string().uuid().parse(req.params.id);
    const body = UpsertMetricsBody.parse(req.body);
    const row = await upsertMetrics(id, body);
    res.status(201).json(row);
  } catch (err: any) {
    res.status(400).json({ error: err?.message ?? String(err) });
  }
});

const port = Number(process.env.PORT ?? "8787");
app.listen(port, () => {
  console.log(`qe-api listening on http://localhost:${port}`);
});

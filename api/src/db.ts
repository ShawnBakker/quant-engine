import pg from "pg";

const { Pool } = pg;

function requireEnv(name: string, fallback?: string): string {
  const v = process.env[name] ?? fallback;
  if (!v) throw new Error(`Missing required env var: ${name}`);
  return v;
}

export const pool = new Pool({
  host: requireEnv("PGHOST", "127.0.0.1"),
  port: Number(requireEnv("PGPORT", "5432")),
  database: requireEnv("PGDATABASE", "qe"),
  user: requireEnv("PGUSER", "qe_user"),
  password: requireEnv("PGPASSWORD", "qe_password"),
});

export async function dbHealthcheck(): Promise<void> {
  const res = await pool.query("SELECT 1 as ok");
  if (res.rows?.[0]?.ok !== 1) {
    throw new Error("DB healthcheck failed");
  }
}

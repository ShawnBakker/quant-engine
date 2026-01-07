param(
  [string]$Container = "qe_postgres",
  [string]$User = "qe_user",
  [string]$Db = "qe",
  [string]$MigrationPath = ".\db\migrations\init.sql"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $MigrationPath)) {
  throw "Migration file not found: $MigrationPath"
}

Get-Content -Raw $MigrationPath | docker exec -i $Container psql -U $User -d $Db

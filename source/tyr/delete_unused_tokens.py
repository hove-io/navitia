#!/usr/bin/env python3
"""
Script to delete unused tokens from the TYR database.

Reads a CSV file (semicolon-separated) containing token information and generates
SQL DELETE statements for the 'key' table based on the token prefix column.

For entries where the token prefix was corrupted by Excel (scientific notation or
truncated), the deletion falls back to using the key ID (tyr_id).

Usage:
    # Generate SQL file (dry-run, review before executing):
    python delete_unused_tokens.py --csv tokens.csv --output delete_tokens.sql

    # Execute directly against the database (requires SQLALCHEMY_DATABASE_URI or --db-uri):
    python delete_unused_tokens.py --csv tokens.csv --execute --db-uri "postgresql://user:pass@host/db"
"""

import argparse
import csv
import re
import sys


def parse_csv(csv_path):
    """
    Parse the CSV file and return two lists:
    - valid_entries: list of (tyr_id, token_prefix) with valid hex prefixes
    - corrupted_entries: list of (tyr_id, login, raw_prefix) for corrupted prefixes
    """
    valid_entries = []
    corrupted_entries = []

    with open(csv_path, "r", encoding="latin-1") as f:
        reader = csv.reader(f, delimiter=";")
        header = next(reader)
        print(f"CSV columns: {header}")

        for i, row in enumerate(reader, start=2):
            if len(row) < 3 or not row[0].strip():
                continue

            tyr_id = row[0].strip()
            login = row[1].strip()
            token_prefix = row[2].strip()

            if not token_prefix:
                continue

            # Valid token prefixes are 8-character hex strings
            if re.match(r"^[0-9a-fA-F]{8}$", token_prefix):
                valid_entries.append((tyr_id, token_prefix))
            else:
                corrupted_entries.append((tyr_id, login, token_prefix))

    return valid_entries, corrupted_entries


def generate_sql(valid_entries, corrupted_entries):
    """Generate SQL DELETE statements."""
    lines = []
    lines.append("-- =============================================================")
    lines.append("-- Script de suppression des tokens inutilisés depuis 365 jours")
    lines.append("-- =============================================================")
    lines.append("-- ATTENTION: Exécuter dans une transaction pour pouvoir annuler")
    lines.append("-- en cas de problème.")
    lines.append("-- =============================================================")
    lines.append("")
    lines.append("BEGIN;")
    lines.append("")

    # --- Part 1: Delete by token prefix (valid hex entries) ---
    lines.append(f"-- Partie 1: Suppression par préfixe de token ({len(valid_entries)} entrées)")
    lines.append("-- Utilisation d'une table temporaire pour les préfixes")
    lines.append("")
    lines.append("CREATE TEMPORARY TABLE _token_prefixes_to_delete (prefix TEXT NOT NULL);")
    lines.append("")

    # Batch INSERT for efficiency
    batch_size = 100
    for i in range(0, len(valid_entries), batch_size):
        batch = valid_entries[i : i + batch_size]
        values = ", ".join(f"('{entry[1]}')" for entry in batch)
        lines.append(f"INSERT INTO _token_prefixes_to_delete (prefix) VALUES {values};")

    lines.append("")
    lines.append("-- Vérification du nombre de tokens qui seront supprimés (préfixes)")
    lines.append(
        "SELECT COUNT(*) AS tokens_to_delete_by_prefix FROM key k "
        "WHERE EXISTS (SELECT 1 FROM _token_prefixes_to_delete t WHERE k.token LIKE t.prefix || '%');"
    )
    lines.append("")
    lines.append("-- Suppression des tokens correspondant aux préfixes")
    lines.append(
        "DELETE FROM key k "
        "WHERE EXISTS (SELECT 1 FROM _token_prefixes_to_delete t WHERE k.token LIKE t.prefix || '%');"
    )
    lines.append("")
    lines.append("DROP TABLE _token_prefixes_to_delete;")
    lines.append("")

    # --- Part 2: Delete by key ID (corrupted entries) ---
    if corrupted_entries:
        lines.append(
            f"-- Partie 2: Suppression par ID de clé ({len(corrupted_entries)} entrées)"
        )
        lines.append(
            "-- Ces entrées avaient un préfixe de token corrompu par Excel (notation scientifique)"
        )
        lines.append("-- On utilise donc le tyr_id (= key.id) pour les supprimer directement.")
        lines.append("")

        for tyr_id, login, raw_prefix in corrupted_entries:
            lines.append(
                f"-- login={login}, préfixe corrompu: {raw_prefix}"
            )

        lines.append("")
        key_ids = ", ".join(entry[0] for entry in corrupted_entries)
        lines.append(f"DELETE FROM key WHERE id IN ({key_ids});")
        lines.append("")

    # --- Summary ---
    total = len(valid_entries) + len(corrupted_entries)
    lines.append(f"-- Total attendu: ~{total} tokens supprimés")
    lines.append("")
    lines.append("-- Vérifier le résultat avant de valider:")
    lines.append("-- Si tout est correct, exécuter: COMMIT;")
    lines.append("-- Sinon, exécuter: ROLLBACK;")
    lines.append("")
    lines.append("-- Décommenter la ligne suivante pour valider:")
    lines.append("-- COMMIT;")
    lines.append("")
    lines.append("-- Ou annuler avec:")
    lines.append("-- ROLLBACK;")

    return "\n".join(lines)


def execute_sql(sql, db_uri):
    """Execute SQL directly against the database."""
    try:
        import sqlalchemy
    except ImportError:
        print("ERROR: sqlalchemy is required for --execute mode. Install with: pip install sqlalchemy")
        sys.exit(1)

    engine = sqlalchemy.create_engine(db_uri)
    with engine.connect() as conn:
        # Split and execute statements
        for statement in sql.split(";"):
            statement = statement.strip()
            if statement and not statement.startswith("--"):
                print(f"Executing: {statement[:80]}...")
                result = conn.execute(sqlalchemy.text(statement))
                if result.returns_rows:
                    for row in result:
                        print(f"  Result: {row}")
        conn.commit()
    print("Done.")


def main():
    parser = argparse.ArgumentParser(
        description="Delete unused tokens from the TYR database"
    )
    parser.add_argument(
        "--csv",
        required=True,
        help="Path to the CSV file with token prefixes",
    )
    parser.add_argument(
        "--output",
        default="delete_tokens.sql",
        help="Output SQL file path (default: delete_tokens.sql)",
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Execute SQL directly against the database instead of writing to file",
    )
    parser.add_argument(
        "--db-uri",
        help="Database URI (e.g., postgresql://user:pass@host/db). "
        "Can also be set via SQLALCHEMY_DATABASE_URI env var.",
    )

    args = parser.parse_args()

    # Parse CSV
    print(f"Reading CSV: {args.csv}")
    valid_entries, corrupted_entries = parse_csv(args.csv)
    print(f"Valid token prefixes: {len(valid_entries)}")
    print(f"Corrupted entries (will use key ID): {len(corrupted_entries)}")

    if corrupted_entries:
        print("\nCorrupted entries details:")
        for tyr_id, login, raw_prefix in corrupted_entries:
            print(f"  tyr_id={tyr_id}, login={login}, prefix='{raw_prefix}'")

    # Generate SQL
    sql = generate_sql(valid_entries, corrupted_entries)

    if args.execute:
        import os

        db_uri = args.db_uri or os.environ.get("SQLALCHEMY_DATABASE_URI")
        if not db_uri:
            print(
                "ERROR: --db-uri or SQLALCHEMY_DATABASE_URI env var required for --execute mode"
            )
            sys.exit(1)
        print(f"\nExecuting against database...")
        execute_sql(sql, db_uri)
    else:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(sql)
        print(f"\nSQL written to: {args.output}")
        print("Review the file, then execute it against your database.")


if __name__ == "__main__":
    main()

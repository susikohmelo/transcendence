
#!/bin/bash

DB_PATH="/app/data/db.sqlite"

mkdir -p "/app/data"

# Create the SQLite DB and users table
sqlite3 $DB_PATH <<EOF
CREATE TABLE IF NOT EXISTS users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT UNIQUE NOT NULL,
	password TEXT NOT NULL,
	created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
EOF

echo "SQLite database initialized at $DB_PATH"

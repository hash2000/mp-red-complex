CREATE TABLE accounts (
    user_hash TEXT PRIMARY KEY,           -- SHA-256 от masterSeed (имя папки)
    username TEXT NOT NULL UNIQUE,        -- Логин для входа
    created_at INTEGER NOT NULL,          -- Unix timestamp
    last_login INTEGER                    -- Unix timestamp
);

CREATE INDEX idx_accounts_username ON accounts(username);

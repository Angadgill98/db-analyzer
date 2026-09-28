CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    mail TEXT NOT NULL UNIQUE,
    pass TEXT NOT NULL,
    name TEXT NOT NULL,
    db UUID[] NOT NULL DEFAULT '{}'
);
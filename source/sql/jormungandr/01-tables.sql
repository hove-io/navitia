-- 01-tables.sql
-- création du schema si il n'existe pas, et des tables
DO $$
BEGIN

    CASE WHEN (select count(*) = 0 from pg_namespace where nspname = 'jormungandr')
    THEN
        CREATE SCHEMA jormungandr;
    ELSE
        RAISE NOTICE 'schema "jormungandr" already exists, skipping';
    END CASE;

END$$;


CREATE TABLE IF NOT EXISTS public.database_version (
    version INTEGER NOT NULL
);


CREATE TABLE IF NOT EXISTS jormungandr.user(
    id SERIAL PRIMARY KEY,
    login text UNIQUE,
    email text UNIQUE
);

CREATE TABLE IF NOT EXISTS jormungandr.key(
    id SERIAL PRIMARY KEY,
    user_id int NOT NULL REFERENCES jormungandr.user,
    token text NOT NULL,
    valid_until date
);

CREATE TABLE IF NOT EXISTS jormungandr.instance(
    id SERIAL PRIMARY KEY,
    key text UNIQUE NOT NULL,
    name text,
    free bool default false
);

CREATE TABLE IF NOT EXISTS jormungandr.api(
    id SERIAL PRIMARY KEY,
    name   text NOT NULL UNIQUE
    --method text NOT NULL UNIQUE,
);

CREATE TABLE IF NOT EXISTS jormungandr.authorization(
    user_id int REFERENCES jormungandr.user,
    instance_id int REFERENCES jormungandr.instance,
    api_id int REFERENCES jormungandr.api,
--    max_allowed_request int,
    PRIMARY KEY (user_id, instance_id, api_id)
);


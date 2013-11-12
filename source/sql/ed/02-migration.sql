DO $$
    BEGIN
        BEGIN
            ALTER TABLE realtime.message ADD COLUMN is_active BOOLEAN NOT NULL DEFAULT TRUE;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column is_active already exists in realtime.message.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE realtime.at_perturbation ADD COLUMN is_active BOOLEAN NOT NULL DEFAULT TRUE;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column is_active already exists in realtime.at_perturbation.';
        END;
    END;
$$;

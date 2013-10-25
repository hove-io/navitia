
DO $$
    DECLARE db_version int;
BEGIN
    db_version := coalesce((select version from public.database_version limit 1), 0);
    CASE WHEN db_version < 1
    THEN
       RAISE NOTICE 'update database to version 1';

        -- TODO
        INSERT INTO jormungandr.api (name) VALUES ('ALL');

        INSERT INTO public.database_version VALUES(1);
    ELSE
        RAISE NOTICE 'database already in version 1, skipping';
    END CASE;
END$$;


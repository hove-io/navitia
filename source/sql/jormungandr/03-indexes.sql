-- 03-indexes.sql
-- création des index qui n'existe pas
DO $$
BEGIN

--    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'poi_type_uri_idx')
--    THEN
--        CREATE UNIQUE INDEX poi_type_uri_idx ON navitia.poi_type(uri);
--    ELSE
--        RAISE NOTICE 'relation "poi_type_uri_idx" already exists, skipping';
--    END CASE;


END$$;



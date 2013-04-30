-- 03-indexes.sql
-- création des index qui n'existe pas
DO $$
BEGIN

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'poi_type_uri_idx')
    THEN
        CREATE UNIQUE INDEX poi_type_uri_idx ON navitia.poi_type(uri);
    ELSE
        RAISE NOTICE 'relation "poi_type_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'poi_uri_idx')
    THEN
        CREATE UNIQUE INDEX poi_uri_idx ON navitia.poi(uri);
    ELSE
        RAISE NOTICE 'relation "poi_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'commercial_mode_uri_idx')
    THEN
        CREATE UNIQUE INDEX commercial_mode_uri_idx ON navitia.commercial_mode(uri);
    ELSE
        RAISE NOTICE 'relation "commercial_mode_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'physical_mode_uri_idx')
    THEN
        CREATE UNIQUE INDEX physical_mode_uri_idx ON navitia.physical_mode(uri);
    ELSE
        RAISE NOTICE 'relation "physical_mode_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'company_uri_idx')
    THEN
        CREATE UNIQUE INDEX company_uri_idx ON navitia.company(uri);
    ELSE
        RAISE NOTICE 'relation "company_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'network_uri_idx')
    THEN
        CREATE UNIQUE INDEX network_uri_idx ON navitia.network(uri);
    ELSE
        RAISE NOTICE 'relation "network_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'line_uri_idx')
    THEN
        CREATE UNIQUE INDEX line_uri_idx ON navitia.line(uri);
    ELSE
        RAISE NOTICE 'relation "line_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'route_uri_idx')
    THEN
        CREATE UNIQUE INDEX route_uri_idx ON navitia.route(uri);
    ELSE
        RAISE NOTICE 'relation "route_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'journey_pattern_uri_idx')
    THEN
        CREATE UNIQUE INDEX journey_pattern_uri_idx ON navitia.journey_pattern(uri);
    ELSE
        RAISE NOTICE 'relation "journey_pattern_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'vehicle_journey_uri_idx')
    THEN
        CREATE UNIQUE INDEX vehicle_journey_uri_idx ON navitia.vehicle_journey(uri);
    ELSE
        RAISE NOTICE 'relation "vehicle_journey_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'stop_area_uri_idx')
    THEN
        CREATE UNIQUE INDEX stop_area_uri_idx ON navitia.stop_area(uri);
    ELSE
        RAISE NOTICE 'relation "stop_area_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'stop_point_uri_idx')
    THEN
        CREATE UNIQUE INDEX stop_point_uri_idx ON navitia.stop_point(uri);
    ELSE
        RAISE NOTICE 'relation "stop_point_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'connection_uri_idx')
    THEN
        CREATE UNIQUE INDEX connection_uri_idx ON navitia.connection(uri);
    ELSE
        RAISE NOTICE 'relation "connection_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'journey_pattern_point_uri_idx')
    THEN
        CREATE UNIQUE INDEX journey_pattern_point_uri_idx ON navitia.journey_pattern_point(uri);
    ELSE
        RAISE NOTICE 'relation "journey_pattern_point_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'journey_pattern_point_connection_uri_idx')
    THEN
        CREATE UNIQUE INDEX journey_pattern_point_connection_uri_idx ON navitia.journey_pattern_point_connection(uri);
    ELSE
        RAISE NOTICE 'relation "journey_pattern_point_connection_uri_idx" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_indexes where indexname = 'admin_uri_idx')
    THEN
        CREATE UNIQUE INDEX admin_uri_idx ON navitia.admin(uri);
    ELSE
        RAISE NOTICE 'relation "admin_uri_idx" already exists, skipping';
    END CASE;


END$$;



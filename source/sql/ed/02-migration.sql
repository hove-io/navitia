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

DO $$
    BEGIN
        BEGIN
            ALTER TABLE realtime.message ADD COLUMN message_status_id int NOT NULL REFERENCES realtime.message_status;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column message_status_id already exists in realtime.message.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.connection ADD COLUMN display_duration integer NOT NULL DEFAULT 0;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column display_duration already exists in navitia.connection.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.vehicle_journey ADD COLUMN start_time integer;
            ALTER TABLE navitia.vehicle_journey ADD COLUMN end_time integer;
            ALTER TABLE navitia.vehicle_journey ADD COLUMN headway_sec integer;
            ALTER TABLE navitia.stop_time DROP COLUMN start_time;
            ALTER TABLE navitia.stop_time DROP COLUMN end_time;
            ALTER TABLE navitia.stop_time DROP COLUMN headway_sec;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column start_time,end_time,headway_sec already exists in navitia.vehicle_journey';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.stop_point ADD COLUMN platform_code TEXT NULL DEFAULT NULL;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column platform_code already exists in navitia.stop_point.';
        END;
    END;
$$;

CREATE OR REPLACE FUNCTION add_external_code(tblname varchar) RETURNS VOID AS $BODY$
    BEGIN
        EXECUTE format('ALTER TABLE navitia.%I ADD COLUMN external_code TEXT;', tblname);
        EXECUTE format('UPDATE navitia.%I SET external_code = uri;', tblname);
        EXECUTE format('ALTER TABLE navitia.%I ALTER external_code SET NOT NULL', tblname);
    EXCEPTION
        WHEN duplicate_column THEN RAISE NOTICE 'columun external_code already exists in %', $1;
    END;
$BODY$ LANGUAGE plpgsql;

DO
$BODY$
DECLARE
    m varchar;
    arr varchar[] = array['network', 'line', 'route', 'stop_area', 'stop_point', 'vehicle_journey'];
BEGIN
        FOREACH m IN ARRAY arr
        LOOP
            PERFORM add_external_code(m);
        END LOOP;
END;
$BODY$ LANGUAGE plpgsql;


DO $$
    DECLARE count_multi int;
BEGIN
    count_multi := coalesce((select count(*) from georef.admin where geometrytype(boundary::geometry) = 'MULTIPOLYGON' limit 1), 0);
    CASE WHEN count_multi = 0
        THEN
            -- Ajout d'une colonne temporaire
            ALTER TABLE georef.admin ADD COLUMN boundary_tmp GEOGRAPHY(POLYGON, 4326);
            -- Déplacer le contenu de la colonne boundary dans boundary_tmp
            UPDATE georef.admin set boundary_tmp=boundary;
            -- Vidage de la colonne boundary
            UPDATE georef.admin  SET boundary = NULL;
            -- Changement du type de boundary
            ALTER TABLE georef.admin ALTER COLUMN boundary TYPE GEOGRAPHY(MULTIPOLYGON, 4326);
            -- Déplacer le contenu de la colonne boundary_tmp dans boundary
            UPDATE georef.admin  SET boundary = ST_Multi(st_astext(boundary_tmp));
            -- Suppression de la colonne temporaire
            ALTER TABLE georef.admin DROP COLUMN boundary_tmp;
    ELSE
        RAISE NOTICE 'column boundary already type MULTIPOLYGON, skipping';
    END CASE;
END
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.line ADD COLUMN sort integer NOT NULL DEFAULT 2147483647;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column sort already exists in navitia.line.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.network ADD COLUMN sort integer NOT NULL DEFAULT 2147483647;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column sort already exists in navitia.network.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.network ADD COLUMN website TEXT;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column website already exists in navitia.network.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.stop_area ADD visible BOOLEAN NOT NULL DEFAULT True;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column visible already exists in navitia.stop_area.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE georef.poi ADD COLUMN visible BOOLEAN NOT NULL DEFAULT True;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column visible already exists in navitia.poi.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE georef.poi ADD COLUMN address_name TEXT;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column address_name already exists in navitia.poi.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE georef.poi ADD COLUMN address_number TEXT;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column address_number already exists in navitia.poi.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.vehicle_journey ADD
                COLUMN previous_vehicle_journey_id BIGINT REFERENCES navitia.vehicle_journey;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column previous_vehicle_journey already exists in navitia.vehicle_journey';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.vehicle_journey ADD
                COLUMN next_vehicle_journey_id BIGINT REFERENCES navitia.vehicle_journey;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column next_vehicle_journey already exists in navitia.vehicle_journey';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.vehicle_journey ADD
                COLUMN utc_to_local_offset INT REFERENCES navitia.vehicle_journey;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column utc_to_local_offset already exists in navitia.vehicle_journey';
        END;
    END;
$$;

DO $$
    DECLARE count_admin int;
BEGIN
    count_admin := coalesce((select  count(*) from  pg_catalog.pg_tables where schemaname = 'navitia' and tablename='admin'), 0);
    CASE WHEN count_admin > 0
        THEN
			INSERT INTO georef.admin SELECT * FROM navitia.admin;
			INSERT INTO georef.rel_admin_admin SELECT * FROM navitia.rel_admin_admin;
    ELSE
        RAISE NOTICE 'table admin already exists, skipping';
    END CASE;
END
$$;

DO $$
    DECLARE count_poi int;
BEGIN
    count_poi := coalesce((select  count(*) from  pg_catalog.pg_tables where schemaname = 'navitia' and tablename='poi'), 0);
    CASE WHEN count_poi > 0
        THEN
			INSERT INTO georef.poi_type SELECT * FROM navitia.poi_type;
			insert into georef.poi (id,weight,coord,name,uri,visible,poi_type_id,address_name,address_number)
			select id,weight,coord,name,uri,visible,poi_type_id,address_name,address_number from navitia.poi;
			INSERT INTO georef.poi_properties SELECT * FROM navitia.poi_properties;
    ELSE
        RAISE NOTICE 'table poi already exists, skipping';
    END CASE;
END
$$;

DO $$
    DECLARE count_synonym int;
BEGIN
    count_synonym := coalesce((select  count(*) from  pg_catalog.pg_tables where schemaname = 'navitia' and tablename='synonym'), 0);
    CASE WHEN count_synonym > 0
        THEN
			INSERT INTO georef.synonym SELECT * FROM navitia.synonym;
    ELSE
        RAISE NOTICE 'table synonym already exists, skipping';
    END CASE;
END
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.stop_area ADD COLUMN timezone TEXT;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column timezone already exists in navitia.stop_area';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.parameters ADD COLUMN timezone TEXT;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column timezone already exists in navitia.parameters';
        END;
    END;
$$;

DROP TABLE IF EXISTS navitia.admin CASCADE;
DROP TABLE IF EXISTS navitia.rel_admin_admin CASCADE;
DROP TABLE IF EXISTS navitia.poi CASCADE;
DROP TABLE IF EXISTS navitia.synonym;

DROP TABLE IF EXISTS navitia.alias;
DROP TABLE IF EXISTS navitia.rel_stop_point_admin;
DROP TABLE IF EXISTS navitia.rel_stop_area_admin;
DROP TABLE IF EXISTS navitia.rel_poi_admin;
DROP FUNCTION IF EXISTS georef.match_stop_area_to_admin();
DROP FUNCTION IF EXISTS georef.match_stop_point_to_admin();
DROP FUNCTION IF EXISTS georef.update_admin_coord();
DROP FUNCTION IF EXISTS georef.match_admin_to_admin();
DROP TABLE IF EXISTS navitia.journey_pattern_point_connection CASCADE;
DROP FUNCTION IF EXISTS georef.update_boundary(bigint);

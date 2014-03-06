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
    count_multi := coalesce((select count(*) from navitia.admin where geometrytype(boundary::geometry) = 'MULTIPOLYGON' limit 1), 0);
    CASE WHEN count_multi = 0 
        THEN
            -- Ajout d'une colonne temporaire
            ALTER TABLE navitia.admin ADD COLUMN boundary_tmp GEOGRAPHY(POLYGON, 4326);
            -- Déplacer le contenu de la colonne boundary dans boundary_tmp
            UPDATE navitia.admin set boundary_tmp=boundary;
            -- Vidage de la colonne boundary
            UPDATE navitia.admin  SET boundary = NULL;
            -- Changement du type de boundary
            ALTER TABLE navitia.admin ALTER COLUMN boundary TYPE GEOGRAPHY(MULTIPOLYGON, 4326);
            -- Déplacer le contenu de la colonne boundary_tmp dans boundary
            UPDATE navitia.admin  SET boundary = ST_Multi(st_astext(boundary_tmp));
            -- Suppression de la colonne temporaire
            ALTER TABLE navitia.admin DROP COLUMN boundary_tmp;
    ELSE
        RAISE NOTICE 'column boundary already type MULTIPOLYGON, skipping';
    END CASE;
END
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.line ADD COLUMN sort integer NOT NULL DEFAULT -1;
        EXCEPTION
            WHEN duplicate_column THEN RAISE NOTICE 'column sort already exists in navitia.line.';
        END;
    END;
$$;

DO $$
    BEGIN
        BEGIN
            ALTER TABLE navitia.network ADD COLUMN sort integer NOT NULL DEFAULT -1;
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


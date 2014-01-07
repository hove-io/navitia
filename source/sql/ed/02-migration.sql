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

CREATE OR REPLACE FUNCTION add_original_uri(tblname varchar) RETURNS VOID AS $BODY$
    BEGIN
        EXECUTE format('ALTER TABLE navitia.%I ADD COLUMN original_uri TEXT;', tblname);
        EXECUTE format('UPDATE navitia.%I SET original_uri = uri;', tblname);
        EXECUTE format('ALTER TABLE navitia.%I ALTER original_uri SET NOT NULL', tblname);
    EXCEPTION
        WHEN duplicate_column THEN RAISE NOTICE 'columun original_uri already exists in %', $1;
    END;
$BODY$ LANGUAGE plpgsql;

DO
$BODY$
DECLARE
    m varchar;
    arr varchar[] = array['physical_mode', 'commercial_mode', 'contributor', 'company', 'network', 'line', 'route', 'journey_pattern', 'vehicle_journey', 'stop_area', 'stop_point', 'journey_pattern_point'];
BEGIN
        FOREACH m IN ARRAY arr
        LOOP
            PERFORM add_original_uri(m);
        END LOOP;
END;
$BODY$ LANGUAGE plpgsql;


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
    DECLARE count_multi int;
BEGIN
    count_multi := coalesce((select count(*) from navitia.admin where geometrytype(boundary) = 'MULTIPOLYGON' limit 1), 0);
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
END$$;


DO $$
    DECLARE db_version int;
BEGIN
    db_version := coalesce((select version from public.database_version limit 1), 0);
    CASE WHEN db_version < 1
    THEN
        RAISE NOTICE 'update database to version 1';
        INSERT INTO navitia.connection_type (id, name) VALUES
            (0, 'StopPoint'), (1, 'StopArea'),
            (2, 'Walking'), (3, 'VJ'), (4, 'Guaranteed'),
            (5, 'Default');

        INSERT INTO navitia.connection_kind (id, name) VALUES
            (0, 'extension'), (1, 'guarantee'), (2, 'undefined');
        INSERT INTO navitia.odt_type (id, name) VALUES
    (0, 'regular_line'), (1, 'virtual_with_stop_time'), (2, 'virtual_without_stop_time'), (3, 'stop_point_to_stop_point'), (4, 'adress_to_stop_point'), (5, 'odt_point_to_point');
        INSERT INTO public.database_version VALUES(1);
    ELSE
        RAISE NOTICE 'database already in version 1, skipping';
    END CASE;
END$$;

DO $$
    DECLARE db_version int;
BEGIN
    db_version := coalesce((select version from public.database_version limit 1), 0);
    CASE WHEN db_version < 2
        THEN
        insert into navitia.object_type VALUES
        (0, 'ValidityPattern'),
        (1, 'Line'),
        (2, 'JourneyPattern'),
        (3, 'VehicleJourney'),
        (4, 'StopPoint'),
        (5, 'StopArea'),
        (6, 'Network'),
        (7, 'PhysicalMode'),
        (8, 'CommercialMode'),
        (9, 'Connection'),
        (10, 'JourneyPatternPoint'),
        (11, 'Company'),
        (12, 'Route'),
        (13, 'POI'),
        (15, 'StopPointConnection'),
        (16, 'Contributor'),
        (17, 'StopTime'),
        (18, 'Address'),
        (19, 'Coord'),
        (20, 'Unknown'),
        (21, 'Way'),
        (22, 'Admin'),
        (23, 'POIType');
        update public.database_version set version=2;
    ELSE
        RAISE NOTICE 'database already in version 1, skipping';
    END CASE;
END$$;

DO $$
    DECLARE db_version int;
BEGIN
    db_version := coalesce((select version from public.database_version limit 1), 0);
    CASE WHEN db_version < 3
        THEN
        INSERT INTO navitia.connection_kind (id, name) VALUES (6, 'stay_in');
        update public.database_version set version=3;
    ELSE
        RAISE NOTICE 'database already in version 3, skipping';
    END CASE;
END$$;

DO $$
    DECLARE db_version int;
BEGIN
    db_version := coalesce((select version from public.database_version limit 1), 0);
    CASE WHEN db_version < 4
        THEN
	insert into realtime.message_status values (0, 'information'),
		(1, 'warning'),
		(2, 'disrupt');
        update public.database_version set version=4;
    ELSE
        RAISE NOTICE 'database already in version 4, skipping';
    END CASE;
END$$;


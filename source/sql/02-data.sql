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

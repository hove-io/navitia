-- 01-tables.sql
-- création du schema si il n'existe pas, et des tables
DO $$
BEGIN

    CASE WHEN (select count(*) = 0 from pg_namespace where nspname = 'navitia')
    THEN
        CREATE SCHEMA navitia;
    ELSE
        RAISE NOTICE 'schema "navitia" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_namespace where nspname = 'georef')
    THEN
        CREATE SCHEMA georef;
    ELSE
        RAISE NOTICE 'schema "georef" already exists, skipping';
    END CASE;

    CASE WHEN (select count(*) = 0 from pg_namespace where nspname = 'realtime')
    THEN
        CREATE SCHEMA realtime;
    ELSE
        RAISE NOTICE 'schema "realtime" already exists, skipping';
    END CASE;
END$$;


CREATE TABLE IF NOT EXISTS public.database_version (
    version INTEGER NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.parameters (
    beginning_date DATE NOT NULL,
    end_date DATE NOT NULL
);
COMMENT ON COLUMN navitia.Parameters.beginning_date IS 'date de début de validité des données => ValidityPattern::beginning_date';


CREATE TABLE IF NOT EXISTS navitia.connection_kind (
    id BIGINT NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.connection_type (
    id BIGINT NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.odt_type (
    id BIGINT NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.properties (
    id BIGINT PRIMARY KEY,
-- Accès UFR
    wheelchair_boarding BOOLEAN NOT NULL,
-- Abris couvert
    sheltered BOOLEAN NOT NULL,
-- Ascenseur
    elevator BOOLEAN NOT NULL,
-- Escalier mécanique
    escalator BOOLEAN NOT NULL,
-- Embarquement vélo
    bike_accepted BOOLEAN NOT NULL,
-- Parc vélo
    bike_depot BOOLEAN NOT NULL,
-- Annonce visuelle
    visual_announcement BOOLEAN NOT NULL,
-- Annonce sonore
    audible_announcement BOOLEAN NOT NULL,
-- Accompagnement à l'arrêt
    appropriate_escort BOOLEAN NOT NULL,
-- Information claire à l'arrêt
    appropriate_signage BOOLEAN NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.vehicle_properties (
    id BIGSERIAL PRIMARY KEY,
-- Accès UFR
    wheelchair_accessible BOOLEAN NOT NULL,
-- Embarquement vélo
    bike_accepted BOOLEAN NOT NULL,
-- Air conditionné
    air_conditioned BOOLEAN NOT NULL,
-- Annonce visuelle
    visual_announcement BOOLEAN NOT NULL,
-- Annonce sonore
    audible_announcement BOOLEAN NOT NULL,
-- Accompagnement
    appropriate_escort BOOLEAN NOT NULL,
-- Information claire
    appropriate_signage BOOLEAN NOT NULL,
-- Ligne Scolaire
    school_vehicle BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.validity_pattern (
    id BIGINT PRIMARY KEY,
    days BIT VARYING(400) NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.physical_mode (
    id BIGINT PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.commercial_mode (
    id BIGINT PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.contributor (
    id BIGINT PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.company (
    id BIGINT PRIMARY KEY,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    address_name TEXT,
    address_number TEXT,
    address_type_name TEXT,
    phone_number TEXT,
    mail TEXT,
    website TEXT,
    fax TEXT
);

CREATE TABLE IF NOT EXISTS navitia.network (
    id BIGINT PRIMARY KEY,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL,
    sort integer NOT NULL DEFAULT 2147483647,
    website TEXT
);

CREATE TABLE IF NOT EXISTS navitia.line (
    id BIGINT PRIMARY KEY,
    network_id BIGINT NOT NULL REFERENCES navitia.network,
    commercial_mode_id BIGINT NOT NULL REFERENCES navitia.commercial_mode,
    comment TEXT,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL,
    name TEXT NOT NULL,
    code TEXT,
    color TEXT,
    sort integer NOT NULL DEFAULT 2147483647
);

CREATE TABLE IF NOT EXISTS navitia.rel_line_company (
    line_id BIGINT NOT NULL REFERENCES navitia.line,
    company_id BIGINT NOT NULL REFERENCES navitia.company,
    CONSTRAINT rel_line_company_pk PRIMARY KEY (line_id, company_id)
);

CREATE TABLE IF NOT EXISTS navitia.route (
    id BIGINT PRIMARY KEY,
    line_id BIGINT NOT NULL REFERENCES navitia.line,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.journey_pattern (
    id BIGINT PRIMARY KEY,
    route_id BIGINT NOT NULL REFERENCES navitia.route,
    physical_mode_id BIGINT NOT NULL REFERENCES navitia.physical_mode,
    comment TEXT,
    uri TEXT NOT NULL,
    name TEXT NOT NULL,
    is_frequence BOOLEAN NOT NULL
);

DO $$
BEGIN
--pour migrer les données il faut que la table VJ existe
    CASE WHEN ((SELECT COUNT(1) = 0 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME='navitia.journey_pattern' AND COLUMN_NAME='physical_mode_id')
        AND (SELECT COUNT(1) = 1 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME='navitia.vehicle_journey' AND COLUMN_NAME='physical_mode_id'))
    THEN
    -- Ajout de la nouvelle colonne
        CREATE TABLE IF NOT EXISTS navitia.journey_pattern_tmp (
            id BIGINT PRIMARY KEY,
            route_id BIGINT NOT NULL REFERENCES navitia.route,
            physical_mode_id BIGINT NOT NULL REFERENCES navitia.physical_mode,
            comment TEXT,
            uri TEXT NOT NULL,
            name TEXT NOT NULL,
            is_frequence BOOLEAN NOT NULL
        );
        INSERT INTO navitia.journey_pattern_tmp
        SELECT
        navitia.journey_pattern.id,
        navitia.journey_pattern.route_id,
        MAX(navitia.vehicle_journey.physical_mode_id),
        navitia.journey_pattern.comment,
        navitia.journey_pattern.uri,
        navitia.journey_pattern.name,
        navitia.journey_pattern.is_frequence
        FROM
        navitia.journey_pattern,
        navitia.vehicle_journey
        WHERE
        navitia.vehicle_journey.journey_pattern_id = navitia.journey_pattern.id
        GROUP BY
        navitia.journey_pattern.id,
            navitia.journey_pattern.route_id,
            navitia.journey_pattern.comment,
            navitia.journey_pattern.uri,
            navitia.journey_pattern.name,
            navitia.journey_pattern.is_frequence;

        DROP TABLE navitia.journey_pattern CASCADE;
        ALTER TABLE navitia.journey_pattern_tmp RENAME TO journey_pattern;
    ELSE
    END CASE;
END$$;

CREATE TABLE IF NOT EXISTS navitia.vehicle_journey (
    id BIGINT PRIMARY KEY,
--    properties_id BIGINT REFERENCES navitia.properties,
    adapted_validity_pattern_id BIGINT NOT NULL REFERENCES navitia.validity_pattern,
    validity_pattern_id BIGINT REFERENCES navitia.validity_pattern,
    company_id BIGINT NOT NULL REFERENCES navitia.company,
    journey_pattern_id BIGINT NOT NULL REFERENCES navitia.journey_pattern,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL,
    comment TEXT,
    odt_message TEXT, -- Utiliser pour stocker le message TAD
    name TEXT NOT NULL,
    odt_type_id BIGINT NULL,
    vehicle_properties_id BIGINT NULL REFERENCES navitia.vehicle_properties,
    theoric_vehicle_journey_id BIGINT REFERENCES navitia.vehicle_journey,
    previous_vehicle_journey_id BIGINT REFERENCES navitia.vehicle_journey,
    next_vehicle_journey_id BIGINT REFERENCES navitia.vehicle_journey,
    start_time INTEGER,
    end_time INTEGER,
    headway_sec INTEGER
);

ALTER TABLE navitia.vehicle_journey DROP COLUMN IF EXISTS physical_mode_id;

CREATE TABLE IF NOT EXISTS navitia.stop_area (
    id BIGINT PRIMARY KEY,
    properties_id BIGINT REFERENCES navitia.properties,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL,
    name TEXT NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    comment TEXT,
    visible BOOLEAN NOT NULL DEFAULT True
);

CREATE TABLE IF NOT EXISTS navitia.stop_point (
    id BIGINT PRIMARY KEY,
    properties_id BIGINT REFERENCES navitia.properties,
    uri TEXT NOT NULL,
    external_code TEXT NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    fare_zone INTEGER,
    name TEXT NOT NULL,
    comment TEXT,
    stop_area_id BIGINT NOT NULL REFERENCES navitia.stop_area,
    platform_code TEXT
);



CREATE TABLE IF NOT EXISTS navitia.connection (
    departure_stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point,
    destination_stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point,
    connection_type_id BIGINT NOT NULL REFERENCES navitia.connection_type,
    properties_id BIGINT REFERENCES navitia.properties,
    duration INTEGER NOT NULL,
    max_duration INTEGER NOT NULL,
    display_duration INTEGER NOT NULL,
    CONSTRAINT connection_pk PRIMARY KEY (departure_stop_point_id, destination_stop_point_id)
);


CREATE TABLE IF NOT EXISTS navitia.journey_pattern_point (
    id BIGINT PRIMARY KEY,
    journey_pattern_id BIGINT NOT NULL REFERENCES navitia.journey_pattern,
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    "order" INTEGER NOT NULL,
    comment TEXT,
    stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point
);


CREATE TABLE IF NOT EXISTS navitia.stop_time (
    id BIGSERIAL PRIMARY KEY,
    vehicle_journey_id BIGINT NOT NULL REFERENCES navitia.vehicle_journey,
    journey_pattern_point_id BIGINT NOT NULL REFERENCES navitia.journey_pattern_point,
    arrival_time INTEGER,
    departure_time INTEGER,
    local_traffic_zone INTEGER,
    odt BOOLEAN NOT NULL,
    pick_up_allowed BOOLEAN NOT NULL,
    drop_off_allowed BOOLEAN NOT NULL,
    is_frequency BOOLEAN NOT NULL,
    comment TEXT,
    date_time_estimated BOOLEAN NOT NULL DEFAULT false,
    properties_id BIGINT REFERENCES navitia.properties
);

CREATE TABLE IF NOT EXISTS navitia.week_pattern (
    id BIGSERIAL PRIMARY KEY,
	monday BOOLEAN NOT NULL,
	tuesday BOOLEAN NOT NULL,
	wednesday BOOLEAN NOT NULL,
	thursday BOOLEAN NOT NULL,
	friday BOOLEAN NOT NULL,
	saturday BOOLEAN NOT NULL,
	sunday BOOLEAN NOT NULL
);

-- enum for exception type
DO $$
BEGIN
    CASE WHEN (SELECT count(*) = 0 FROM pg_type where pg_type.typname='exception_type')
    THEN
        CREATE TYPE exception_type AS ENUM ('Add', 'Sub');
    ELSE
        RAISE NOTICE 'exception_type already exists, skipping';
    END CASE;

END$$;


CREATE TABLE IF NOT EXISTS navitia.calendar (
    id BIGSERIAL PRIMARY KEY,
    external_code TEXT NOT NULL,
    uri TEXT NOT NULL,
    name TEXT NOT NULL,
    week_pattern_id BIGINT NOT NULL REFERENCES navitia.week_pattern
);

CREATE TABLE IF NOT EXISTS navitia.exception_date (
    id BIGSERIAL PRIMARY KEY,
    datetime DATE NOT NULL,
    type_ex exception_type NOT NULL,
    calendar_id BIGINT NOT NULL REFERENCES navitia.calendar
);

CREATE TABLE IF NOT EXISTS navitia.period (
    id BIGSERIAL PRIMARY KEY,
    calendar_id BIGINT NOT NULL REFERENCES navitia.calendar,
    begin_date DATE NOT NULL,
    end_date DATE NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.rel_calendar_line (
    calendar_id BIGINT NOT NULL REFERENCES navitia.calendar,
    line_id BIGINT NOT NULL REFERENCES navitia.line,
    CONSTRAINT rel_calendar_line_pk PRIMARY KEY (calendar_id, line_id)
);

-- Schéma Georef

CREATE TABLE IF NOT EXISTS georef.synonym (
    id BIGINT PRIMARY KEY,
    key TEXT NOT NULL,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS georef.poi_type (
    id BIGINT PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS georef.poi (
    id BIGINT PRIMARY KEY,
    weight INTEGER NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    visible BOOLEAN NOT NULL DEFAULT True,
    poi_type_id BIGINT NOT NULL REFERENCES georef.poi_type,
    address_name TEXT,
    address_number TEXT
);

CREATE TABLE IF NOT EXISTS georef.poi_properties(
  poi_id bigint NOT NULL REFERENCES georef.poi,
  key text,
  value text
);

CREATE TABLE IF NOT EXISTS georef.admin (
    id BIGINT PRIMARY KEY,
    name TEXT NOT NULL,
    comment TEXT,
    post_code TEXT,
    insee TEXT,
    level INTEGER NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    boundary GEOGRAPHY(MULTIPOLYGON, 4326),
    uri TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS georef.rel_admin_admin (
    master_admin_id BIGINT NOT NULL REFERENCES georef.admin,
    admin_id BIGINT NOT NULL REFERENCES georef.admin,
    CONSTRAINT rel_admin_admin_pk PRIMARY KEY (master_admin_id, admin_id)
);

CREATE TABLE IF NOT EXISTS georef.way (
                id BIGSERIAL PRIMARY KEY,
                name TEXT NOT NULL,
                uri TEXT NOT NULL,
                type TEXT
);

CREATE TABLE IF NOT EXISTS georef.node (
                id BIGSERIAL PRIMARY KEY,
                coord GEOGRAPHY(POINT, 4326)
);

CREATE TABLE IF NOT EXISTS georef.edge (
                source_node_id BIGINT NOT NULL REFERENCES georef.node,
                target_node_id BIGINT NOT NULL REFERENCES georef.node,
                way_id BIGINT NOT NULL,
                the_geog GEOGRAPHY(LINESTRING, 4326) NOT NULL,
                pedestrian_allowed BOOLEAN NOT NULL,
                cycles_allowed BOOLEAN NOT NULL,
                cars_allowed BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS georef.house_number (
                way_id BIGINT REFERENCES georef.way,
                coord GEOGRAPHY(POINT, 4326) NOT NULL,
                number TEXT NOT NULL,
                left_side BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS georef.rel_way_admin (
                admin_id BIGINT NOT NULL REFERENCES georef.admin,
                way_id BIGINT NOT NULL REFERENCES georef.way,
                CONSTRAINT rel_way_admin_pk PRIMARY KEY (admin_id, way_id)
);

--- Tables temporaires
CREATE UNLOGGED TABLE IF NOT EXISTS georef.tmp_rel_way_admin (
                admin_id BIGINT NOT NULL,
                way_id BIGINT NOT NULL
);
-- Cette table sert à redresser les données : Fusion des voies
CREATE UNLOGGED TABLE IF NOT EXISTS georef.fusion_ways
(
  id bigint NOT NULL, -- le nouveau id de la voie
  way_id bigint NOT NULL
);


create table if NOT EXISTS navitia.object_type(
    id int primary key,
    name text NOT NULL
);

CREATE TABLE IF NOT EXISTS realtime.message_status(
    id int primary key,
	name TEXT NOT NULL    
);

CREATE TABLE IF NOT EXISTS realtime.message(
    id BIGSERIAL primary key,
    uri text NOT NULL,
    start_publication_date timestamptz NOT NULL,
    end_publication_date timestamptz NOT NULL,
    start_application_date timestamptz NOT NULL,
    end_application_date timestamptz NOT NULL,
    start_application_daily_hour timetz NOT NULL,
    end_application_daily_hour timetz NOT NULL,
    active_days bit(8) NOT NULL,
    object_uri text NOT NULL,
    object_type_id int NOT NULL REFERENCES navitia.object_type,
    message_status_id int NOT NULL REFERENCES realtime.message_status,
    is_active BOOLEAN NOT NULL DEFAULT TRUE
);


CREATE TABLE IF NOT EXISTS realtime.localized_message(
    message_id BIGINT NOT NULL REFERENCES realtime.message,
    language text NOT NULL,
    body text NOT NULL,
    title text,
    CONSTRAINT localized_message_pk PRIMARY KEY (message_id, language)
);

CREATE TABLE IF NOT EXISTS realtime.at_perturbation(
    id BIGSERIAL primary key,
    uri text NOT NULL,
    start_application_date timestamptz NOT NULL,
    end_application_date timestamptz NOT NULL,
    start_application_daily_hour timetz NOT NULL,
    end_application_daily_hour timetz NOT NULL,
    active_days bit(8) NOT NULL,
    object_uri text NOT NULL,
    object_type_id int NOT NULL REFERENCES navitia.object_type,
    is_active BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS navitia.ticket(
	ticket_key TEXT PRIMARY KEY,
	ticket_title TEXT,
	ticket_comment TEXT
);

CREATE TABLE IF NOT EXISTS navitia.dated_ticket(
	id BIGINT PRIMARY KEY,
	ticket_id TEXT REFERENCES navitia.ticket,
	valid_from DATE NOT NULL,
	valid_to DATE NOT NULL,
	ticket_price INT NOT NULL,
	comments TEXT,
	currency TEXT
);

-- add enum
DO $$
BEGIN
    CASE WHEN (SELECT count(*) = 0 FROM pg_type where pg_type.typname='fare_od_mode')
    THEN
        CREATE TYPE fare_od_mode AS ENUM ('Zone', 'StopArea', 'Mode');
    ELSE
        RAISE NOTICE 'fare_od_mode already exists, skipping';
    END CASE;

    CASE WHEN (SELECT count(*) = 0 FROM pg_type where pg_type.typname='fare_transition_condition')
    THEN
        CREATE TYPE fare_transition_condition AS ENUM ('nothing', 'exclusive', 'with_changes');
    ELSE
        RAISE NOTICE 'fare_transition_condition already exists, skipping';
    END CASE;

END$$;


CREATE TABLE IF NOT EXISTS navitia.transition(
	id BIGINT PRIMARY KEY,
	before_change TEXT NOT NULL,
	after_change TEXT NOT NULL,
	start_trip TEXT NOT NULL,
	end_trip TEXT NOT NULL,
	global_condition fare_transition_condition NOT NULL,
    ticket_id TEXT,
    FOREIGN KEY (ticket_id) REFERENCES navitia.ticket(ticket_key)
);

CREATE TABLE IF NOT EXISTS navitia.origin_destination(
	id BIGINT PRIMARY KEY,
	origin_id TEXT NOT NULL,
	origin_mode fare_od_mode NOT NULL,
	destination_id TEXT NOT NULL,
	destination_mode fare_od_mode NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.od_ticket(
    id BIGINT PRIMARY KEY,
    od_id BIGINT NOT NULL REFERENCES navitia.origin_destination,
    ticket_id TEXT NOT NULL REFERENCES navitia.ticket
);

CREATE TABLE IF NOT EXISTS navitia.admin_stop_area(
    admin_id Text NOT NULL,
    stop_area_id BIGINT NOT NULL REFERENCES navitia.stop_area
);

-- enum for meta vj 
DO $$
BEGIN
    CASE WHEN (SELECT count(*) = 0 FROM pg_type where pg_type.typname='vj_classification')
    THEN
        CREATE TYPE vj_classification AS ENUM ('Theoric', 'Adapted', 'RealTime');
    ELSE
        RAISE NOTICE 'vj_classification already exists, skipping';
    END CASE;

END$$;

CREATE TABLE IF NOT EXISTS navitia.meta_vj(
    id BIGINT PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.rel_metavj_vj(
    meta_vj BIGINT REFERENCES navitia.meta_vj,
    vehicle_journey BIGINT REFERENCES navitia.vehicle_journey,
    vj_class vj_classification NOT NULL
);

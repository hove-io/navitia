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
END$$;


CREATE TABLE IF NOT EXISTS navitia.database_version (
    version INTEGER NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.parameters (
    beginning_date DATE NOT NULL
);
COMMENT ON COLUMN navitia.Parameters.beginning_date IS 'date de début de validité des données => ValidityPattern::beginning_date';


CREATE TABLE IF NOT EXISTS navitia.journey_pattern_point_connection_type (
    id BIGINT NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.connection_type (
    id BIGINT NOT NULL PRIMARY KEY,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.properties (
    id BIGSERIAL PRIMARY KEY,
    wheelchair_boarding BOOLEAN NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.synonym (
    id BIGSERIAL PRIMARY KEY,
    key TEXT NOT NULL,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.alias (
    id BIGSERIAL PRIMARY KEY,
    key TEXT NOT NULL,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.poi_type (
    id BIGSERIAL PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.poi (
    id BIGSERIAL PRIMARY KEY,
    weight INTEGER NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    poi_type_id BIGINT NOT NULL REFERENCES navitia.poi_type
);


CREATE TABLE IF NOT EXISTS navitia.admin (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    comment TEXT,
    post_code TEXT,
    insee TEXT,
    level INTEGER NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    boundary GEOGRAPHY(POLYGON, 4326),
    uri TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.rel_poi_admin (
    poi_id BIGINT NOT NULL REFERENCES navitia.poi,
    admin_id BIGINT NOT NULL REFERENCES navitia.admin,
    CONSTRAINT rel_poi_admin_pk PRIMARY KEY (poi_id, admin_id)
);


CREATE TABLE IF NOT EXISTS navitia.validity_pattern (
    id BIGSERIAL PRIMARY KEY,
    days BIT VARYING(400) NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.physical_mode (
    id BIGSERIAL PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS navitia.commercial_mode (
    id BIGSERIAL PRIMARY KEY,
    uri TEXT NOT NULL,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.company (
    id BIGSERIAL PRIMARY KEY,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.network (
    id BIGSERIAL PRIMARY KEY,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.line (
    id BIGSERIAL PRIMARY KEY,
    network_id BIGINT NOT NULL REFERENCES navitia.network,
    commercial_mode_id BIGINT NOT NULL REFERENCES navitia.commercial_mode,
    comment TEXT,
    uri TEXT NOT NULL,
    name TEXT NOT NULL,
    code TEXT,
    color TEXT
);

CREATE TABLE IF NOT EXISTS navitia.rel_line_company (
    line_id BIGINT NOT NULL REFERENCES navitia.line,
    company_id BIGINT NOT NULL REFERENCES navitia.company,
    CONSTRAINT rel_line_company_pk PRIMARY KEY (line_id, company_id)
);

CREATE TABLE IF NOT EXISTS navitia.route (
    id BIGSERIAL PRIMARY KEY,
    line_id BIGINT NOT NULL REFERENCES navitia.line,
    comment TEXT,
    name TEXT NOT NULL,
    uri TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.journey_pattern (
    id BIGSERIAL PRIMARY KEY,
    route_id BIGINT NOT NULL REFERENCES navitia.route,
    comment TEXT,
    uri TEXT NOT NULL,
    name TEXT NOT NULL,
    is_frequence BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS navitia.vehicle_journey (
    id BIGSERIAL PRIMARY KEY,
    properties_id BIGINT REFERENCES navitia.properties,
    adapted_validity_pattern BIGINT NOT NULL REFERENCES navitia.validity_pattern,
    validity_pattern_id BIGINT REFERENCES navitia.validity_pattern,
    company_id BIGINT NOT NULL REFERENCES navitia.company,
    physical_mode_id BIGINT NOT NULL REFERENCES navitia.physical_mode,
    journey_pattern_id BIGINT NOT NULL REFERENCES navitia.journey_pattern,
    uri TEXT NOT NULL,
    comment TEXT,
    name TEXT NOT NULL,
    theoric_vehicle_journey BIGINT REFERENCES navitia.vehicle_journey
);

CREATE TABLE IF NOT EXISTS navitia.stop_area (
    id BIGSERIAL PRIMARY KEY,
    properties_id BIGINT REFERENCES navitia.properties,
    uri TEXT NOT NULL,
    name TEXT NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    comment TEXT
);

CREATE TABLE IF NOT EXISTS navitia.rel_stop_area_admin (
    stop_area_id BIGINT NOT NULL REFERENCES navitia.stop_area,
    admin_id BIGINT NOT NULL REFERENCES navitia.admin,
    CONSTRAINT rel_stop_area_admin_pk PRIMARY KEY (stop_area_id, admin_id)
);

CREATE TABLE IF NOT EXISTS navitia.stop_point (
    id BIGSERIAL PRIMARY KEY,
    properties_id BIGINT REFERENCES navitia.properties,
    uri TEXT NOT NULL,
    coord GEOGRAPHY(POINT, 4326),
    fare_zone INTEGER,
    name TEXT NOT NULL,
    comment TEXT,
    stop_area_id BIGINT NOT NULL REFERENCES navitia.stop_area
);



CREATE TABLE IF NOT EXISTS navitia.rel_stop_point_admin (
    admin_id BIGINT NOT NULL REFERENCES navitia.admin,
    stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point,
    CONSTRAINT rel_stop_point_admin_pk PRIMARY KEY (admin_id, stop_point_id)
);

CREATE TABLE IF NOT EXISTS navitia.connection (
    departure_stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point,
    destination_stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point,
    connection_type_id BIGINT NOT NULL REFERENCES navitia.connection_type,
    properties_id BIGINT REFERENCES navitia.properties,
    duration INTEGER NOT NULL,
    max_duration INTEGER NOT NULL,
    uri TEXT NOT NULL,
    CONSTRAINT connection_pk PRIMARY KEY (departure_stop_point_id, destination_stop_point_id)
);


CREATE TABLE IF NOT EXISTS navitia.journey_pattern_point (
    id BIGSERIAL PRIMARY KEY,
    journey_pattern_id BIGINT NOT NULL REFERENCES navitia.journey_pattern,
    name TEXT NOT NULL,
    uri TEXT NOT NULL,
    "order" INTEGER NOT NULL,
    comment TEXT,
    stop_point_id BIGINT NOT NULL REFERENCES navitia.stop_point
);


CREATE TABLE IF NOT EXISTS navitia.journey_pattern_point_connection (
    departure_journey_pattern_point_id BIGINT NOT NULL REFERENCES navitia.journey_pattern_point,
    destination_journey_pattern_point_id BIGINT NOT NULL REFERENCES navitia.journey_pattern_point,
    jpp_connecton_type_id BIGINT NOT NULL REFERENCES navitia.journey_pattern_point_connection_type,
    length INTEGER NOT NULL,
    uri TEXT NOT NULL,
    CONSTRAINT journey_pattern_point_connection_pk PRIMARY KEY (departure_journey_pattern_point_id, destination_journey_pattern_point_id)
);


CREATE TABLE IF NOT EXISTS navitia.stop_time (
    id BIGSERIAL PRIMARY KEY,
    vehicle_journey_id BIGINT NOT NULL REFERENCES navitia.vehicle_journey,
    journey_pattern_point_id BIGINT NOT NULL REFERENCES navitia.journey_pattern_point,
    arrival_time INTEGER,
    departure_time INTEGER,
    local_traffic_zone INTEGER,
    start_time INTEGER,
    end_time INTEGER,
    headway_sec INTEGER,
    odt BOOLEAN NOT NULL,
    pick_up_allowed BOOLEAN NOT NULL,
    wheelchair_boarding BOOLEAN NOT NULL,
    drop_off_allowed BOOLEAN NOT NULL,
    is_frequency BOOLEAN NOT NULL
);

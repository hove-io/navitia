-- MAJ des way_id de la table georef.house_number
CREATE OR REPLACE FUNCTION georef.nearest_way_id(point_in GEOGRAPHY(POINT, 4326))
  RETURNS bigint AS
$func$
BEGIN
RETURN (select tmp.way_id from(
select ed.way_id, ST_Distance(ed.the_geog, point_in) as distance
from georef.edge ed
where st_dwithin(
		point_in,
		ed.the_geog,100)
AND st_expand(ed.the_geog::geometry, 100) && point_in::geometry

order by distance
limit 1 )tmp
);

END
$func$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION georef.update_house_number() RETURNS VOID AS $$
UPDATE georef.house_number set way_id=georef.nearest_way_id(coord);
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION navitia.update_admin_coord() RETURNS VOID AS $$
UPDATE navitia.admin set coord = st_centroid(boundary::geometry)
where st_X(coord::geometry)=0
and st_y(coord::geometry)=0
$$
LANGUAGE SQL;

-- Pour chaque rue trouve les zones administratives qui le contiennent
CREATE OR REPLACE FUNCTION georef.match_way_to_admin() RETURNS VOID AS $$
INSERT INTO georef.rel_way_admin(way_id, admin_id)
SELECT DISTINCT way.id, ad.id
FROM
georef.way way,
navitia.admin ad,
    georef.edge edge
WHERE
    edge.way_id = way.id
AND st_within(edge.the_geog::geometry, ad.boundary::geometry)
AND ad.boundary && edge.the_geog -- On force l’utilisation des indexes spatiaux en comparant les bounding boxes
$$
LANGUAGE SQL;

CREATE OR REPLACE FUNCTION navitia.match_admin_to_admin() RETURNS VOID AS $$
INSERT INTO navitia.rel_admin_admin(master_admin_id, admin_id)
select distinct ad1.id, ad2.id
from navitia.admin ad1, navitia.admin ad2
WHERE st_within(ad2.coord::geometry, ad1.boundary::geometry)
AND ad2.coord && ad1.boundary -- On force l’utilisation des indexes spatiaux en comparant les bounding boxes
and ad1.id <> ad2.id
and ad2.level > ad1.level
$$
LANGUAGE SQL;

-- fonction permettant de mettre à jour les boundary des admins
CREATE OR REPLACE FUNCTION georef.update_boundary(adminid bigint)
  RETURNS VOID AS
$$
DECLARE 
	ret GEOGRAPHY;
BEGIN
	SELECT ST_Multi(ST_ConvexHull(ST_Collect(
		ARRAY(
				select aa.coord::geometry from (
					select n.coord as coord from georef.rel_way_admin rel,
						georef.edge e, georef.node n
					where rel.admin_id=adminid
					and rel.way_id=e.way_id
					and e.source_node_id=n.id
					UNION
					select n.coord as coord from georef.rel_way_admin rel,
						georef.edge e, georef.node n
					where rel.admin_id=adminid
					and rel.way_id=e.way_id
					and e.target_node_id=n.id)aa)))) INTO ret;
	CASE  geometrytype(ret::geometry) 
		WHEN 'MULTIPOLYGON'  THEN
			UPDATE navitia.admin 
			set boundary = ret::geometry
			where navitia.admin.id=adminid;
		WHEN 'MultiLineString'  THEN
			UPDATE navitia.admin 
			set boundary = ST_Multi(ST_Polygonize(ret::geometry))
			where navitia.admin.id=adminid;
		ELSE 
			UPDATE navitia.admin 
			set boundary = NULL
			where navitia.admin.id=adminid;
	END CASE;
END
$$
LANGUAGE plpgsql;

--- FUSION WAYS
CREATE OR REPLACE FUNCTION georef.admin_way() RETURNS VOID AS $$
delete from georef.admin_way;
INSERT INTO georef.admin_way SELECT distinct on(v.admin_insee, v.way_name) v.way_name, v.admin_insee, v.way_id
FROM georef.valid_ways_admins as v;
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION georef.fusion_way() RETURNS VOID AS $$
delete from georef.fusion_ways;
INSERT INTO georef.fusion_ways SELECT DISTINCT on(to_clean.way_id) to_clean.way_id, admin_way.way_id 
from (select v.way_id, v.way_name, v.admin_insee 
      from georef.valid_ways_admins v where v.way_id not in 
        (select way_id from georef.admin_way)) to_clean
join georef.admin_way on (admin_way.admin_insee, admin_way.way_name) = (to_clean.admin_insee, to_clean.way_name)
group by to_clean.way_id, admin_way.way_id
;
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION georef.update_edge() RETURNS VOID AS $$
update georef.edge
set way_id=georef.fusion_ways.way_id from georef.fusion_ways
where georef.edge.way_id = georef.fusion_ways.original_way_id;
UPDATE georef.edge SET way_id = georef.fusion_ways.way_id 
from georef.fusion_ways 
where georef.edge.way_id = georef.fusion_ways.way_id;
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION georef.update_rel_way_admin() RETURNS VOID AS $$
delete from georef.rel_way_admin 
where way_id in( select distinct(original_way_id) FROM georef.fusion_ways)
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION georef.update_way() RETURNS VOID AS $$
DELETE from georef.way 
WHERE id IN (select distinct(original_way_id) FROM georef.fusion_ways);
$$
LANGUAGE SQL;

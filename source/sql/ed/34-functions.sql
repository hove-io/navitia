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


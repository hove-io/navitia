-- Pour chaque stop area trouve les zones administratives qui le contiennent
CREATE OR REPLACE FUNCTION georef.match_stop_area_to_admin() RETURNS VOID AS $$
INSERT INTO navitia.rel_stop_area_admin(stop_area_id, admin_id)
SELECT sa.id, ad.id
FROM
	navitia.stop_area sa,
	navitia.admin ad
WHERE
	st_within(sa.coord::geometry, ad.boundary::geometry)
	AND ad.boundary && sa.coord -- On force l’utilisation des indexes spatiaux en comparant les bounding boxes
$$
LANGUAGE SQL;

-- Pour chaque stop point trouve les zones administratives qui le contiennent
CREATE OR REPLACE FUNCTION georef.match_stop_point_to_admin() RETURNS VOID AS $$
INSERT INTO navitia.rel_stop_point_admin(stop_point_id, admin_id)
SELECT sp.id, ad.id
FROM
	navitia.stop_point sp,
	navitia.admin ad
WHERE
	st_within(sp.coord::geometry, ad.boundary::geometry)
	AND ad.boundary && sp.coord -- On force l’utilisation des indexes spatiaux en comparant les bounding boxes
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

-- Pour chaque poi trouve les zones administratives qui le contiennent
CREATE OR REPLACE FUNCTION georef.match_poi_to_admin() RETURNS VOID AS $$
INSERT INTO navitia.rel_poi_admin(poi_id, admin_id)
SELECT DISTINCT poi.id, ad.id
FROM
	navitia.poi poi,
	navitia.admin ad
WHERE st_within(poi.coord::geometry, ad.boundary::geometry)
	AND poi.coord && ad.boundary -- On force l’utilisation des indexes spatiaux en comparant les bounding boxes
$$
LANGUAGE SQL;

-- Chargement de la table georef.fusion_ways
CREATE OR REPLACE FUNCTION georef.fusion_ways_by_admin_name() RETURNS VOID AS $$
delete from georef.fusion_ways; -- Netoyage de la table
insert into georef.fusion_ways(id, way_id)
select test2.id, test1.way_id from
(
SELECT a.id as admin_id, w.name as way_name, w.id as way_id
  FROM georef.way w, georef.rel_way_admin r, navitia.admin a
  where w.id=r.way_id
  and r.admin_id=a.id
  and a.level=8 -- On choisit que les communes
  group by w.name, a.id, w.id
  order by a.id, w.name, w.id) test1
  inner join (
  SELECT a.id as admin_id, w.name as way_name, min(w.id) as id
  FROM georef.way w, georef.rel_way_admin r, navitia.admin a
  where w.id=r.way_id
  and r.admin_id=a.id
  and a.level=8
  group by a.id, w.name
  order by a.id, w.name)test2
  on(test1.admin_id = test2.admin_id and test1.way_name = test2.way_name)
$$
LANGUAGE SQL;

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

-- MAJ des way_id de la table georef.edge
CREATE OR REPLACE FUNCTION georef.update_edge() RETURNS VOID AS $$
update georef.edge ge
set way_id=fw.id
from georef.fusion_ways fw
where ge.way_id=fw.way_id
and fw.id <> fw.way_id
and ge.way_id <> fw.id

$$
LANGUAGE SQL;

-- MAJ de la table .. pour supprimer les voies plus tard
CREATE OR REPLACE FUNCTION georef.add_fusion_ways() RETURNS VOID AS $$
insert into georef.fusion_ways
select w.id, w.id from georef.way w
left outer join georef.rel_way_admin a
on (w.id = a.way_id)
where a.admin_id is NULL
$$
LANGUAGE SQL;
-- Ajout des voies qui ne sont pas dans la table de fusion : cas voie appartient à un seul admin avec un level 9
CREATE OR REPLACE FUNCTION georef.complete_fusion_ways() RETURNS VOID AS $$
insert into georef.fusion_ways(id, way_id)
 select georef.way.id, georef.way.id from georef.way
where georef.way.id in ( select w.id from georef.way w
left outer join georef.fusion_ways fw
on(w.id=fw.way_id)
where fw.id IS NULL)
$$
LANGUAGE SQL;


CREATE OR REPLACE FUNCTION georef.add_way_name() RETURNS VOID AS $$
UPDATE georef.way
SET name = 'NC:' || CAST(id as varchar)
where name = ''
$$
LANGUAGE SQL;

CREATE OR REPLACE FUNCTION georef.clean_way() RETURNS VOID AS $$
delete from georef.way
where id in ( select w.id from georef.way w
left outer join georef.fusion_ways fw
on(w.id=fw.id)
where fw.id IS NULL)
$$
LANGUAGE SQL;

CREATE OR REPLACE FUNCTION georef.clean_way_name() RETURNS VOID AS $$
update georef.way set name='' where name like 'NC:%'
$$
LANGUAGE SQL;

CREATE OR REPLACE FUNCTION georef.insert_tmp_rel_way_admin() RETURNS VOID AS $$
delete from georef.tmp_rel_way_admin;
insert into georef.tmp_rel_way_admin select distinct rwa.admin_id, fw.id from georef.rel_way_admin rwa, georef.fusion_ways fw
where rwa.way_id=fw.way_id
$$
LANGUAGE SQL;

CREATE OR REPLACE FUNCTION georef.insert_rel_way_admin() RETURNS VOID AS $$
delete from georef.rel_way_admin;
insert into georef.rel_way_admin select * from georef.tmp_rel_way_admin
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

 -- Conversion des coordonnées vers wgs84
CREATE OR REPLACE FUNCTION georef.coord2wgs84(lon real, lat real, coord_in int)
  RETURNS RECORD AS
$$
DECLARE
  ret RECORD;
BEGIN
	SELECT ST_X(aa.new_coord::geometry) as lon, ST_Y(aa.new_coord::geometry) as lat from (
	SELECT ST_Transform(ST_GeomFromText('POINT('||lon||' '||lat||')',coord_in),4326) As new_coord)aa INTO ret;
	RETURN ret;
END
$$
LANGUAGE plpgsql;

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
	CASE  geometrytype(ret)
		WHEN 'MULTIPOLYGON'  THEN
			UPDATE navitia.admin
			set boundary = ret
			where navitia.admin.id=adminid;
		WHEN 'MultiLineString'  THEN
			UPDATE navitia.admin
			set boundary = ST_Multi(ST_Polygonize(ret))
			where navitia.admin.id=adminid;
		ELSE
			UPDATE navitia.admin
			set boundary = NULL
			where navitia.admin.id=adminid;
	END CASE;
END
$$
LANGUAGE plpgsql;


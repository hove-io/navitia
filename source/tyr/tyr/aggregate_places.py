from tyr import celery, db, app
from sqlalchemy import Table, MetaData, create_engine, select, not_
import sqlalchemy
from navitiacommon import models
from sqlalchemy.orm import sessionmaker, load_only
from geoalchemy2.types import Geography
from geoalchemy2.shape import to_shape
from sqlalchemy.ext.declarative import declarative_base
import logging
from flask.ext.script import Command, Option
from tyr.helper import load_instance_config

def make_sqlalchemy_connection_string(instance_config):
    connection_string = "postgresql://" + instance_config.pg_username
    connection_string += ":" + instance_config.pg_password
    connection_string += "@" + instance_config.pg_host
    connection_string += "/" + instance_config.pg_dbname
    return connection_string


@celery.task()
def aggregate_places(instance_config, job_id):
    c_string_ed = make_sqlalchemy_connection_string(instance_config)
    engine_ed = create_engine(c_string_ed)
    meta_ed = MetaData(engine_ed)
    meta_jormungandr = MetaData(bind=db.engine)

    logger = logging.getLogger(__name__)

    instance_ = models.Instance.query.\
        filter(models.Instance.name == instance_config.name)
    if instance_.first():
        instance = instance_.first()
    else:
        logger.error("Instance {0} can not be found".format(
                instance_config.name))
        return

    table_uris = Table("uris", meta_jormungandr,
                        db.Column("id", db.Integer),
                        db.Column("uri", db.String),
                        extend_existing=True,
                        prefixes=['TEMPORARY'])

    try:
        Session_ed = sessionmaker(bind=engine_ed)
        session_ed = Session_ed()
        Base_ed = declarative_base(metadata=meta_ed)
        #We now insert/update places
        def handle_object_instance(type_name, TypeEd):
            logger.info("Begin to handle {0}".format(type_name))
            cls_type = models.get_class_type(type_name)
            rel_instance = cls_type.cls_rel_instance
            has_coord = "coord" in dir(cls_type)
            has_external_code = "external_code" in TypeEd.__table__.c
            #We delete all relations object<->instance
            rel_instance.query.filter_by(instance_id=instance.id).delete()
            db.session.commit()
            #We create a temporary table to stode uris and ids of ed type
            if not table_uris.exists():
                table_uris.create(db.engine, checkfirst=True)
            if db.session.execute(table_uris.delete()): #make sure it's empty
                db.session.commit()

            query_ed = session_ed.query(TypeEd)
            objects_ed = query_ed.all()
            #We insert uris in a temp table to be able to work on it after
            db.session.execute(table_uris.insert(),
                            [{"id" : a.id, "uri" : a.uri} for a in objects_ed])
            #We update the object that are in ed and and tyr
            to_update_query = table_uris.select(
                table_uris.c.uri.in_(cls_type.query.options(load_only(cls_type.uri))))
            for object_ in db.session.execute(to_update_query):
                object_ed = query_ed.get(object_[0])
                to_update = {
                    "external_code" : "" if not has_external_code\
                                        else object_ed.external_code,
                    "name" : object_ed.name,
                }
                if has_coord:
                    to_update["coord"] = object_ed.coord
                cls_type.query.filter_by(uri=object_ed.uri).\
                    update(to_update, synchronize_session=False)
            #We insert links between update objects and instance
            insert_rel_query = cls_type.query.filter(cls_type.uri.in_(
                select([table_uris.c.uri]))).options(load_only(cls_type.id))
            object_instances = []
            for object_ in insert_rel_query.all():
                object_instances.append(rel_instance(object_.id, instance.id))
            db.session.add_all(object_instances)
            db.session.commit()
            #We insert the objects that are in ed but not in jormungandr
            to_insert_query = table_uris.select(
                not_(table_uris.c.uri.in_(cls_type.query.options(load_only(cls_type.uri)))))
            for object_ in db.session.execute(to_insert_query):
                if object_[0] is None:
                    continue
                object_ed = query_ed.get(object_[0])
                new_object = cls_type()
                new_object.uri = object_ed.uri
                new_object.name = object_ed.name
                new_object.external_code = "" if not has_external_code\
                                            else object_ed.external_code
                if has_coord:
                    new_object.coord = to_shape(object_ed.coord).wkt
                new_object.instances.append(instance)
                db.session.add(new_object)
            ##We delete object that don't have  anymore instance
            db.session.query(cls_type).filter(not_(cls_type.instances.any()))\
                .delete(synchronize_session=False)
            db.session.commit()
            table_uris.drop()
            logger.info("Finished to handle {0}".format(type_name))

        #Unable to factor the class, sqlalchemy handles a dictionnary of
        #the name of existing classes, I don't know how to access this dictionnary
        class StopAreaEd(Base_ed):
            __table__ = Table("stop_area", meta_ed,
                         autoload=True, autoload_with=engine_ed)
        handle_object_instance("stop_area", StopAreaEd)

        class StopPointEd(Base_ed):
            __table__ = Table("stop_point", meta_ed,
                        autoload=True, autoload_with=engine_ed)
        handle_object_instance("stop_point", StopPointEd)

        class PoiEd(Base_ed):
            __table__ = Table("poi", meta_ed,
                        autoload=True, autoload_with=engine_ed)
        handle_object_instance("poi", PoiEd)

        class AdminEd(Base_ed):
            __table__ = Table("admin", meta_ed,
                         autoload=True, autoload_with=engine_ed)
        handle_object_instance("admin", AdminEd)

        class LineEd(Base_ed):
            __table__ = Table("line", meta_ed,
                         autoload=True, autoload_with=engine_ed)
        handle_object_instance("line", LineEd)

        class RouteEd(Base_ed):
            __table__ = Table("route", meta_ed,
                        autoload=True, autoload_with=engine_ed)
        handle_object_instance("route", RouteEd)

        class NetworkEd(Base_ed):
            __table__ = Table("network", meta_ed,
                        autoload=True, autoload_with=engine_ed)
        handle_object_instance("network", NetworkEd)
    finally:
        session_ed.close()


class AggregatePlacesCommand(Command):

    def get_options(self):
        return [
            Option('-n', '--name', dest='name_', default="",
                   help="Launch aggregate places, if no name is specified "
                   "this is launched for all instances")
        ]

    def run(self, name_=""):
        if name_:
            instances = models.Instance.query.filter_by(name=name_).all()
        else:
            instances = models.Instance.query.all()

        job_id = 1
        instances_name = [instance.name for instance in instances]
        for instance_name in instances_name:
            instance_config = None
            try:
                instance_config = load_instance_config(instance_name)
            except ValueError:
                logging.getLogger(__name__).\
                    info("Unable to find instance " + instance_name)
                continue
            aggregate_places(instance_config, job_id)
            job_id += 1


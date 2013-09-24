#encoding: utf-8
from sqlalchemy import select
from sindri.saver.utils import parse_active_days, from_timestamp, from_time, \
        FunctionalError

def build_at_perturbation_dict(perturbation):
    """
    construit à partir d'un object protobuf pbnavitia.realtime.AtPerturbation
    le dictionnaire utilisé pour l'insertion en base
    """
    result = {}
    result['uri'] = perturbation.uri
    result['object_uri'] = perturbation.object.object_uri
    result['object_type_id'] = perturbation.object.object_type

    result['active_days'] = parse_active_days(perturbation.active_days)

    result['start_application_date'] = from_timestamp(
            perturbation.start_application_date)
    result['end_application_date'] = from_timestamp(
            perturbation.end_application_date)

    result['start_application_daily_hour'] = from_time(
            perturbation.start_application_daily_hour)
    result['end_application_daily_hour'] = from_time(
            perturbation.end_application_daily_hour)

    return result


def persist_at_perturbation(meta, conn, perturbation):
    """
    enregistre en base la perturbation at
    """
    row_id = find_at_perturbation_id(meta, conn, perturbation.uri)
    save_at_perturbation(meta, conn, row_id, perturbation)


def find_at_perturbation_id(meta, conn, perturbation_uri):
    """
    retourne l'id en base de la perturbation correspondant à cette URI
    si celle ci est présente dans ED sinon retourne None
    """
    perturbation_table = meta.tables['realtime.at_perturbation']
    query = select([perturbation_table.c.id],
            perturbation_table.c.uri == perturbation_uri)
    result = conn.execute(query)
    row = result.fetchone()
    return row[0] if row else None

def save_at_perturbation(meta, conn, perturbation_id, perturbation):
    """
    update ou insert la perturbation
    """
    perturbation_table = meta.tables['realtime.at_perturbation']
    if not perturbation_id:
        query = perturbation_table.insert()
    else:
        query = perturbation_table.update().where(
                perturbation_table.c.id == perturbation_id)

    conn.execute(query.values(build_at_perturbation_dict(
        perturbation)))

from flask.ext.restful import Resource, fields, marshal_with
from jormungandr import i_manager
from jormungandr.protobuf_to_dict import protobuf_to_dict

status = {
    "status": fields.Nested( {
        "data_version": fields.Integer(),
        "end_production_date": fields.String(),
        "is_connected_to_rabbitmq": fields.Boolean(),
        "last_load_at": fields.String(),
        "last_load_status": fields.Boolean(),
        "kraken_version": fields.String(attribute="navitia_version"),
        "nb_threads": fields.Integer(),
        "publication_date": fields.String(),
        "start_production_date": fields.String()
    })
}
class Status(Resource):
    @marshal_with(status)
    def get(self, region):
        response = i_manager.dispatch([], "status", instance_name=region)
        return protobuf_to_dict(response, use_enum_labels=True), 200

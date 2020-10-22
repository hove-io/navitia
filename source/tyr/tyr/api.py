#!/usr/bin/env python
# coding: utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from tyr import resources, app, api, ridesharing_service
import flask_restful

# we always want pretty json
flask_restful.representations.json.settings = {'indent': 4}
api.app.url_map.strict_slashes = False

api.add_resource(
    resources.Instance,
    '/v<int:version>/instances/',
    '/v<int:version>/instances/<int:id>/',
    '/v<int:version>/instances/<string:name>/',
)

api.add_resource(resources.Api, '/v<int:version>/api/')

api.add_resource(resources.User, '/v0/users/', '/v0/users/<int:user_id>/')
api.add_resource(resources.UserV1, '/v1/users/', '/v1/users/<int:user_id>/')

api.add_resource(
    resources.Key,
    '/v<int:version>/users/<int:user_id>/keys/',
    '/v<int:version>/users/<int:user_id>/keys/<int:key_id>/',
)

api.add_resource(resources.Authorization, '/v<int:version>/users/<int:user_id>/authorizations/')

api.add_resource(resources.Index, '/')

api.add_resource(resources.Status, '/v0/status')

api.add_resource(
    resources.Job, '/v0/jobs/', '/v0/jobs/<string:instance_name>/', '/v0/jobs/<int:id>/', endpoint=str('jobs')
)

api.add_resource(
    resources.EndPoint,
    '/v<int:version>/end_points/',
    '/v<int:version>/end_points/<int:id>/',
    endpoint=str('end_points'),
)

api.add_resource(
    resources.TravelerProfile,
    '/v0/instances/<string:name>/traveler_profiles/',
    '/v0/instances/<string:name>/traveler_profiles/<string:traveler_type>',
)

api.add_resource(
    resources.BillingPlan,
    '/v<int:version>/billing_plans/',
    '/v<int:version>/billing_plans/<int:billing_plan_id>',
)

api.add_resource(resources.InstancePoiType, '/v0/instances/<string:instance_name>/poi_types')

api.add_resource(
    resources.AutocompleteParameter,
    '/v<int:version>/autocomplete_parameters/',
    '/v<int:version>/autocomplete_parameters/<string:name>',
)

api.add_resource(resources.AutocompletePoiType, '/v0/autocomplete_parameters/<string:name>/poi_types')

api.add_resource(resources.InstanceDataset, '/v0/instances/<instance_name>/last_datasets')

api.add_resource(resources.AutocompleteDataset, '/v0/autocomplete_parameters/<ac_instance_name>/last_datasets')

api.add_resource(resources.AutocompleteUpdateData, '/v0/autocomplete_parameters/<ac_instance_name>/update_data')

api.add_resource(
    resources.MigrateFromPoiToOsm, '/v0/instances/<string:instance_name>/actions/migrate_from_poi_to_osm'
)

api.add_resource(
    resources.DeleteDataset, '/v0/instances/<string:instance_name>/actions/delete_dataset/<string:type>'
)

api.add_resource(resources.BssProvider, '/v0/bss_providers', '/v0/bss_providers/<string:id>')

api.add_resource(
    resources.EquipmentsProvider,
    '/v0/equipments_providers',
    '/v0/equipments_providers/<string:id>',
    '/v1/equipments_providers',
    '/v1/equipments_providers/<string:id>',
)

api.add_resource(
    ridesharing_service.RidesharingService,
    '/v<int:version>/ridesharing_services',
    '/v<int:version>/ridesharing_services/<string:id>',
)

api.add_resource(
    resources.StreetNetworkBackend,
    '/v0/streetnetwork_backends',
    '/v0/streetnetwork_backends/<string:backend_id>',
    '/v1/streetnetwork_backends',
    '/v1/streetnetwork_backends/<string:backend_id>',
    endpoint=str('streetnetwork_backends'),
)

# TODO: Find a way to handle GET on an URL and POST to another in one class
api.add_resource(resources.CitiesStatus, '/v0/cities/status')
api.add_resource(resources.Cities, '/v0/cities/')


@app.errorhandler(Exception)
def error_handler(exception):
    """
    log exception
    """
    app.logger.exception('')

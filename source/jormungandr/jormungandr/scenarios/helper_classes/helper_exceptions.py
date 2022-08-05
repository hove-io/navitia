# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
from __future__ import absolute_import
import copy
from navitiacommon import response_pb2


class PtException(Exception):
    def __init__(self, pt_journey_with_error):
        super(PtException, self).__init__()
        self._response = copy.deepcopy(pt_journey_with_error)

    def get(self):
        return self._response


class NoGraphicalIsochroneFoundException(PtException):
    def __init__(self):
        self._response = _make_error_response(
            error_id=response_pb2.Error.no_solution,
            message="No public transit journey found in graphical isochrone computation.",
        )


class InvalidDateBoundException(PtException):
    def __init__(self, pt_journey_with_error):
        super(PtException, self).__init__()
        self._response = copy.deepcopy(pt_journey_with_error)


def _make_error_response(message, error_id):

    r = response_pb2.Response()
    r.error.message = message
    r.error.id = error_id
    return r


class EntryPointException(Exception):
    def __init__(self, error_id, error_message):
        super(EntryPointException, self).__init__()
        self._response = _make_error_response(error_id=error_id, message=error_message)

    def get(self):
        return self._response


class StreetNetworkException(Exception):
    def __init__(self, error_id, error_message):
        super(StreetNetworkException, self).__init__()
        self._response = _make_error_response(error_id=error_id, message=error_message)

    def get(self):
        return self._response


class FinaliseException(EntryPointException):
    def __init__(self, exception):
        super(FinaliseException, self).__init__(
            error_id=response_pb2.Error.internal_error, error_message=str(exception)
        )

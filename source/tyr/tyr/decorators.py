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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io


from functools import wraps
from flask import request
from flask_restful import marshal
from navitiacommon import models
from tyr.helper import get_instance_logger, rm_tempfile, save_in_tmp
from tyr.fields import one_job_fields, error_fields


class update_data(object):
    def __init__(self, is_autocomplete=False):
        self.is_autocomplete = is_autocomplete

    def __call__(self, func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if self.is_autocomplete:
                instance = models.AutocompleteParameter.query.\
                    filter_by(name=kwargs.get('ac_instance_name')).first_or_404()
            else:
                instance = models.Instance.query_existing().\
                    filter_by(name=kwargs.get('instance_name')).first_or_404()

            if not request.files:
                return marshal({'error': {'message': 'the Data file is missing'}}, error_fields), 404
            content = request.files['file']
            logger = get_instance_logger(instance)
            logger.info('content received: %s', content.filename)
            filename = save_in_tmp(content)
            feature, job = func(*args, files=[filename], instance=instance, **kwargs)
            job = models.db.session.merge(job) #reatache the object
            rm_tempfile(filename)
            return marshal({'job': job}, one_job_fields), 200
        return wrapper

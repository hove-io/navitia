# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
# powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
# a non ending quest to the responsive locomotion way of traveling!
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
import json
from flask import render_template, abort, make_response, url_for
from jinja2 import TemplateNotFound
from jormungandr.modules_loader import ABlueprint


class TisseoRouting(ABlueprint):
    def __init__(self, api, name):
        super(TisseoRouting, self).__init__(api,
                                            name,
                                            __name__,
                                            description='Test module',
                                            status='testing',
                                            index_endpoint='index_json',
                                            static_folder='static',
                                            template_folder='templates')

    def setup(self):
        self.add_url_rule('/', defaults={'page': 'index'},
                          view_func=self.page)
        self.add_url_rule('/<page>', view_func=self.page)
        self.add_url_rule('/index.json', view_func=self.index_json)

    def page(self, page):
        try:
            return render_template('%s.html' % page)
        except TemplateNotFound:
            abort(404)

    def index_json(self):
        return make_response(json.dumps({'Raw': 'json', 'link': url_for(self.name + '.page')},
                                        sort_keys=True))

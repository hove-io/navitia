#!/usr/bin/env python
# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from tyr import app, db, manager
from flask_migrate import Migrate, MigrateCommand
from tyr.command import ReloadKrakenCommand, BuildDataCommand, LoadDataCommand


migrate = Migrate(app, db)
manager.add_command('db', MigrateCommand)
manager.add_command('reload_kraken', ReloadKrakenCommand())
manager.add_command('build_data', BuildDataCommand())
manager.add_command('load_data', LoadDataCommand())

if __name__ == '__main__':
    app.config['DEBUG'] = True
    manager.run()

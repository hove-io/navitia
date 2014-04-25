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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from tyr import app, db
import sys
from flask_script import Manager
from flask_migrate import Migrate, MigrateCommand
from tyr.command import AggregatePlacesCommand, ReloadAtCommand, AtReloader,\
    ReloadKrakenCommand, BuildDataCommand

manager = Manager(app)

migrate = Migrate(app, db)
manager.add_command('db', MigrateCommand)
#A command used in development environment to run aggregate places without
#having to run tyr
manager.add_command('aggregate_places', AggregatePlacesCommand())
manager.add_command('reload_at', ReloadAtCommand())
manager.add_command('at_reloader', AtReloader())
manager.add_command('reload_kraken', ReloadKrakenCommand())
manager.add_command('build_data', BuildDataCommand())

if __name__ == '__main__':
    app.config['DEBUG'] = True
    manager.run()

#!/usr/bin/env python
# coding=utf-8
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

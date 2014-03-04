#!/usr/bin/env python
# coding=utf-8
from tyr import app, db
import sys
from flask_script import Manager
from flask_migrate import Migrate, MigrateCommand
from tyr.aggregate_places import AggregatePlacesCommand

manager = Manager(app)

migrate = Migrate(app, db)
manager.add_command('db', MigrateCommand)
manager.add_command('aggregate_places', AggregatePlacesCommand())

if __name__ == '__main__':
    app.config['DEBUG'] = True
    manager.run()

#!/usr/bin/env python
# coding=utf-8
from tyr import app
import sys
from flask_script import Manager


manager = Manager(app)

if __name__ == '__main__':
    app.config['DEBUG'] = True
    manager.run()

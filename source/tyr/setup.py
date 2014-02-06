#!/usr/bin/env python

from distutils.core import setup
import glob

setup(name='tyr',
        version='0.39.0',
        description="Provide an API for managing users and handle data updates",
        author='CanalTP',
        author_email='alexandre.jacquin@canaltp.fr',
        url='www.navitia.io',
        packages=['tyr'],
        requires=['configobj'],
        scripts=['manage_tyr.py'],
        data_files=[
            ('/usr/share/tyr/migrations', ['migrations/alembic.ini', 'migrations/env.py', 'migrations/script.py.mako']),
            ('/usr/share/tyr/migrations/versions', glob.glob('migrations/versions/*.py')),
            ('/etc/init.d', ['tyr_beat', 'tyr_worker'])
        ],
)

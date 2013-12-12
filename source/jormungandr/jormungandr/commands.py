from flask_script import Command
from jormungandr import i_manager
from jormungandr.db import syncdb


class CreateDb(Command):

    def run(self):
        i_manager.stop()
        syncdb()

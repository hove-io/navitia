from flask.ext.script import Command, Option
from tyr.tasks import build_all_data
import logging

class BuildDataCommand(Command):
    """A command used to build all the datasets
    """

    def run(self):
        logging.info("Run build data")
        build_all_data()

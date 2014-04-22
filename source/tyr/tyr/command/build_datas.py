from flask.ext.script import Command, Option
from tyr.tasks import build_all_datas
import logging

class BuildDatasCommand(Command):
    """A command used to build all the datas
    """

    def run(self):
        logging.info("Run build datas")
        build_all_datas()
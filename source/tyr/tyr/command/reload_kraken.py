from flask.ext.script import Command, Option
from navitiacommon import models
from tyr.tasks import reload_kraken
import logging

class ReloadKrakenCommand(Command):
    """A command used for run trigger a reload of  an instance
    """

    def get_options(self):
        return [
            Option(dest='instance_name', help="name of the instance to reload")
        ]

    def run(self, instance_name):
        logging.info("Run command reload kraken")
        instance = models.Instance.query.filter_by(name=instance_name).first()
        reload_kraken.delay(instance.id)

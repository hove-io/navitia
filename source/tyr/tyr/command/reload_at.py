from flask.ext.script import Command, Option
from navitiacommon import models
from tyr.tasks import reload_at
import logging

class ReloadAtCommand(Command):
    """A command used for run trigger a reload of realtime information
    for an instance
    """

    def get_options(self):
        return [
            Option(dest='instance_name', help="name of the instance to reload")
        ]

    def run(self, instance_name):
        instance = models.Instance.query.filter_by(name=instance_name).first()
        reload_at.delay(instance.id)

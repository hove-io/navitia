from flask.ext.script import Command, Option
from navitiacommon import models
from tyr.helper import load_instance_config
from tyr.aggregate_places import aggregate_places
import logging

class AggregatePlacesCommand(Command):
    """A command used in development environment to run aggregate places without
    having to run tyr
    """

    def get_options(self):
        return [
            Option('-n', '--name', dest='name_', default="",
                   help="Launch aggregate places, if no name is specified "
                   "this is launched for all instances")
        ]

    def run(self, name_=""):
        if name_:
            instances = models.Instance.query.filter_by(name=name_).all()
        else:
            instances = models.Instance.query.all()

        if not instances:
            logging.getLogger(__name__).\
                error("Unable to find any instance for name '{name}'"
                      .format(name=name_))
            return

        #TODO: create a real job
        job_id = 1
        instances_name = [instance.name for instance in instances]
        for instance_name in instances_name:
            instance_config = None
            try:
                instance_config = load_instance_config(instance_name)
            except ValueError:
                logging.getLogger(__name__).\
                    info("Unable to find instance " + instance_name)
                continue
            aggregate_places(instance_config, job_id)
            job_id += 1



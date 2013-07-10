"""main file

read the configuration
initialize the loggers
run the watching
"""

from pyed.config import Config
from pyed.watching import Watching
import logging

if __name__ == '__main__':
    CONFIG = Config("/home/vlara/workspace/pyed/pyed.ini")
    if not CONFIG.is_valid():
        print "Configuration non valide"
        SystemExit(1)

    FORMATTER = logging.Formatter('%(asctime)s -  %(levelname)s - %(message)s')

    ROOT_HANDLER = logging.FileHandler(CONFIG.get("logs_files", "pyed"))
    ROOT_HANDLER.setFormatter(FORMATTER)
    ROOT_LOGGER = logging.getLogger('root')
    ROOT_LOGGER.setLevel(logging.INFO)
    ROOT_LOGGER.addHandler(ROOT_HANDLER)

    PYED_HANDLER = logging.FileHandler(CONFIG.get("logs_files", "pyed"))
    PYED_HANDLER.setFormatter(FORMATTER)
    PYED_LOGGER = logging.getLogger('pyed')
    PYED_LOGGER.setLevel(logging.INFO)
    PYED_LOGGER.addHandler(PYED_HANDLER)

    GTFS2ED_HANDLER = logging.FileHandler(CONFIG.get("logs_files", "gtfs2ed"))
    GTFS2ED_HANDLER.setFormatter(FORMATTER)
    GTFS2ED_LOGGER = logging.getLogger('gtfs2ed')
    GTFS2ED_LOGGER.setLevel(logging.INFO)
    GTFS2ED_LOGGER.addHandler(GTFS2ED_HANDLER)

    OSM2ED_HANDLER = logging.FileHandler(CONFIG.get("logs_files", "osm2ed"))
    OSM2ED_HANDLER.setFormatter(FORMATTER)
    OSM2ED_LOGGER = logging.getLogger('osm2ed')
    OSM2ED_LOGGER.setLevel(logging.INFO)
    OSM2ED_LOGGER.addHandler(OSM2ED_HANDLER)


    ED2NAV_HANDLER = logging.FileHandler(CONFIG.get("logs_files", "ed2nav"))
    ED2NAV_HANDLER.setFormatter(FORMATTER)
    ED2NAV_LOGGER = logging.getLogger('ed2nav')
    ED2NAV_LOGGER.setLevel(logging.INFO)
    ED2NAV_LOGGER.addHandler(ED2NAV_HANDLER)

    PYED_LOGGER.info(" Initialization of watching ...") 
    WATCHING = Watching(CONFIG)
    PYED_LOGGER.info(" watching initialized") 
    PYED_LOGGER.info(" Launching watching") 
    WATCHING.run()
    PYED_LOGGER.info(" watching launched") 

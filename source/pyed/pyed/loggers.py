"""
All functions related to the loggers:
    now init_loggers
"""

import logging

def init_loggers(config):
    """ Initialize all the loggers,
        It will be great to be able to customize the level of logging
    """
    formatter = logging.Formatter('%(asctime)s -  %(levelname)s - %(message)s')

    root_handler = logging.FileHandler(config.get("logs_files", "pyed"))
    root_handler.setFormatter(formatter)
    root_logger = logging.getLogger('root')
    root_logger.setLevel(logging.INFO)
    root_logger.addHandler(root_handler)

    pyed_handler = logging.FileHandler(config.get("logs_files", "pyed"))
    pyed_handler.setFormatter(formatter)
    pyed_logger = logging.getLogger('pyed')
    pyed_logger.setLevel(logging.INFO)
    pyed_logger.addHandler(pyed_handler)

    gtfs2ed_handler = logging.FileHandler(config.get("logs_files", "gtfs2ed"))
    gtfs2ed_handler.setFormatter(formatter)
    gtfs2ed_logger = logging.getLogger('gtfs2ed')
    gtfs2ed_logger.setLevel(logging.INFO)
    gtfs2ed_logger.addHandler(gtfs2ed_handler)

    OSM2ed_handler = logging.FileHandler(config.get("logs_files", "osm2ed"))
    OSM2ed_handler.setFormatter(formatter)
    OSM2ed_logger = logging.getLogger('osm2ed')
    OSM2ed_logger.setLevel(logging.INFO)
    OSM2ed_logger.addHandler(OSM2ed_handler)


    ed2nav_handler = logging.FileHandler(config.get("logs_files", "ed2nav"))
    ed2nav_handler.setFormatter(formatter)
    ed2nav_logger = logging.getLogger('ed2nav')
    ed2nav_logger.setLevel(logging.INFO)
    ed2nav_logger.addHandler(ed2nav_handler)

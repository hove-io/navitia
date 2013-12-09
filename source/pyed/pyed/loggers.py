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

    root_handler = logging.FileHandler(config.get("log_files", "pyed"))
    root_handler.setFormatter(formatter)
    root_logger = logging.getLogger('root')
    root_logger.setLevel(logging.INFO)
    root_logger.addHandler(root_handler)

    pyed_handler = logging.FileHandler(config.get("log_files", "pyed"))
    pyed_handler.setFormatter(formatter)
    pyed_logger = logging.getLogger('pyed')
    pyed_logger.setLevel(logging.INFO)
    pyed_logger.addHandler(pyed_handler)

    fusio2ed_handler = logging.FileHandler(config.get("log_files", "fusio2ed"))
    fusio2ed_handler.setFormatter(formatter)
    fusio2ed_logger = logging.getLogger('fusio2ed')
    fusio2ed_logger.setLevel(logging.INFO)
    fusio2ed_logger.addHandler(fusio2ed_handler)

    OSM2ed_handler = logging.FileHandler(config.get("log_files", "osm2ed"))
    OSM2ed_handler.setFormatter(formatter)
    OSM2ed_logger = logging.getLogger('osm2ed')
    OSM2ed_logger.setLevel(logging.INFO)
    OSM2ed_logger.addHandler(OSM2ed_handler)

    ed2nav_handler = logging.FileHandler(config.get("log_files", "ed2nav"))
    ed2nav_handler.setFormatter(formatter)
    ed2nav_logger = logging.getLogger('ed2nav')
    ed2nav_logger.setLevel(logging.INFO)
    ed2nav_logger.addHandler(ed2nav_handler)

    nav2rt_handler = logging.FileHandler(config.get("log_files", "nav2rt"))
    nav2rt_handler.setFormatter(formatter)
    nav2rt_logger = logging.getLogger('nav2rt')
    nav2rt_logger.setLevel(logging.INFO)
    nav2rt_logger.addHandler(ed2nav_handler)

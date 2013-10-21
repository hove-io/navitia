#!/usr/bin/env python
"""main file

read the configuration
initialize the loggers
run the watching
"""

from pyed.config import Config
from pyed.watching import Watching
from pyed.loggers import init_loggers

import sys
import logging
import argparse

def main(config_filename):
    config = Config(config_filename)
    if not config.is_valid():
        logging.basicConfig(filename='/var/log/pyed.log',level=logging.ERROR)
        logging.error("Configuration non valide")
        SystemExit(2)

    init_loggers(config)
    pyed_logger = logging.getLogger('pyed')
    pyed_logger.info(" Initialization of watching ...")
    watching = Watching(config)
    pyed_logger.info(" watching initialized")

    pyed_logger.info(" Launching watching daemon")
    try:
        watching.run()
    except Exception:
        pyed_logger.exception("Fatal error, ")
    pyed_logger.info(" watching daemon launched")

if __name__ == '__main__':
    PARSER = argparse.ArgumentParser(description="Launcher of pyed")
    PARSER.add_argument('-c', '--config_file', type=str, required=True)
    CONFIG_FILENAME = ""
    STATUS = ""
    USERNAME = None
    PASSWORD = None
    try:
        ARGS = PARSER.parse_args()
        CONFIG_FILENAME = ARGS.config_file
    except argparse.ArgumentTypeError:
        logging.basicConfig(filename='/var/log/ed/pyed.log',level=logging.ERROR)
        logging.error("Bad usage, learn how to use me with pyed.py -h")
        sys.exit(1)

    main(CONFIG_FILENAME)


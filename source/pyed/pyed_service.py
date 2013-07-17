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

def main(config_filename, status, user_name, user_password):
    config = Config(config_filename)
    if not config.is_valid():
        logging.basicConfig(filename='/var/log/pyed.log',level=logging.ERROR)
        logging.error("Configuration non valide")
        SystemExit(2)

    init_loggers(config)
    pyed_logger = logging.getLogger('pyed')
    pyed_logger.info(" Initialization of watching ...")
    watching = Watching(config, user_name, user_password)
    pyed_logger.info(" watching initialized")

    if status == 'start':
        pyed_logger.info(" Launching watching daemon")
        watching.start()
        pyed_logger.info(" watching daemon launched")
    elif status == 'stop':
        pyed_logger.info(" Stopping pyed daemon")
        watching.stop()
        pyed_logger.info(" Pyed daemon stopped")
    elif status == 'restart':
        pyed_logger.info(" Restarting pyed daemon")
        pyed_logger.info(" Stopping pyed daemon")
        watching.stop()
        pyed_logger.info(" Pyed daemon stopped")
        pyed_logger.info(" Start pyed daemon")
        watching.start()
        pyed_logger.info(" Pyed daemon started")

if __name__ == '__main__':
    PARSER = argparse.ArgumentParser(description="Launcher of pyed")
    PARSER.add_argument('status',  choices=['start', 'stop', 'restart'])
    PARSER.add_argument('-c', '--config_file', type=str, required=True)
    PARSER.add_argument('-u', '--user_name', type=str, 
                        help="User name use to launch daemon")
    PARSER.add_argument('-p', '--user_password', type=str)
    CONFIG_FILENAME = ""
    STATUS = ""
    USERNAME = None
    PASSWORD = None
    try:
        ARGS = PARSER.parse_args()
        CONFIG_FILENAME = ARGS.config_file
        STATUS = ARGS.status
        USERNAME = None if not ARGS.user_name else ARGS.user_name
        PASSWORD = None if not ARGS.user_password else ARGS.user_password
    except argparse.ArgumentTypeError:
        logging.basicConfig(filename='/var/log/pyed.log',level=logging.ERROR)
        logging.error("Bad usage, learn how to use me with pyed.py -h")
        sys.exit(1)

    main(CONFIG_FILENAME, STATUS, USERNAME, PASSWORD)


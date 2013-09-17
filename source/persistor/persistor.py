#!/usr/bin/env python
#encoding: utf-8
"""
    service charger de persister dans ED les messages temps réel
"""
import sys

from persistor.daemon import Persistor
import argparse


def main():
    """
        main: ce charge d'interpreter les parametres de la ligne de commande
    """
    parser = argparse.ArgumentParser(description="Launcher of persistor")
    parser.add_argument('config_file', type=str)
    config_file = ""
    try:
        args = parser.parse_args()
        config_file = args.config_file
    except argparse.ArgumentTypeError:
        print("Bad usage, learn how to use me with %s -h" % sys.argv[0])
        sys.exit(1)

    daemon = Persistor()
    daemon.load_config(config_file)
    daemon.run()


    sys.exit(0)


if __name__ == "__main__":
    main()

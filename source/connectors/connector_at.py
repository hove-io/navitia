#!/usr/bin/env python
#encoding: utf-8
"""
    service charger de créer les messages temps réel à partir de la base at
"""
import sys

from connectors.at.daemon import ConnectorAT
import argparse


def main():
    """
        main: ce charge d'interpreter les parametres de la ligne de commande
    """
    parser = argparse.ArgumentParser(description="connector_at se charge "
            "de génerer les flux temps réels associés à "
            "une base at")
    parser.add_argument('config_file', type=str)
    config_file = ""
    try:
        args = parser.parse_args()
        config_file = args.config_file
    except argparse.ArgumentTypeError:
        print("Bad usage, learn how to use me with %s -h" % sys.argv[0])
        sys.exit(1)

    daemon = ConnectorAT()
    daemon.init(config_file)
    daemon.run()


    sys.exit(0)


if __name__ == "__main__":
    main()

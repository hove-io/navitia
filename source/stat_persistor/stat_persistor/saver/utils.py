#encoding: utf-8
import datetime


class FunctionalError(ValueError):
    """
    Exception lancé lorsque que la donnée à traiter n'est pas valide
    """
    pass


class TechnicalError(ValueError):
    """
    Exception lancé lors d'un probléme technique
    typiquement la base de données est inaccessible
    """
    pass


def from_timestamp(timestamp):
    #@TODO: pour le moment on remet à l'heure local
    #à virer le jour ou kraken géreras tout en UTC
    return datetime.datetime.fromtimestamp(timestamp)


def from_time(time):
    return datetime.datetime.utcfromtimestamp(time).time()


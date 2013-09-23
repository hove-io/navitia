#encoding: utf-8
import datetime
from sindri.saver import FunctionalError

def from_timestamp(timestamp):
    #@TODO: pour le moment on remet à l'heure local
    #à virer le jour ou kraken géreras tout en UTC
    return datetime.datetime.fromtimestamp(timestamp)

def from_time(time):
    return datetime.datetime.utcfromtimestamp(time).time()


def parse_active_days(active_days):
    """
    permet de parser les champs active_days
    retourne la valeur par défaut si non initialisé: "11111111"
    sinon vérifie que c'est bien une chaine de 8 0 ou 1 et la retourne
    raise une FunctionalError si le format n'est pas valide
    """
    if not active_days:
        return '11111111'
    else:
        if (active_days.count('0') + active_days.count('1')) != 8:
            raise FunctionalError('active_days not valid: ' + active_days)
        return  active_days

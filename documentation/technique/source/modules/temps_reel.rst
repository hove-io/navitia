Temps réel et perturbations
===========================

Navitia gère plusieurs notions de temps réel :

    * horaires théoriques
    * horaires adaptées
    * perturbations
    * messages


Alerte Trafic
==============
Alerte trafic gère deux types de temps réel: des messages et des perturbations.

Messages
--------
Les messages permettent d'associer un texte à un objet TC.

Les messages ont les propriété suivantes:
- une URI
- une période de publication
- une période d'application
- une période d'application journalières
- un calendrier d'application
- un objet TC
- un titre
- un contenu

Les deux textes (titre et contenu) peuvent être dans plusieurs langues.


Perturbations
-------------
Les perturbations permettent de supprimer des circulations, où de supprimer des dessertes sur une circulation.

Les perturbations ont les propriété suivantes:
- une URI
- une période d'application
- une période d'application journalières
- un calendrier d'application
- un objet TC

Les perturbations sur les objets suivants supprime des circulations:
- Line
- VehicleJourney
- Network

Pour que la circulation soit supprimée le départ à l'origine doit être dans la période d'application de la perturbation.


Les perturbations sur les objets suivants supprime des dessertes:
- StopArea
- StopPoint

Pour que la desserte soit supprimée le départ ou l'arrivée doit être dans la période d'application de la perturbation.


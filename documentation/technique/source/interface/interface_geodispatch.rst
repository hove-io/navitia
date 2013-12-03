EJP - point d'entrée unique
===========================

L'EJP est un webservice destiné à sélectionner et à relayer automatiquement une requête vers le moteur NAViTiA le plus approprié pour générer la réponse.
Le choix est fait en fonction des coordonnées géographiques par exemple.

Pour l'instant seules deux services sont gérées par l'EJP :

* journeys
* journeysarray

Les paramètres d'appels sont donc strictement les mêmes que pour un appel direct à NAViTiA (voir la description des interfaces Journeys).

La seule restriction est que les paramètres *origin* et *destination* doivent porter *sur des coordonnées*.

Cela permet à un intégrateur de calculer des itinéraires sur différents services NAViTiA de manière totalement
transparente, sans savoir quel moteur est effectivement utiliés.

Fonctionnement
**************

Ce composant charge la géométrie de toutes les communes de France. À partir
des coordonnées, il retrouve dans quelle commune se trouve la coordonnée demandée.

Un premier fichier de configuration associe chaque commune à un moteur NAViTiA.

Un deuxième fichier de configuration associe chaque moteur NAViTiA à une URI. La requête est passée
telle quelle au bon moteur NAViTiA sous-jacent. La réponse de l'EJP est la réponse du moteur NAViTiA correspondant.

Retours
*******
Lorsque tous les paramètres sont bons, la réponse sera celle de NAViTiA.

Cependant l'EJP peut retourner des réponses spécifiques :

* Lorsque une des deux coordonnées n'est pas dans une commune couverte par un NAViTiA
* Lorsque le départ ou la destination ne correspondent pas au même moteur NAViTiA: le calcul porte-à-porte trans-moteur n'est pas géré pour le moment.

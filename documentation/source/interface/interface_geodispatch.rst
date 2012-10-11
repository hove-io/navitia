Geo dispatcher
==============

Le GeoDispatcher est un webservice destiné à sélectionner le bon moteur NAViTiA en
fonction des coordonnées géographiques et à relayer la requête.

Pour l'instant seules deux interfaces sont gérées par GeoDispatcher :

* journeys
* journeysarray

Les paramètres d'appels sont les mêmes que pour un appel direct à NAViTiA (voir la description des interfaces Journeys).

La seule restriction est que les paramètres *origin* et *destination* doivent porter sur des coordonnées.

Cela permet à un intégrateur de calculer des itinéraires sur différents services NAViTiA de manière totalement
transparente, sans avoir à savoir quel moteur est effectivement utiliés.

Fonctionnement
**************

Ce composant charge la géométrie de toutes les communes de France. À partir
des coordonnées, il retrouve il dans quelle commune se trouve la coordonnées demandée.

Un premier fichier de configuration associe chaque commune à un moteur NAViTiA.

Un deuxième fichier de configuration associe chaque moteur NAViTiA à une URL. La requête est passée
telle quelle au NAViTiA. La réponse de GeoDispatcher est la réponse du NAViTiA correspondant.

Retours
*******

Lorsque tous les paramètres sont bons, la réponse sera celle de NAViTiA.

Cependant GeoDispatcher peut retourner des réponses spécifiques :

* Lorsque une des deux coordonnées n'est pas dans une commune couverte par un NAViTiA
* Lorsque le départ ou la destination n'est pas couverte par un NAViTiA

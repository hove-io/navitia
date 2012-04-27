Street network
===============

Ce module est en charge de gérer le référentiel routier.

Ce référentiel peut être alimenté par différentes sources de données telles que 

* Téléatlas
* OpenStreetMap
* ...

Le module permet essentiellement de :

* Calculer des itinéraires sur la voirie
* Convertir des coordonnées en adresses et vice-versa

Services publics
----------------

StreetNetwork
*************
Cette API calcul un itinéraire entre deux points

**Entrées** :

* startLon, startLat, endLon, endLat : obligatoire, réels, coordonnées de début et de fin en degrés décimale
* mode : optionnel, chaîne de caractère qui indique le mode utilisé sur l'itinéraire; valeurs acceptées : foot (par défaut), car, bike

**Sortie** :

* Informations générales :
    
  * Longueur du trajet (en mètres)
  * Durée du trajet (en secondes)

* Information de rendu graphique :

  * Liste de coordonnées permettant un rendu sur une carte

* Information de guidage :

  * Liste de segments permettant de guider l'utilisateur. Un segement se compose du nom de rue et de la longuer à suivre sur cette rue

Géocodage (inverse ou non)
**************************

À partir d'une coordonnées, on retourne l'adresse.

À partir d'une adresse, on retourne une coordonnée.

Services internes
-----------------

Indexe spatial
**************
Cela permet de faire les opérations suivantes optimisés à l'aide d'un k-d tree:

* Trouver tous les items dans un rayon de X mètres
* Trouver le N éléments les plus proches

Nearest Segment
***************

Permet de trouver le segment le plus proche des coordonnées.
Les k-d tree ne permettant que d'indexer des points, on fait l'approximation suivante :

#. On cherche le nœud le plus proche
#. Pour chaque arc adjacent, projeter le nœud sur l'arc
#. Garder l'arc le plus proche
#. Retourner l'arc, la position projetée, la distance au segment

Sur la figure suivante, le point x sera projeté sur le point p

.. aafig::

       |
      po x
       |
       |
  -----O---------
       |
       |

Lorsque la projection orthogonale tombe en dehors de l'arc, la projection tombe au nœud le plus proche. Ainsi le point x est projeté sur u :


.. aafig::

   x
    .
     . 
      o-------------o
      u             v

Calcul isochrone
****************

Calcul les itinéraires d'un nœud vers tous les nœuds. Cela permet à partir d'une adresse retrouver quels sont les arrêts les plus proches.

Le calcul permet de passer en paramètre un entier qui représente la distance maximale.

Trouver les points d'arrêt les plus proches
*******************************************

À partir d'une coordonnée, on veut retrouver tous les arrêtes accessibles à une certaine distance. Le calcul se fait de la manière suivante :

#. Trouver tous les stop point à moins de X mètres
#. Trouver l'arc le plus proche pour chaque stop point
#. Lancer un calcul isochrone bridé à X mètres
#. Pour chaque stop point, retourner l'extrémité du segment la plus proche

Il se peut que pour un stop point, il n'y ait pas de trajet acceptable.

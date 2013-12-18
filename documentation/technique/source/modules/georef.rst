Référentiel géographique
========================

Le module GeoRef est en charge de gérer le référentiel des données géographiques.

Ce référentiel peut être alimenté par différentes sources de données telles que :

* Téléatlas
* OpenStretMap
* ...

Dictionnaire
------------

* Une adresse( numéro d'une maison) représente un point dans la rue, elle se caractérise par les éléments suivants :
	#. Numéro dans la rue
	#. Coordonnées géographiques

* Rue(voie) se caractérise par les éléments suivants:
	#. Type de la rue
	#. Nom de la rue
	#. Liste des adresses à gauche de la rue
	#. Liste des adresses à droite de la rue

Fonctionnement général
----------------------

Ce module est en charge de :

* Calculer des itinéraires sur la voirie.
* Convertir des coordonnées en adresses et vice-versa.

Paramètres d'entrée
-----------------------------

les paramètres d'entrée de ce module peuvent être dans les cas suivants:

Caclcul d'itinéraire
*********************

* startLon, startLat, endLon, endLat : obligatoire, réels, coordonnées de début et de fin en degrés décimale
* mode : optionnel, chaîne de caractère qui indique le mode utilisé sur l’itinéraire; valeurs acceptées : foot (par défaut), car, bike

Conversion coordonnées en adresse
*********************************

* Des coordonnées : Lon, Lat

Conversion adresse en coordonnées
*********************************

* Une adresse qui peut être constituée d'un numéro, nom de la rue et la commune.


Format de sortie
-----------------------------
Le module fournit en sortie les informations suivantes :

* Informations générales :

  * Longueur du trajet (en mètres)
  * Durée du trajet (en secondes)

* Information de rendu graphique :

  * Liste de coordonnées permettant un rendu sur une carte

* Information de guidage :

  * Liste de segments permettant de guider l'utilisateur. Un segement se compose du nom de rue et de la longuer à suivre sur cette rue

Transformation/calcul du service
--------------------------------

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
      po- - - -+ x
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
#. Trouver le nœud le plus proche
#. Lancer un calcul isochrone bridé à X mètres
#. Pour chaque stop point, projeter sur le segment le plus proche

    #. Pour les deux extrémités, verifier si la distance jusqu'à ce nœud plus la distance de la projection est inférieure à X mètre
    #. Si oui, la rajouter à la liste des solution à retourner

Il se peut que pour un stop point, il n'y ait pas de trajet acceptable.

Trouver une adresse à partir de coordonnées
*******************************************

À partir d'une coordonnée, on cherche l'adresse. la recherche se fait la façon suivante :

#. Trouver la rue à partir des coordonnées
#. Trouver le numéro dont sa distance et la plus petite par rapport aux coordonnées.

Trouver les coordonnées d'une adresse
*************************************

À partir d'une adresse, on cherche les coordonnées. la recherche se fait la façon suivante :

#. Le point d'entrée (par exemple FirstLetter) identifie l'ID de la rue et le numéro recherché dans la rue à partir d'une adresse manuellement saisie
#. Si le numéro existe dans la rue, on récupère ses coordonnées.
#. Dans le cas où la rue contient des adresses et aucune ne correspond à l'adresse recherchée :

    #. Si l'adresse recherchée à un numéro plus grand à tous les numéros de la rue, on récupère les coordonnées de l'adresse dont le numéro est le plus grand.
    #. Si l'adresse recherchée à un numéro plus petit à tous les numéros de la rue, on récupère les coordonnées de l'adresse dont le numéro est le plus petit.
    #. Dans le cas contraire, on extrapole les coordonnées.

**Cas particuliers :**

Dans les cas suivants, on récupère le barycentre de la rue :

* Si la rue ne contient aucun numéro.

* Si l'adresse recherchée est pair et la rue ne contient que des numéros impairs.

* Si l'adresse recherchée est impair et la rue ne contient que des numéros pairs.

Module utilisé
**************

Fonctions internes
*******************

UseCase
**************

Exemple d'utilisation
---------------------

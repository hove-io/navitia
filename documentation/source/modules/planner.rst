Planner
=======

Le module Planner est en charge des calculs d'itinéraires multi-modaux, multi-transporteur.
Ce module expose plusieurs API de type GET

  * journeys: retourne les différents itinéraires pertinents par rapport à une demande
  * journeysarray: retourne la liste des itinéraires "arrivée au plus tôt" pour une liste d'horaire

Fonctionnement général
**********************

.. toctree::
   :maxdepth: 2
   
   planner_general_function.rst

Paramètres d'entrée
*******************

Comme toute les fonctions exposées de NAViTiA, le calcul d'itinéraire accepte des requêtes GET ou POST. 
Il est toutefois conseillé d'utiliser la méthode POST sur ce type de requête, les URI pouvant être assez longue.

Certains paramètres sont partagés pour toutes les API de calcul d'itinéraire. 

Paramètres généraux
-------------------

+-----------------+-----------------------------+-------------------------------------+---------------------------------------+
| Paramètre       | Type                        | Remarque                            | Exemple                               |
+=================+=============================+=====================================+=======================================+
| Origin          | Appel par coordonnées:      | Coordonnées en WGS84                | &origin=coord:48.829934:2.391728      |
|                 |    coord:<lon>:<lat>        | Ne pas confondre lat et lon…        |                                       |
|                 +-----------------------------+-------------------------------------+---------------------------------------+
|                 | Appel par code unique:      | Issu du retour d'un appel à la      | &origin=stop_area:TCL-0722            |
|                 |    URI issue de FirstLetter | fonction FirstLetter                |                                       |
+-----------------+-----------------------------+-------------------------------------+---------------------------------------+
| Destination     | Appel par coordonnées:      | Coordonnées en WGS84                | &destination=coord:48.8402:2.3193     |
|                 |    coord:<lon>:<lat>        | Ne pas confondre lat et lon…        |                                       |
|                 +-----------------------------+-------------------------------------+---------------------------------------+
|                 | Appel par code unique:      | Issu du retour d'un appel à la      | &destination=stop_area:5555555        |
|                 |    URI issue de FirstLetter | fonction FirstLetter                |                                       |
+-----------------+-----------------------------+-------------------------------------+---------------------------------------+
| ClockWise       | booléen                     | Itinéraires "partir après"          | &ClockWise=1                          |
|                 |                             | Si &ClockWise=0, les itinéraires    |                                       |
|                 |                             | seront du type "arriver avant"      |                                       |
+-----------------+-----------------------------+-------------------------------------+---------------------------------------+



Paramètres spécifiques à l'API "Journeys"
-----------------------------------------

+-----------------+--------------------------+-------------------------------------+------------------------------------------+
| Paramètre       | Type                     | Remarque                            | Exemple                                  |
+=================+==========================+=====================================+==========================================+
| DateTime        | <yyyymmdd>T<hhmi>        | Horaire demandé                     | &DateTime=20121007T0715                  |
|                 |                          | Le fuseau horaire n’est pas géré    |                                          |
+-----------------+--------------------------+-------------------------------------+------------------------------------------+



Paramètres spécifiques à l'API "JourneyArray"
---------------------------------------------

+-----------------+--------------------------+-------------------------------------+------------------------------------------+
| Paramètre       | Type                     | Remarque                            | Exemple                                  |
+=================+==========================+=====================================+==========================================+
| DateTime[]      | <yyyymmdd>T<hhmi>        | Liste d’horaires demandés           | &DateTime[]=20121007T0715&DateTime[]=    |
|                 | paramètre répété n fois  | Le fuseau horaire n’est pas géré    |   20121007T0807&DateTime[]=20121007T0853 |
+-----------------+--------------------------+-------------------------------------+------------------------------------------+


Format de sortie
****************

  :download:'exemple de flux <../_static/planner_example.json>'


Transformation/calcul du service
********************************


.. toctree::
   :maxdepth: 2
   
   planner_algo.rst

Exemple d'utilisation
*********************

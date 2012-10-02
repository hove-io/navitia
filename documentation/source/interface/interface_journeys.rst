Journeys
========

La fonction Journeys est en charge des calculs d'itinéraires multi-modaux, multi-transporteur.
Cette fonction est exposé sous différentes API de type GET

  * journeys: retourne les différents types d'itinéraires pertinents par rapport à une demande
  * journeysarray: retourne la liste des itinéraires "arrivée au plus tôt" pour une liste d'horaire. Pour chaque horaire, une seule réponse sera fournit, celle-ci pouvant être "pas de solution".

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



Paramètres spécifiques à l'API "JourneysArray"
---------------------------------------------

+-----------------+--------------------------+-------------------------------------+------------------------------------------+
| Paramètre       | Type                     | Remarque                            | Exemple                                  |
+=================+==========================+=====================================+==========================================+
| DateTime[]      | <yyyymmdd>T<hhmi>        | Liste d’horaires demandés           | &DateTime[]=20121007T0715&DateTime[]=    |
|                 | paramètre répété n fois  | Le fuseau horaire n’est pas géré    |   20121007T0807&DateTime[]=20121007T0853 |
+-----------------+--------------------------+-------------------------------------+------------------------------------------+

Exemple d'appel:

http://www.navitia.com/journeysarray?format=json&origin=coord:4.915:45.731&destination=coord:4.825:45.760&datetime[]=20120930T0800&datetime[]=20120930T0900&datetime[]=20120930T1000&clockwise=1#jsonNav_l


Format de sortie
****************

Itinéraire sans solution
------------------------

Les itinéraires sans solutions sont présentés sous la forme suivante:

.. image:: ../_static/ex_journey_noresponse.png

La liste des types de non réponse est décrite de façon exhaustive dans le fichier navitia.proto

Réponse au service "journeysarray"
----------------------------------


En sortie, NAViTiA fournit une liste d'objet "Journey" contenant chacun un itinéraire présenté intégralement.

Cet itinéraire combine tous les modes couvert par le NAViTiA sous-jacent, c'est à dire, à minima, le mode piéton.

En fonction du mode utilisé pour chaque étape ("section") de l'itinéraire, le détail peut être

  * soit de type "ROAD_NETWORK" ("filaire de voirie", pour les modes piéton, vélos, véhicule personnel...) 
  * soit de type "PUBLIC_TRANSPORT" ("transport en commun" pour les modes bus, train, avion...) 

La réponse repliée est structurée ainsi:


.. image:: ../_static/ex_journey_folded.png


Section de transport en commun dépliée:

.. image:: ../_static/ex_journey_publictransport_unfolded.png


Section de marche à pied dépliée:

.. image:: ../_static/ex_journey_roadnetwork_unfolded.png


Liste des points d'arrêts intermédiaires dépliée:

.. image:: ../_static/ex_journey_stoppointlist_unfolded.png



Exemple d'utilisation
*********************

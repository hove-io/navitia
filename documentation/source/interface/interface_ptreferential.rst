Requêtes sur le référentiel d'objet de transport public
=======================================================

Le module "PTReferential" de NAViTiA est en charge de l'exposition des données du référentiel de transport en commun.
Il permet aux intégrateur de récupérer des listes d'objets de transports (arrêts, lignes, modes...) 
en fonction de multiples filtres, sans nécessairement avoir de notion du MCD sous-jacent.

Ce module expose différentes API de type GET. Par exemple:

  * stopareas: retourne une liste de zones d'arrêt
  * stoppoints: retourne une liste de points d'arrêt
  * lines: retourne une liste de lignes
  * modes: retourne une liste de modes commerciaux

Pour chacun de ces types de données, il est possible de filtrer par un autre type de données.

Chaque requête sera donc encodée sous la forme suivante:

\http://www.navitia.com/navitia/*{REQUESTED_OBJECTS}*?format={JSON/XML/PB}&filter={*FILTER_SEMANTIC*}

Paramètres d'entrée
*******************

Type d'objet
------------

\http://www.navitia.com/navitia/**{REQUESTED_OBJECTS}**?format={JSON/XML/PB}&filter={FILTER_SEMANTIC}

Le premier paramètre est en fait le nom de l'API exposé. Il correspond au type d'objet à choisir. Celui-ci est déterminant pour la structure des objets en sortie. 

  * /lines: retourne une liste de lignes
  * /routes: retourne une liste de parcours
  * /vehiclejourneys: retourne une liste de circulations
  * /stoppoints: retourne une liste de points d'arrêt
  * /stopareas: retourne une liste de zones d'arrêt
  * /networks: retourne une liste de réseaux commerciaux
  * /companies: retourne une liste de transporteurs
  * /modes: retourne une liste de modes commerciaux
  * /modetypes: retourne une liste de modes standardisés
  * /cities: retourne une liste de communes
  * /connections: retourne une liste de correspondances
  * /routepoints: retourne une liste de points d'arrêt sur parcours
  * /districts: retourne une liste de départements
  * /countries: retourne une liste de pays


Filtres complémentaires
-----------------------

\http://www.navitia.com/navitia/{REQUESTED_OBJECTS}?format={JSON/XML/PB}&filter={**FILTER_SEMANTIC**}

La clause *filter* permet de fournir à NAViTiA la règle de restriction à appliquer sur l'ensemble des données de type *REQUESTED_OBJECTS*.
Celle-ci s'exprime sous la même forme qu'une clause *WHERE* de SQL simplifiée.
Ainsi:

  * seul les opérateurs suivant sont interprétés: *AND, IN, OR, (, )*
  * les filtres peuvent porter sur les objets suivants:
    
    * stoparea
    * route
    * vehiclejourney
    * stoparea
    * stoppoint
    * network
    * company
    * mode
    * modetype
    * city
    * connection
    * routepoint
    * district
    * country

  * sur chacun des objets sus-cités, les propriétés suivantes sont filtrables:

    * external_code: permet de filtrer les données retournées selon le code externe de l'objet
    * name: permet de filtrer les données retournées selon le nom de l'objet
    * ID:  permet de filtrer les données retournées selon l'ID initial de l'objet (cet ID permet la traçabilité de l'objet et n'est pas unique)

  * certains filtres sont spécifiques à certains objets

    * pour les lignes

      * code: permet de filtrer les lignes selon leur code commercial

    * pour les arrêts

      * main: permet de filtrer les zones d'arrêts selon leur taille


Pour chaque type de filtre, il est possible de restreinte de 1 à n code externe en utilisant les opérateur OR, AND ou IN:
  * filtre simple: \http://.../navitia/lines?format=json&filter=modeec=BUS
  * filtres multiples: \http://.../navitia/lines?format=json&filter=StopArea.external_code=SA_CHATELET and (Mode.external_code=BUS or mode.external_code=Metro)
  * filtres combinés: \http://.../navitia/lines?format=json&filter=Mode.external_code in (BUS,Metro)


Format de sortie
****************

Le format de sortie est fonction du type d'objet souhaité. Chaque objet est auto-descriptif. 

Exemple d'utilisation
*********************

Exemples :

  * Récupérer un arrêt spécifique : http://.../navitia/stopareas?format=json&StopArea=SA_0001 ou encore : http://navitia/StopAreas?format=pb&StopArea=SA_0001
  * Liste de tous les arrêts : http://.../navitia/stopareas?format=json
  * Liste des zones d'arrêts de la ligne RER A : http://.../navitia/stopareas?format=json&line=LI_RER_A 
  * Liste des lignes de la zone d'arrêt Châtelet : http://navitia/lines?format=json&StopArea=SA_CHATELET 
  * Liste des lignes de bus de la zone d'arrêt Châtelet : http://.../navitia/lines?format=json&StopArea=SA_CHATELET&Mode=BUS 
  * Liste des lignes de bus et de Metro de la zone d'arrêt Châtelet : http://.../navitia/lines?format=json&StopArea=SA_CHATELET&Mode[]=BUS&Mode[]=Metro 
  * Et… Liste des lignes autour de la coordonnée d’une adresse : http://.../navitia/lines?format=json&coord=4.915:45.731


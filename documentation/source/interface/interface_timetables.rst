Les horaires
=============

Les interfaces contenues dans timetables permettent de renvoyer des horaires présentés de différentes manières

Liste des interfaces :
	* next_departures : Renvoie la liste des prochains départs selon les paramètres passés en entrée 
	* departure_board : Renvoie la liste des départs/arrivées entre deux arrêts
	* line_schedule : Renvoie la liste des horaires d'une ligne 	



NextDepartures 
****************

Paramètres d'entrée
---------------------

+---------------+------------------------+-------------------------------------+----------------------------------------+
| Paramètre     | Type                   | Remarque                            | Exemple                                |
+===============+========================+=====================================+========================================+
| filter        | Filter                 | Filtre sur les horaires que l'on    | filter=line.external_code=line:TCL01   |
|               |                        | veut.                               | stop_point.external_code=stop_point:01 |
|               |                        | Reprend exactatement la synthaxe    |                                        |
|               |                        | les filtres de pt_ref.              |                                        |
+---------------+------------------------+-------------------------------------+----------------------------------------+
| datetime      | <yyyymmdd>T<hhmi>      | La date à partir de laquelle on     | datetime=20121122T1054                 |
|               |                        | souhaite les horaires.              |                                        |
+---------------+------------------------+-------------------------------------+----------------------------------------+
| max_datetime  | <yyyymmdd>T<hhmi>      | La date maximale pour les horaires. | max_datetime=20121122T1056             |
+---------------+------------------------+-------------------------------------+----------------------------------------+  
| nb_departures | nombre entier          | Le nombre maximal d'horaires        | nb_departures=2000                     |
|               |                        | souhaité                            |                                        |
+---------------+------------------------+-------------------------------------+----------------------------------------+
| max_depth     | nombre entier          | Même argument que dans ptref        | max_depth=2                            |
+---------------+------------------------+-------------------------------------+----------------------------------------+
| wheelchair    | booléen                | &wheelchair=1 on veut que les       | &wheelchair=1                          |
|               |                        | départs prenent en compte           |                                        |
|               |                        | l'accessibilité                     |                                        |
|               |                        | &wheelchair=0 on ne prend pas en    |                                        |
|               |                        | l'accessibilité                     |                                        |
+---------------+------------------------+-------------------------------------+----------------------------------------+

Remarques : 
	* Si aucun des deux paramètres *nb_departures* ou *max_datetime* n'est renseigné il sera alors assigné au paramètre *nb_departure* la valeur 10.
	* Si les deux paramètres limitant *nb_departures* et *max_datetime* sont renseignés, les deux paramètres par défaut sont alors pris en compte. La liste sera coupé à la première des conditions atteinte. 

Format de sortie
------------------

Liste de stopTime : 

DepartureBoard
**************** 

Paramètres d'entrée
---------------------

+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| Paramètre               | Type                   | Remarque                            | Exemple                                |
+=========================+========================+=====================================+========================================+
| departure_filter        | Filter                 | Filtre sur les horaires de départ.  | filter=line.external_code=line:TCL01   |
|                         |                        |                                     | stop_point.external_code=stop_point:01 |
|                         |                        | Reprend exactatement la synthaxe    |                                        |
|                         |                        | les filtres de pt_ref.              |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| arrival_filter          | Filter                 | Filtre sur les horaires d'arrivée.  | filter=line.external_code=line:TCL01   |
|                         |                        |                                     | stop_point.external_code=stop_point:01 |
|                         |                        | Reprend exactatement la synthaxe    |                                        |
|                         |                        | les filtres de pt_ref.              |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| datetime                | <yyyymmdd>T<hhmi>      | La date à partire de laquelle on    | datetime=20121122T1054                 |
|                         |                        | les horaires.                       |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| max_datetime            | <yyyymmdd>T<hhmi>      | La date maximale pour les horaires. | max_datetime=20121122T1056             |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+  
| nb_departures           | nombre entier          | Le nombre maximal d'horaires        | nb_departures=2000                     |
|                         |                        | souhaité                            |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| max_depth               | nombre entier          | Même argument que dans ptref        | max_depth=2                            |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+

Mêmes remarques que précédement.

Format de sortie
------------------


LineSchedule
**************

Paramètres d'entrée
---------------------

+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| Paramètre               | Type                   | Remarque                            | Exemple                                |
+=========================+========================+=====================================+========================================+
| filter                  | Filter                 | filtre sur les lignes.              | filter=line.external_code=line:TCL01   |
|                         |                        | Reprend exactatement la synthaxe    |                                        |
|                         |                        | les filtres de pt_ref.              |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| datetime                | <yyyymmdd>             | La date à partire de laquelle on    | datetime=20121122T1054                 |
|                         |                        | les horaires.                       |                                        |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+
| changetime              | T<hhmi>                | L'heure de départ du schedule     . | changetime=T0500                       |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+  
| max_depth               | nombre entier          | Même argument que dans ptref        | max_depth=2                            |
+-------------------------+------------------------+-------------------------------------+----------------------------------------+


                                

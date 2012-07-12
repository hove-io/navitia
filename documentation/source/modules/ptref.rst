PT Referential
==============

Le module PTReferential permet de retrouver des objets de transport en commun en utilisant des requêtes en pseudo-SQL.

Fonctionnement général
----------------------

Le module PTReferential traite les requêtes saisies par les utilisateurs et renvoie toutes les colonnes des objets sélectionnés en respectant les conditions de sélection.

Paramètres d'entrée
-----------------------------

Le module à un seul paramètre en entrée, ce dernier est sous la forme d'une requête en pseudo-SQL.

La syntaxe de la requête est la suivante:

**SELECT** <*select_list*> **FROM** <*object_source*> [ **WHERE** <*search_condition*> ]

avec :

* *select_list* ={object_name.column_name}[, ...n]

  * object_name : le nom de l'objet type stop_points, stop_areas, ...
  * column_name : le nom de la colonne de l'objet type name, external_code, ...

* *object_source* ={object_name}

  * object_name : le nom de l'objet "table" type stop_points, StopArea, ...

* *search_condition* ={object_name.column_name OPERATOR {Value | object_name.column_name}} [,...n]

  * object_name : le nom de l'objet type stop_points, stop_areas, ...
  * column_name : le nom de la colonne de l'objet type name, external_code, ...
  * OPERATOR : l'opérateur logique.

Opérateurs logiques de PTReferential
**************

Les opérateurs acceptés par le module PTReferential sont : 
 * **=**  : Opérateur égal
 * **<>** : Opérateur différent
 * **<**  : Opérateur inférieur
 * **>**  : Opérateur supérieur
 * **<=** : Opérateur inférieur ou égal
 * **>=** : Opérateur supérieur ou égal

Format de sortie
-----------------------------

Transformation/calcul du service
--------------------------------

Après réception de la requête, la procédure suivie par PTReferential pour donner une réponse est la suivante :

#. Parsage de la requète pour récuperer les informations suivantes :

    #. les colonnes à sélectionner <*select_list*>
    #. l'objet utilisé pour la sélection <*object_source*>
    #. les colonnes utilisées dans la clause <*search_condition*>

#. Regrouppement des conditions <*search_condition*> par objet

#. Récupération de l'ensemble des index de l'objet <*object_source*> dans une liste: **Object_Index**

#. Pour chaque groupe de conditions par objet :

    #. Récupération des index de l'objet <*object_source*> en respectant le groupe de condistions: **Condition_Index**.
    #. Récupération de l'intersection des index dans une liste: **Result_Index** = Intersection(**Object_Index**, **Condition_Index**).

#. Récupération des colonnes <*select_list*> de la liste finale des index pour construire la réponse.

Module utilisé
**************

Fonctions internes 
**************

UseCase
**************

Limitation
----------------------

le module ne permet pas de répondre à des requêtes avec plusieures objets dans la clause "FROM".

Exemple d'utilisation
---------------------

dans l'exemple, nous supposons que nous avons deux objets **stop_areas** et **stop_points** dont la structure et les données de chaqu'un sont:

 * stop_areas
+---------------------------+------------------------+
|   external_code           |        name            |
+===========================+========================+
|       10186               |         Europe         |
+---------------------------+------------------------+
|       10182               |         Ecluzelles     |
+---------------------------+------------------------+
|       10179               |         Maraudeau      |
+---------------------------+------------------------+
|       10191               |         Lanaja         |
+---------------------------+------------------------+

 * stop_points 
+---------------------------+------------------------+------------------------+
|   external_code           |         name           |        stop_area_idx   |
+===========================+========================+========================+
|       20347               |         Europe         |       10186            |
+---------------------------+------------------------+------------------------+
|       20348               |         Europe         |       10186            |
+---------------------------+------------------------+------------------------+
|       20339               |         Ecluzelles     |       10182            |
+---------------------------+------------------------+------------------------+
|       20340               |         Ecluzelles     |       10182            |
+---------------------------+------------------------+------------------------+
|       20333               |         Maraudeau      |       10179            |
+---------------------------+------------------------+------------------------+
|       20334               |         Maraudeau      |       10179            |
+---------------------------+------------------------+------------------------+
|       20357               |         Lanaja         |       10191            |
+---------------------------+------------------------+------------------------+
|       20358               |         Lanaja         |       10191            |
+---------------------------+------------------------+------------------------+

Requête 1 : *select stop_areas.external_code, stop_areas.name from stop_areas where stop_areas.name=Europe*

Résultat :

+---------------------------+------------------------+
|   external_code           |        name            |
+===========================+========================+
|       10186               |         Europe         |
+---------------------------+------------------------+


Requête 2 : *select stop_points.external_code, stop_points.name from stop_points where stop_points.name=Europe*

Résultat :

+---------------------------+------------------------+
|   external_code           |         name           |
+===========================+========================+
|       20347               |         Europe         |
+---------------------------+------------------------+
|       20348               |         Europe         |
+---------------------------+------------------------+

Requête 3 : *select stop_points.name,stop_points.external_code, stop_areas.external_code, stop_areas.name from stop_points where stop_areas.external_code=10186*

Résultat : les données de l'objets stop_points

+---------------------------+------------------------+
|   external_code           |         name           |
+===========================+========================+
|       20347               |         Europe         |
+---------------------------+------------------------+
|       20348               |         Europe         |
+---------------------------+------------------------+

en plus les données de l'objet stop_areas

+---------------------------+------------------------+
|   external_code           |         name           |
+===========================+========================+
|       10186               |         Europe         |
+---------------------------+------------------------+

Tests unitaires
---------------



PT Referential
==============

Le module PTReferential permet de retrouver des objets de transport en commun en utilisant des requêtes en pseudo-SQL.

Fonctionnement général
----------------------

Le module PTReferential traite les requêtes saisies par les utilisateurs et renvoie toutes les colonnes des objets sélectionnés en respectant les conditions de sélection.

Syntaxe générale d'une requête
-----------------------------

La syntaxe d'une requête à traiter par PTReferential est la suivante :

**SELECT** *select_list* **FROM** *object_source* [**WHERE** *search_condition*] 

avec :
 * *select_list* = {object_name.column_name}[, ...n] 
   * object_name : le nom de l'objet "table" type StopPoint, StopArea, ...
   * column_name : le nom de la colonne de l'objet "table" type CommercailName, ExternalCode, ...

 * *object_source* = {object_name} 
   * object_name : le nom de l'objet "table" type StopPoint, StopArea, ...

 * *search_condition* = {object_name.column_name OPERATOR {Value | object_name.column_name}} [,...n]
   * object_name : le nom de l'objet "table" type StopPoint, StopArea, ...
   * column_name : le nom de la colonne de l'objet "table" type CommercailName, ExternalCode, ...
   * OPERATOR : l'opérateur logique.

Opérateurs logiques de PTReferential
------------------------------------

La liste des opérateurs acceptés par le module PTReferential sont :EQ, NEQ, LT, GT, LEQ, GEQ

Exemple d'une requête
-----------------------------

dans l'exemple, nous supposons que nous avons deux objets **StopArea** et **StopPoint** dont la structure et les données de chaqu'un sont:

 * StopArea
+---------------------------+------------------------+
|   ExternalCode            |        CommercialName  |
+===========================+========================+
|       10186               |         Europe         |
|       10182               |         Ecluzelles     |
|       10179               |         Maraudeau      |
|       10191               |         Lanaja         |
+---------------------------+------------------------+

 * StopPoint 

+---------------------------+------------------------+------------------------+
|   ExternalCode            |        CommercialName  |        StopArea_Id     |
+===========================+========================+========================+
|       20347               |         Europe         |       10186            |
|       20348               |         Europe         |       10186            |
|       20339               |         Ecluzelles     |       10182            |
|       20340               |         Ecluzelles     |       10182            |
|       20333               |         Maraudeau      |       10179            |
|       20334               |         Maraudeau      |       10179            |
|       20357               |         Lanaja         |       10191            |
|       20358               |         Lanaja         |       10191            |
+---------------------------+------------------------+------------------------+



Limitation
----------------------

le module ne permet pas de répondre à des requêtes avec plusieures tables dans la clause "FROM".

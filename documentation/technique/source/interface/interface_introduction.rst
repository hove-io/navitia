Introduction
============

Généralités
-----------

Tous les modules sont des web-services REST.

* http/XML
* http/JSON
* http/ProtocolBuffer

Chaque ressource étant proposée en lecture seule, seul les méthodes GET et POST sont implémentées.

Toutefois la méthode GET étant limitée à quelques ko, et la méthode POST à 100ko, il est donc conseillé d'utiliser la méthode POST sur les longues URI (calcul d'itinéraire en particulier).

NAViTiA peut retourner les codes http suivants:

+-----------+------------------------------------+----------------------------------------------------------------------+
| Code http | Correspondance                     | Remarque                                                             |
+===========+====================================+======================================================================+
| 200       | OK                                 | La requête  a aboutit et la réponse est techniquement normale.       |
|           |                                    | Response body is XML (Content-Type: application/xml)                 |
|           |                                    | or JSON (Content-type: application/json).                            |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 400       | Bad Request                        | Les paramètres de la requête sont invalides.                         |
|           |                                    | Utiliser le service Analyze de NAViTiA pour aider au debug           |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 401       | Unauthorized                       | Non gérée pour le moment                                             |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 403       | Forbidden                          | Non gérée pour le moment                                             |
|           | (Authenticated but not authorized) | Non gérée pour le moment                                             |
|           |                                    | Non gérée pour le moment                                             |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 500       | Internal Server Error              | Erreur technique globale                                             |
+-----------+------------------------------------+----------------------------------------------------------------------+

.. warning::
   Les erreurs purement fonctionnelles sont retournée en code 200, l'erreur étant spécifiée dans la réponse.
   Par exemple, pour une réponse sur un itinéraire sans solution: "pas de solution d'itinéraire: pas de données pour la date fournie en paramètre"
   Il est nécessaire que l'application cliente prenne en compte ce type de réponse.



Versioning d'interface
----------------------

Attention, l'interface de sortie JSON est susceptible d'évoluer en "compatibilité ascendante":

  * Les noeuds décris dans une version d'interface n seront toujours présent sous la même forme dans les versions n+1 (même nom, même "type")
  * Des nouveaux noeuds sont susceptibles d'apparaitre, qui ne modifient pas la structure de l'interface n
  * Aucun noeud ne disparait depuis une version n vers une version n+1



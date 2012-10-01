Introduction
============

Généralités
-----------

Tous les modules sont des web-services REST.

* http/XML
* http/JSON
* http/ProtocolBuffer

Chaque ressource étant proposée en lecture seule, seul les méthodes GET et POST sont implémentées. 

Toutefois les méthodes GET sont limités à 2ko, et les méthodes POST à 100ko, il est donc conseillé d'utiliser les méthodes POST sur les longues URI (calcul d'itinéraire en particulier).

NAViTiA peut retourner les codes http suivants:

+-----------+------------------------------------+----------------------------------------------------------------------+
| Code http | Correspondance                     | Remarque                                                             |
+===========+====================================+======================================================================+
| 200       | OK                                 | Request completed successfully.                                      |
|           |                                    | Response body is XML (Content-Type: application/xml)                 |
|           |                                    | or JSON (Content-type: application/json).                            |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 400       | Bad Request                        | Request parameters are invalid.                                      |
|           |                                    | The response body is plain text (Content-Type: text/plain)           |
|           |                                    | and will indicate the problem.                                       |
|           |                                    | Examples: query parameters indicate invalid column labels,           |
|           |                                    | limit provided without offset,                                       |
|           |                                    | trigger is blocking request for some domain rule violation.          |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 401       | Unauthorized                       | The request requires user authentication.                            |
|           | (Not authenticated)                | Credentials are provided in the WWW-Authenticate header              |
|           |                                    | using the HTTP Basic or Digest Authentication scheme.                |
|           |                                    | If the user name/password are provided correctly,                    |
|           |                                    | then 401 also indicates invalid credentials.                         |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 403       | Forbidden                          | The user does not have sufficient privileges to complete the request |
|           | (Authenticated but not authorized) | The user's granted role(s) is not enabled                            |
|           |                                    | for the request type on the SQL Resource.                            |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 404       | Not Found                          | SQL Resource does not exist.                                         |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 405       | Method Not Allowed                 | Request content type and method are invalid.                         |
+-----------+------------------------------------+----------------------------------------------------------------------+
| 500       | Internal Server Error              | GLobal error                                                         |
+-----------+------------------------------------+----------------------------------------------------------------------+

.. warning::
   Les erreurs purement fonctionnelles sont retournée en code 200, l'erreur étant spécifiée dans la réponse.
   Par exemple, pour une réponse "NoSolution": "pas de solution d'itinéraire: pas de données pour la date fournie en paramètre"
   Il est nécessaire que l'application cliente prenne en compte ce type de réponse.



Versioning d'interface
----------------------


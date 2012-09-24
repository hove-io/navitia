Introduction
============



Numéro de version
-----------------

Le numéro de version des applicatifs est défini sous la forme:

    X.Y.Z

Avec:
    - *X* le numéro de version majeur
    - *Y* le numéro de version mineur
    - *Z* le numéro de patch

Le numéro de patch est utilisé afin d'appliquer des corrections sur une version déjà livré, et qu'il n'est pas possible de livrer la version suivante.

Chaque version est de plus surnommée d'un nom de station ou de gare célèbre (par exemple NAViTiA "Gare d'Austerlitz")

Architecture logicielle
-----------------------
.. image:: ../_static/ArchiLogicielle.PNG

Interfaces: généralités
-----------------------

Tous les modules sont des web-services REST.

* http/XML
* http/JSON
* http/ProtocolBuffer

Chaque ressource étant proposée en lecture seule, seul les méthodes GET et POST sont implémentées.

Codes http gérés:

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
| 500       | Internal Server Error              | Generally an SQL error.                                              |
|           |                                    | Examples: the SQL Resource definition query is invalid,              |
|           |                                    | the request parameters created an invalid SQL statement,             |
|           |                                    | the request violated a database constraint.                          |
+-----------+------------------------------------+----------------------------------------------------------------------+

.. warning::
   Les erreurs purement fonctionnelles sont retournée en code 200, l'erreur étant spécifiée dans la réponse.
   Par exemple, pour une réponse "NoSolution": "pas de solution d'itinéraire: pas de données pour la date fournie en paramètre"
   Il est nécessaire que l'application cliente prenne en compte ce type de réponse.


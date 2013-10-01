Architecture Technique
++++++++++++++++++++++

Navitia se décompose en un ensemble de composant spécialisé dans un seul rôle (en règle général):

- ED
    - gtfs2ed
    - osm2ed
    - ed2nav
    - nav2rt
    - pyed
    - Sindri
- Kraken
- Jormungandr

.. image:: ../_static/Archi_navitia2.png

ED
--
ED est la base de données principale, c'est ici que l'ensemble des données sont stockées
que ce soit les horaires théorique ou temps réel, ainsi que les données de voirie.

Autour de cette base de données gravite tous les connecteurs servant à l'alimenter ou à alimenter Kraken.

La base de de données utilisée est postgresql avec l'extension postgis pour gérer les données géographiques.
Le modéle de la base est disponible dans "documentation/annexe/model.architect" 

gtfs2ed
~~~~~~~
gtfs2ed est responsable de l'intégration d'un fichier gtfs++ dans ED

osm2ed
~~~~~~
osm2ed est responsable de l'intégration d'un export OSM dans ED

ed2nav
~~~~~~
ed2nav se charge de générer pour kraken un export des données théorique et filaire.
Ce fichier est communément appelé un '.nav'

nav2rt
~~~~~~
nav2rt charge un .nav et y ajoute les données temps réel présente dans ED.

pyed
~~~~
pyed pilote l'ensemble des composants précédemment cité et et charge de les lancer quand de nouvelles données sont disponibles

Sindri
~~~~~~
Sindri est un service dont le rôle est d'enregistrer dans ED les événements temps réels.
A terme ces événements alimenterons directement kraken, et ne seront enregistré dans ED que pour la reprise sur incident

Sindri reçoit les événements via le broker AMQP sous forme de message ProtocolBuffer.

Les types d'évènements gérer sont les suivants:

- message AT

Kraken
------

Kraken est le moteur de calcul proprement dit, il charge les .nav est expose via zmq les API de calcul.



Jormungandr
-----------

Jormungandr est le web service il interprète les demandes, effectue les appels à kraken afin de constituer la réponse
puis se charge de la mettre en forme en fonction de l'interface demandé

Contrairement aux autres applications Jormungandr est partagé entre toutes les instances Navitia,
ce qui lui permettra à terme de calculer des itinéraires sur plusieurs régions.


Connecteur temps réel
----------------------
Les connecteurs ont pour tache d'interpréter les différents format existant et de publier sur AMQP les événements associés

Chaque **Contributeur** temps réel se voit associé un topic, sous la forme : realtime.contributeur
Le connecteur publie les événements sur l'exchange AMQP global (par défaut navitia) avec ce topic,
ce qui permet à toutes les instances intéressé de pouvoir recevoir ces flux.

Les éléments insérés dans l'exchange **doivent etre persistant** ce qui se réalise en fixant la propriété *delivry_mode* à **2**.
Ceci permettra à Sindri de gérer la reprise sur incident


Brokk
-----
Aussi connu sous le nom de broker, nous utilisons RabbitMQ pour assurer ce rôle.
La majorité des échanges entre les composants doivent passer par lui, seul la communication entre Jormungandr et Kraken utilise zmq,
principalement pour des raisons de performance.

Il ne devrait y avoir qu'un seul exchange partagé par toutes les instances, c'est le topic qui détermine les instances ciblés.
Par défaut le topic utilisé s'appelle "navitia".
Chaque application doit créer l'exchange avant de l'utiliser, sa configuration est la suivante:

- type : topic
- durable: True

Chaque message envoyé doit être  associé a un topic(routing_key) précis, pour le temps réel le format est le suivant: "realtime.contributor".
Pour les communication intra-instance le format est le suivant: "instance_name.type.etc",
typiquement l'ordre de rechargement envoyé par pyed à kraken est associé à la clé suivante: "instance_name.task.reload"

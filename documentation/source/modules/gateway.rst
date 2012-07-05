Le Hub d'accès
==============

Le Hub d'accès est le composant d'accès visible depuis l'extérieur. 
Il a les rôles suivant:

    - Répartition de charge et détection d'erreur
    - Génération des statistiques
    - Génération des flux de réponses dans le format demandé
    - Supervision général de l'état de la plateforme
    - Gestion de l'aspect *"élastique"*


L'approche actuellement retenu est d'avoir un seul type de  moteur de calcul qui gère toutes les API.

Répartition de charge et détection d'erreur
-------------------------------------------
Le Hub d'accès se charge d'aiguiller les requêtes qu'elle reçoit sur la bonne instance du moteur de calcul selon l'algorithme *round-robin* pondéré.
Il doit en plus tenir compte de l'état des moteurs, c'est à dire que si l'un d'entre eux est en erreur, il est sorti du pool.


Structure de données
````````````````````

La structure principale manipulé par le Hub est la classe *Navitia*.
Elle représente une instance d'un moteur de calcul est contient les attributs suivant:
    
    - url => l'url pointant vers cette instance
    - current_thread => nombre de thread disponible sur cette instance
    - unused_thread => le nombre de thread inactif, cet indicateur est utilisé pour la répartition de charge, 
      mais n'as pas de réel valeur (ex: elle peux être négative)
    - last_error_at => date de la dernière erreur
    - enable => état de l'instance, peut être désactivé dans le cas d'erreurs
    - nb_errors => nombre d'erreur rencontré par l'instance, utilisé pour déterminé combien de temps cette instance sera désactivé
    - reactivate_at => 
    - next_decrement => prochain moment ou le nombre d'erreur pourra être décrémenté

Algorithme 
``````````
Les instances de moteurs sont stocké dans un tas binaire. 
Celui ci est trié par état (activé ou pas), nombre de thread inactif et date de dernière requête traité.
De cette façon les instances désactivé ne sont jamais utilisé, sauf si il n'y à plus aucune instance valide.

De même les instances ayant le plus de puissance (avec le plus grand nombre de thread déclaré) sont les plus sollicité, 
l'intérêt principale étant de ne pas utiliser les instances *"cloud"* si nos propres serveurs ne sont pas déjà chargé.


Génération des statistiques
---------------------------

Les statistiques se basent sur les requêtes reçu, et log le type de demande, les paramètres associés, etc.
Dans le cas des calculs d'itinéraires les résultats sont aussi enregistré.

@TODO: format

Le Hub D'accès se charge de générer un fichier de statistique au format *Protocol buffer*, 
ce fichier sera traité par un autre composant pour être persisté dans une base de données quelconque.


Génération des flux de réponses dans le format demandé
------------------------------------------------------

La communication entre le Hub d'accès et les moteurs est réalisés en *Protocol Buffer*.

Le Hub D'accès à la charge de mettre en forme la réponse dans le format demandé par l'utilisateur.
Les format disponible sont:

    - XML 
    - JSON
    - Protocol buffer

Les réponses en *Protocol buffer* ne subissent aucunes transformations, 
c'est le format utilisé pour la communication avec les moteurs qui est renvoyé.
@TODO: et comment on les versionnent?

Supervision général de l'état de la plateforme
-----------------------------------------------

Le Hub d'accès doit fournir une interface de supervision de l'ensemble de la plateforme, en exposant l'état de chacun des moteurs dont il à la charge.

On peux dégager deux API:
    
    - /status
    - /params

status
``````
L'api status doit retourner les informations sur l'état de fonctionnement de toutes les instances, on peut lister entre autre:

    - temps moyen de réponse
    - dernière requête traité
    - nombre de thread
    - date de chargement des données
    - donnée chargé (nom du fichier databases)
    - voir si on peux garder une trace des différentes sources de données (ex: fichier PDT, fichier BOA, etc)


params
``````

L'api params à pour but de présenter l'ensemble des paramètres de configuration de l'application ainsi que leurs source,
c'est à dire si le paramètre à été défini dans le fichier de config, en ligne de commande, ou via la base de données.


Gestion de l'aspect *"élastique"*
---------------------------------

Afin de permettre le déploiement en environnement de type "cloud computing".

Pour ce faire les API suivantes ont étaient ajoutés
    
    - /register
    - /unregister

register
````````
L'api /register permet d'ajouter un nouveau moteur de calcul au pool géré par le Hub.
Il est possible de spécifier le nombre de thread associé à cette instance afin que le Hub tienne compte des écarts de puissance entre les moteurs.
Ceci afin de permettre de conserver des machines normal, tous en ajoutant à la demande des instances en *cloud* dont la puissance est inférieur.


unregister
``````````
L'api /unregister permet d'enlever un moteur du pool.




Le Hub d'accès
==============

Le Hub d'accès est le composant d'accès visible depuis l'extérieur. 
Il a les rôles suivant:

    - Répartition de charge
    - Génération des statistiques
    - Génération des flux de réponses dans le format demandé
    - Supervision général de l'état de la plateforme
    - Gestion de l'aspect *"élastique"*

Répartition de charge
---------------------
Le Hub d'accès se charge d'aiguiller les requêtes qu'elle reçoit sur la bonne instance du moteur de calcul selon l'algorithme *round-robin*.
Il doit en plus tenir compte de l'état des moteurs, c'est à dire que si l'un d'entre eux est en erreur, il est sorti du pool.




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

Les réponses en *Protocol buffer* ne subissent aucunes transformations, c'est le format utilisé pour la communication avec les moteurs qui est renvoyé.
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




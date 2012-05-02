Introduction
============

NAViTiA est une suite applicative complète permettant la conception, la gestion et la diffusion d'information voyageur multi-modale.

Nous présentons dans cette introduction les 4 niveaux suivants :

* Les fournisseurs d’information.
* Le noyau de communication, qui assure l’interface avec les systèmes partenaires. 
  Dans notre architecture, le noyau de communication est principalement composé des services FUSiO (données structurelles) 
  et AlerteTrafic (données événementielles).
* Le SIV qui assure la production des services applicatifs à destination des voyageurs et des partenaires. 
  Dans notre architecture, le SIV est principalement composé du hub NAViTiA.
* Les canaux d’accès à l’information.

Les interfaces de ces sous-systèmes sont claires et bien documentées. 

* Elles reposent sur des « web services », ce qui en garantit l’ouverture et la souplesse d’intégration
  pour intégrer une problématique locale spécifique par exemple.
* Elles s’appuient sur la normalisation Transmodel notamment pour ce qui est des interfaces 
  avec les systèmes des contributeurs ou de l’export du référentiel consolidé vers d’autres systèmes.
* Elles sont évolutives. L’approche en service hébergé facilite l’accès à de nouveaux services développés dans le cadre d’autres projets. 

.. warning::
   La mise à disposition d’interfaces automatisées avec un nombre toujours plus important d’autres systèmes est un point fort de notre solution.


Architecture
************

Organisation des modules
------------------------

Le schéma ci-dessous donne une vision d’ensemble de l’architecture fonctionnelle du système.
Elle permet notamment de visualiser de façon synthétique quelles sont les fonctions des principales composantes du système.

* FUSiO permet aux partenaires (exploitants ou autorités organisatrices) de fournir simplement leurs données théoriques. 
* Le cas échéant AlerteTrafic permet de recueillir les informations conjoncturelles de façon automatique 
  ou manuelle afin d’alimenter NAViTiA-SIV en informations conjoncturelles. Ce module permet également l’envoi de messages aux abonnés.
* Le hub NAViTiA s’appuie sur les modules FUSiO et AlerteTrafic et autorise différents modes de diffusion : 
  Internet, centres de relations clients, afficheurs…
* Ces modes de diffusion peuvent servir différentes communautés d’utilisateurs : voyageurs, télé-conseillers ou partenaires du projet.
 
.. aafig::
 :textual:
 
 /--------------------------------------------------------------------------------\
 |                 Outils de saisie des données partenaires                       |
 \--------------------------------------------------------------------------------/
 
 +--------+ +-----+ +------+ +------+ +------+ +---+  +-----+ +-------------------+
 |CHOUETTE| |OBiTi| |PEGASE| |RAPIDO| |HEURES| |...|  |SAEIV| |Interface de saisie|
 +--------+ +-----+ +------+ +------+ +------+ +---+  +-----+ +-------------------+
      |        |       |        |         |      |       |              |
      |        |       |        |         |      |       |              |
      v        v       v        v         v      v       v              v
 /--------------------------------------------------\  /--------------------------\
 |                       FUSIO                      |  |      AlerteTrafic        |
 \--------------------------------------------------/  \--------------------------/ 
                           |                                       |
                           |                                       |
                           v                                       v
 /--------------------------------------------------------------------------------\
 |                                Hub NAViTiA                                     |
 \--------------------------------------------------------------------------------/
      |          |         |      |       |         |           |
      |          |         |      |       |         |           |
      v          v         v      v       v         v           v
 /----------+/--------+/------+/-------+/---+/--------------+/------+
 |Afficheurs||  Site  || Site ||Serveur||   ||Grille horaire||      |
 |          ||        ||      ||       ||CRC||              ||Bornes|
 |dynamiques||internet||mobile|| vocal ||   ||   papier     ||      |
 +----------/+--------/+------/+-------/+---/+--------------/+------/

Ou plus clairement:
.. image:: _static/Archi_applicative_complete.png
 
Interface
---------

Les articulations présentés dans le schéma du chapitre 'Organisation des modules_' s’articulent 
autour d’interfaces techniques standardisées http/XML. Sur ces standards, la suite NAViTiA définie 
son propre langage d’échange afin d’échanger les informations entre les différents modules et avec 
les applications tierces. Ce langage d’échange est nommé "interface d’échange NAViTiA".

Cependant, afin d’assurer la compatibilité entre toute nouvelle version de la suite NAViTiA 
et les systèmes qui l’utilisent (média, application tierce…), il est possible de faire évoluer 
les différents modules sans modifier l’interface d’échange utilisée. 
Ainsi la mise en place d’une nouvelle version corrective NAViTiA est réalisable sans risques 
de perturbations sur les systèmes tiers qui composent votre système. 

* La suite NAViTiA est caractérisée par le numéro de version de ses modules.
* La mise en œuvre d’un système reposant sur la suite NAViTiA est caractérisée par la version d’interface d’échange sur laquelle le système repose.

.. warning::
   La mise en place d’un système NAViTiA complet nécessite une mise en cohérence de la version des modules qui le compose.

**Remarque**

les éventuelles nouvelles fonctionnalités proposées par toute nouvelle version de la suite NAViTiA 
restent invisibles tant que la version d’interface ne permet pas leur utilisation. Ainsi :

.. warning::
   La mise en place des informations perturbations dans la chaine NAViTiA nécessite une Version d’interface 1.11 au minimum sur les API concernées

Description des modules
***********************

FUSiO : module de description du référentiel théorique
------------------------------------------------------

Ce module permet de décrire :

* Le référentiel topologique de transport en commun.
* L’offre en horaires théoriques sur ce référentiel.
  Afin de définir le référentiel théorique complet, il met en place les procédures suivantes : 
* Récupération des données de chaque contributeur.
* Enrichissement de propriétés complémentaires sur ces données.
* Définition des correspondances entre chaque contributeur.
* Enrichissement sur le référentiel de transport :

  * Données géographique (adresse).
  * Données régionales (lieux remarquables, tourisme…).

Le module offre en sortie :

* La description complète du référentiel théorique. Il permet ainsi d’alimenter directement NAViTiA-SIV en "données prévues".
* Il permet également de fournir des exports au format TRIDENT.

Pour tout complément de description, voir le document de référence FUSiO "FUSiO_Document de référence_nnn.doc".

Alerte-Trafic : module de prise en compte des perturbations
-----------------------------------------------------------

Ce module permet :

* De décrire l’ensemble des perturbations sur le réseau de transport :

  * Récupération des perturbations automatiquement depuis un SAEIV.
  * Fabrications manuelles de perturbations grâce à un site dédié.
  * L’envoi d’alerte aux abonnés du réseau (mail ou SMS par exemple).
  * L’alimentation en données perturbée du module de diffusion NAViTiA-SIV.

Les perturbations sont définies par rapport au référentiel théorique. Il est donc nécessaire de mettre en place les modules FUSiO et NAViTiA-SIV sur le même référentiel du réseau de transport que le module Alerte-Trafic.

.. warning::
   La mise en place de l’information perturbée au sein d’un système d’information voyageur nécessite 
   que chaque élément de la suite NAViTiA (FUSiO, Alerte-trafic, NAViTiA-SIV…) soit:
   
   * Basé sur *le même référentiel de transport*
   * Ce référentiel utilisant des codes d’objets ("codes externes") *pérennes et uniques*
   
   Les modalités de mise en œuvre sont décrites dans le catalogue de service

Pour tout complément de description, voir le document de référence Alerte-Trafic "AlerteTrafic_Document de référence.doc".

Hub NAViTiA : module de fourniture d’information
------------------------------------------------

Ce module est chargé de la diffusion de l’information de l’offre en transport en commun. Le module est capable de mixer l’information théorique et l’information perturbée.

* Afin de bénéficier du temps réel et des informations conjoncturelles, ce module doit être installé avec son module Alerte-Trafic.
* Afin de bénéficier des calculs d’itinéraire piéton avancés, les données topographiques doivent être correctement renseignées dans FUSiO.
* Afin de bénéficier des statistiques avancées, le module nécessite l’utilisation du sous-module gwNAViTiA et son paramétrage à mettre en place conjointement entre Canal TP et le partenaire.

.. warning::
   Pour la mise en place de statistiques métiers (observatoire des demandes de déplacement), une description détaillée des besoins doit être fournie à Canal TP.

Pour tout complément de description, voir le document de référence NAViTiA-SIV "NAViTiA_Document de référence.doc".

Diffusion vers les médias
-------------------------

Média intégré EZ-Publish
++++++++++++++++++++++++

Cette intégration « type » permet de diffuser l’information sur différents médias :

* Site internet standard
* Site internet accessible
* Site mobile
* ...

Le module EZ-Publish permet également de gérer l’ensemble du contenu des rubriques annexe au module NAViTiA-SIV : 
information touristique, information sur les tarifs, sur les points de ventes...

Développement d’un média spécifique
+++++++++++++++++++++++++++++++++++

Le développement d’une application de mise en forme spécifique permet de redéfinir les médias cible :

* Panneaux afficheur.
* Guide horaire papier.
* Widget.
* Site internet spécifique.

Le développement d’une interface spécifique doit suivre les préconisations décries 
dans le document d’intégration "NAViTiA_Manuel_Integration" et être suivie dans le cadre d’un "projet d’intégration NAViTiA" par Canal TP.

Pré-requis
**********

Administration des données
--------------------------

La qualité des données qui alimentent le système impacte l’ensemble de la chaine. Il convient donc de procéder aux actions suivantes :

* Identification d’un administrateur.
* Formation de cet administrateur aux outils NAViTiA.
* Mise en cohérence des données.

  * Points d’arrêts géo-localisés.
  * Horaires mis à jour régulièrement.
  * Informations complémentaires conforment au message client.

Ouverture de compte
-------------------

Le projet type de mise en place d’une solution technique NAViTiA (hors mise en place de médias spécifiques) nécessitera :

* FUSiO

  * Définition des formats d’alimentation :
  
    * Utilisation du format d’échange standard NAViTiA.
    * Utilisation du format d’échange standard TRIDENT CHOUETTE.
    * Utilisation du format d’échange GoogleTransit.
    * Etude d’un connecteur spécifique si besoin.

* Alerte-trafic

  * Mise à disposition d’une plateforme de saisie manuelle des perturbations.
  * Mise en place du module d’alerte push (SMS ou Mail) si besoin.
  * Eventuellement :
  
    * Adaptation de l’application aux besoins spécifiques du projet (se rapporter à la documentation « AlerteTrafic_Document de référence ».
    * Etude d’un connecteur spécifique pour le module pull (alimentation en perturbation automatique) si besoin.
    * Etude d’un connecteur spécifique pour le module push (SMS ou Mail) si besoin.

* Hub NAViTIA
* Media de référence pour qualifier les données
* Mise à disposition des statistiques. Mise en place du module gwNAViTiA

Flux entre les services
-----------------------

Les modules FUSiO, NAViTiA et Alerte-Trafic sont hébergés par Canal TP. Leur mise à disposition est donc géré par Canal TP et transparente pour l’utilisateur.
Si l’application média n’est pas hébergée par Canal TP, il faut vérifier :

* L’application qui interroge le hub NAViTiA doit avoir accès à tout le domaine http://*.navitia.com
* La mise en œuvre d’une intégration spécifique du site de fabrication manuelle des informations perturbées (module Alerte-trafic/site de création des messages) nécessite un accès à NAViTiA-SIV.

Données
-------

* La mise à disposition de l’ensemble des horaires est nécessaire : NAViTiA ne déduit pas automatiquement de données horaires à partir d’un plan de réseau par exemple.
* Les points d’arrêts doivent être géo-localisés, sans quoi le système est bridé.
* Les données doivent être structurées de façon à alimenter correctement FUSiO. Se reporter au document « FUSiO_Document de référence.doc » pour toute information complémentaire concernant la structure des données.

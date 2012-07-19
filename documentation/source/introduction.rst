Introduction
============

**NAViTiA** est un **SIM** (système d'information multimodale). 
C'est une suite applicative complète permettant la conception, la gestion et la diffusion d'information voyageur multi-modale.

Nous présentons dans cette introduction les 4 niveaux suivants :

* Les fournisseurs d’information.
* Le noyau de communication, qui assure l’interface avec les systèmes partenaires. 
  Dans notre architecture, le noyau de communication est principalement composé des services FUSiO (données structurelles) 
  et AlerteTrafic (données événementielles).
* Le SIV qui assure la production des services applicatifs à destination des voyageurs et des partenaires. 
  Dans notre architecture, le SIV est principalement composé du hub NAViTiA.
* Les canaux d’accès à l’information.

Les interfaces de ces sous-systèmes sont claires et documentées. 

* Elles reposent sur des « web services », ce qui en garantit l’ouverture et la souplesse d’intégration
  pour intégrer une problématique locale spécifique par exemple.
* Elles s’appuient sur la normalisation Transmodel notamment pour ce qui est des interfaces 
  avec les systèmes des contributeurs ou de l’export du référentiel consolidé vers d’autres systèmes.
* Elles sont évolutives. L’approche en service hébergé facilite l’accès à de nouveaux services développés dans le cadre d’autres projets. 

.. warning::
   La mise à disposition d’interfaces automatisées avec un nombre toujours plus important d’autres systèmes est un point fort de notre solution.


Présentation générale
*********************

La suite NAViTiA est composée des 3 modules fonctionnels principaux suivants:

* **FUSiO** permet de récupérer les données de transport structurelles des différents partenaires (exploitants ou autorités organisatrices) 
* Le cas échéant **AlerteTrafic** permet de recueillir les informations conjoncturelles de façon automatique 
  ou manuelle afin d’alimenter le hub NAViTiA en informations conjoncturelles. Ce module permet également l’envoi de messages aux abonnés.
* Le **hub NAViTiA** s’appuie sur les 2 précédents modules FUSiO et AlerteTrafic et permet de diffuser une information cohérente sur l'ensemble des médias qui composent le SIM: 
  Internet, centres de relations clients, afficheurs…
* Ces modes de diffusion peuvent servir différentes communautés d’utilisateurs : voyageurs, télé-conseillers ou partenaires du projet.

Le schéma ci-dessous donne une vision d’ensemble de l’architecture fonctionnelle du système.
Elle permet notamment de visualiser de façon synthétique quelles sont les fonctions des principales composantes du système.

.. image:: _static/Archi_applicative_complete.png
   :width: 800px


FUSiO : module de description du référentiel théorique
******************************************************

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
***********************************************************

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
************************************************

Ce module est chargé de la diffusion de l’information de l’offre en transport en commun. Le module est capable de mixer l’information théorique et l’information perturbée.

* Afin de bénéficier du temps réel et des informations conjoncturelles, ce module doit être installé avec son module Alerte-Trafic.
* Afin de bénéficier des calculs d’itinéraire piéton avancés, les données topographiques doivent être correctement renseignées dans FUSiO.
* Afin de bénéficier des statistiques avancées, le module nécessite l’utilisation du sous-module gwNAViTiA et son paramétrage à mettre en place conjointement entre Canal TP et le partenaire.

.. warning::
   Pour la mise en place de statistiques métiers (observatoire des demandes de déplacement), une description détaillée des besoins doit être fournie à Canal TP.

Les chapitres suivant décrivent l'intégralité des fonctions de ce module.

Diffusion vers les médias
*************************

Média intégré Canal TP
++++++++++++++++++++++

Cette intégration « type » permet de diffuser l’information sur différents médias :

* Site internet standard
* Site internet accessible
* Site mobile
* ...

L'intégration proposée par Canal TP permet également de gérer l’ensemble du contenu des rubriques annexes au hub NAViTiA : 
information touristique, information sur les tarifs, sur les points de ventes...

Développement d’un média spécifique
+++++++++++++++++++++++++++++++++++

Le développement d’une application de mise en forme spécifique permet de redéfinir les médias cible :

* Panneaux afficheur.
* Guide horaire papier.
* Widget.
* Site internet spécifique.

Le développement d’une interface spécifique doit suivre les préconisations décries 
dans la suite de cette documentation et être suivie dans le cadre d’un "projet d’intégration NAViTiA" par Canal TP.

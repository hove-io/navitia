Introduction
============

NAViTiA est une suite applicative complète permettant la conception, la gestion et la diffusion d'information voyageur multi-modale.

Nous présentons dans cette introduction les 4 niveaux suivants :

* Les fournisseurs d’information.
* Le noyau de communication, qui assure l’interface avec les systèmes partenaires. Dans notre architecture, le noyau de communication est principalement composé des services FUSiO (données structurelles) et AlerteTrafic (données événementielles).
* Le SIV qui assure la production des services applicatifs à destination des voyageurs et des partenaires. Dans notre architecture, le SIV est principalement composé du hub NAViTiA.
* Les canaux d’accès à l’information.

Les interfaces de ces sous-systèmes sont claires et bien documentées. 

* Elles reposent sur des « web services », ce qui en garantit l’ouverture et la souplesse d’intégration pour intégrer une problématique locale spécifique par exemple.
* Elles s’appuient sur la normalisation Transmodel notamment pour ce qui est des interfaces avec les systèmes des contributeurs ou de l’export du référentiel consolidé vers d’autres systèmes.
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
* Le cas échéant AlerteTrafic permet de recueillir les informations conjoncturelles de façon automatique ou manuelle afin d’alimenter NAViTiA-SIV en informations conjoncturelles. Ce module permet également l’envoi de messages aux abonnés.
* NAViTiA-Core s’appuie sur FUSiO et AlerteTrafic et autorise différents modes de diffusion : Internet, centres de relations clients, afficheurs…
* Ces modes de diffusion peuvent servir différentes communautés d’utilisateurs : voyageurs, télé-conseillers ou partenaires du projet.
 
.. warning::
   INSERER L'IMAGE FUSIO/AT/NAVITIA

.. aafig::
 :textual:
 
 /--------------------------------------------------------------------------------\
 |                Outils de saisie des données partenaires                        |
 \--------------------------------------------------------------------------------/
 +--------+ +-----+ +------+ +------+ +------+ +---+  +-----+ +-------------------+
 |CHOUETTE| |OBiTi| |PEGASE| |RAPIDO| |HEURES| |...|  |SAEIV| |Interface de saisie|
 +--------+ +-----+ +------+ +------+ +------+ +---+  +-----+ +-------------------+
      |        |       |        |        |       |       |                |
      |        |       |        |        |       |       |                |
      v        v       v        v        v       v       v                v
 +-------------------------------------------------+  +---------------------------+
 |                   FUSIO                         |  |    AlerteTrafic           |
 +-------------------------------------------------+  +---------------------------+ 
                         |                                                |
                         |                                                |
                         v                                                v
 +--------------------------------------------------------------------------------+
 |                                Hub NAViTiA                                     |
 +--------------------------------------------------------------------------------+
       |            |           |        |         |           |             |
       |            |           |        |         |           |             |
       v            v           v        v         v           v             v
 /------------\/----------\/--------\/---------\/-----\/----------------\/--------\
 | Afficheurs ||   Site   ||  Site  || Serveur ||     || Grille horaire ||        |
 |            ||          ||        ||         || CRC ||                || Bornes |
 | dynamiques || internet || mobile ||  vocal  ||     ||    papier      ||        |
 \------------/\----------/\--------/\---------/\-----/\----------------/\--------/


Interface
---------
Les articulations présentés dans le schéma du chapitre "2.1 -  Organisation des modules" s’articulent autour d’interfaces techniques standardisées http/XML. Sur ces standards, la suite NAViTiA définie son propre langage d’échange afin d’échanger les informations entre les différents modules et avec les applications tierces. Ce langage d’échange est nommé "interface d’échange NAViTiA".

Cependant, afin d’assurer la compatibilité entre toute nouvelle version de la suite NAViTiA et les systèmes qui l’utilisent (média, application tierce…), il est possible de faire évoluer les différents modules sans modifier l’interface d’échange utilisée. Ainsi la mise en place d’une nouvelle version corrective NAViTiA est réalisable sans risques de perturbations sur les systèmes tiers qui composent votre système. 

* La suite NAViTiA est caractérisée par le numéro de version de ses modules.
* La mise en œuvre d’un système reposant sur la suite NAViTiA est caractérisée par la version d’interface d’échange sur laquelle le système repose.

.. warning::
   La mise en place d’un système NAViTiA complet nécessite une mise en cohérence de la version des modules qui le compose.

*Remarque*

les éventuelles nouvelles fonctionnalités proposées par toute nouvelle version de la suite NAViTiA restent invisibles tant que la version d’interface ne permet pas leur utilisation. Ainsi :
.. warning::
   La mise en place des informations perturbations dans la chaine NAViTiA nécessite une Version d’interface 1.11 au minimum sur les API concernées

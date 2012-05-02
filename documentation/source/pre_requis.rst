Pré-requis
==========

Administration des données
**************************

La qualité des données qui alimentent le système impacte l’ensemble de la chaine. Il convient donc de procéder aux actions suivantes :

* Identification d’un administrateur.
* Formation de cet administrateur aux outils NAViTiA.
* Mise en cohérence *et en qualité* des données.

  * Les données de transport en commun doivent être structurées de façon à alimenter correctement FUSiO. 
  
    * Points d’arrêts géo-localisés.
    * Respect le plus strict possible de la norme TRANSMODEL.
      Se reporter au document "FUSiO_Document de référence.doc" pour toute information complémentaire concernant la structure des données.
    * *Horaires mis à jour régulièrement*. La mise à disposition régulière de l’ensemble des horaires est nécessaire : 
      NAViTiA ne déduit pas automatiquement de données horaires à partir d’un plan de réseau par exemple.

  * Informations complémentaires conforment au message client.


Ouverture de compte
*******************

Le projet type de mise en place d’une solution technique NAViTiA (hors mise en place de médias spécifiques) nécessitera :

* FUSiO

  * Définition des formats d’alimentation :
  
    * Utilisation du format d’échange standard NAViTiA.
    * Utilisation du format d’échange standard TRIDENT CHOUETTE.
    * Utilisation du format d’échange GoogleTransit.
    * Etude d’un connecteur spécifique si besoin.

  * Choix de la couverture géographique des adresses (filaire de voirie)

* Alerte-trafic

  * Mise à disposition d’une plateforme de saisie manuelle des perturbations.
  * Mise en place du module d’alerte push (SMS ou Mail) si besoin.
  * Eventuellement :
  
    * Adaptation de l’application aux besoins spécifiques du projet (se rapporter à la documentation « AlerteTrafic_Document de référence ».
    * Etude d’un connecteur spécifique pour le module pull (alimentation en perturbation automatique) si besoin.
    * Etude d’un connecteur spécifique pour le module push (SMS ou Mail) si besoin.
    * Contractualisation éventuelle avec un prestataire d'envoi de mail ou de SMS

* Hub NAViTIA

  * Définition des regroupements de média pour la traçabilité des statistiques
  * Choix de la valeur des paramètres mis à disposition des internautes (vitesse de marche à pied, distance d'accroche...)
  

* Media de référence pour qualifier les données


Flux entre les services
***********************

Les modules FUSiO, NAViTiA et Alerte-Trafic sont hébergés par Canal TP. Leur mise à disposition est donc géré par Canal TP et transparente pour l’utilisateur.
Si l’application média n’est pas hébergée par Canal TP, il faut vérifier :

* L’application qui interroge le hub NAViTiA doit avoir accès à tout le domaine http://*.navitia.com
* La mise en œuvre d’une intégration spécifique du site de fabrication manuelle des informations perturbées (module Alerte-trafic/site de création des messages) nécessite un accès à NAViTiA-SIV.

VPtranslate
============

Ce module permet de traduire une suite de date exhaustive (régime de circulation) en phrase intelligible:

* Transforme "Circule le 06/04,13/04,20/04,27/04"
* En "circule les lundi du 06/04/ au 03/05"

Cette fonction peut être utilisable par exemple conjointement avec les services suivants :
* Recherche des propriétés d’une desserte : 
  en plus des propriétés d’arrêt et d’horaire desservis, de véhicule utilisé, 
  la consultation de desserte retourne la condition de service. Il est alors possible 
  d’appeler l’API de traduction de régime pour signifier de façon intelligible les jours de circulation.
* En calcul multimodal : 
  NAViTiA calcule la chaine binaire de condition de circulation globale 
  à chaque itinéraire proposé en faisant l’intersection des conditions de service utilisées. 
  Il est donc possible d’appeler ensuite cette fonction afin de fournir au client une réponse 
  multimodale sur l’information de circulation : itinéraire circulant du lundi au vendredi sauf mercredi, du... au...


Paramètres d'entrée
********************************
à partir des paramètres suivants :

* La date de début de circulation : circule à partir du …
* Une chaine de 0 et 1 représentant la condition de circulation : 1111100111110011111001111100


Transformation/calcul du service
********************************

Module utilisé
--------------


Fonctions internes
------------------

Principe de fonctionnement
++++++++++++++++++++++++++

Dans la chaine représentant la condition de circulation ("111100111100111100") 
* NAViTiA recherche les motifs les plus représentatifs (chaque motif est composé de 7 jours).
* Pour chaque motif (du plus représenté au moins représenté) :

  * le motif est associé à n plages de date « du… au … »,
  * et à un libellé associé au motif. Par exemple : 1111100 -> libellé "circule du lundi au vendredi".

Quelque soit le paramétrage, la réponse du traducteur de régime sera juste, mais pas forcément optimale selon des critères subjectifs.

Les paramétrages (voir ci-dessous) permettent de corriger certaines réponses. 
L’intégration dans le média permet également de présenter au mieux la réponse par rapport au client visé.

Paramétrages
++++++++++++

Les paramétrages permettent d’influer sur la réponse. Ainsi pour une demande du type :

* Date de début : lundi 01/06/2009.
* Chaine de régime : "1101100".
* 2 résultats pourront être proposés :

  * Soit :
    Du lundi au vendredi du 01/06/2009 au 05/06/2009, sauf le mercredi 03/06/2009.
  * Soit :
    Du lundi au vendredi sauf mercredi du 01/06/2009 au 05/06/2009.
  * Soit :
    Les lundi, mardi, jeudi et vendredi du 01/06/2009 au 05/06/2009.
  * Selon les paramétrages de présentation mis en place.

Il n’est possible de paramétrer qu’une seule liste de dates pour les jours fériés, basée sur les jours fériés français. 
Pour les autres pays, il est nécessaire de modifier le fichier de paramétrage et 
de désactiver la génération automatique des jours fériés. 
La fonction de calcul automatique des jours fériés retourne :

* Le 1er janvier
* Le 1er mai
* Le 8 mai
* Le 14 juillet
* Le 15 août
* Le 1er novembre
* Le 11 novembre
* Le 25 décembre
* Le lundi de pâques
* Le jeudi de l’ascension
* Le lundi de pentecôte

Exemple
+++++++

* Si "Date=14/09/2009", et que "Pattern=11011001101100"
* Alors la demande porte sur : traduire la liste des jours actifs suivant :

  * 14/09/2009
  * 15/09/2009
  * 17/09/2009
  * 18/09/2009
  * 21/09/2009
  * 22/09/2009
  * 24/09/2009
  * 25/09/2009

* En effet :

.. image:: ../_static/VPTranslator.png

* La réponse NAViTiA sera du type "circule du Lundi au vendredi, sauf mercredi, du 14/09/2009 au 25/09/2009"


UseCase
-------

Format de sortie
****************

Exemple d'utilisation et module de démonstration
************************************************

Tests unitaires
***************

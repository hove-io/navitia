NTFS - Extension tarifaire
======================================

# Introduction

Ce document vient compléter les spécififcations [ntfs_fr](./ntfs_fr.md) pour y ajouter la gestion des tarifs.
Attention, le formatage de ces fichiers est différent.

Bien qu'actuellement utilisée, cette extension est dépréciée :
une future version du format NTFS inclura une gestion des tarifs différente de celle-ci.

# Format des données de tarification

Les données tarifaires sont ajoutées directement dans l'archive ZIP contenant les données NTFS.
Les fichiers sont formatés de la manière suivante :

* Chaque fichier est un fichier **CSV**
* Encodage des fichiers : tous les fichiers sont encodés en **UTF-8**
* Le séparateur de champ est le **";"**
* Les colonnes sont toutes obligatoires et leur ordre doit etre respecté
* Attention, 2 fichiers sont **avec** entête et un autre **sans** entête
* La dernière ligne des fichiers doit se terminer par un retour à la ligne

# Liste des fichiers de tarification
Fichier | Contrainte | Commentaire
--- | --- | ---
prices.csv | Optionnel | Liste des tickets et le prix appliqué (plein tarif uniquement)
fares.csv | Optionnel | Spécifications tarifaires des tickets déclarés dans le fichier **prices.csv**
od_fares.csv | Optionnel | Liste des tickets dont le tarif est définit par une Origine-Destination

## prices.csv
Ce fichier contient le prix (plein tarif uniquement) de chaque ticket. Les conditions d'utilisation de ce ticket sont
précisées soit dans le fichier **fares.csv** soit dans le fichier **od_fares.csv**.
Il est possible de préciser plusieurs tarifs pour un même billet avec des dates de validité différentes (pour gérer un changement de tarif).

> **Attention : Ce fichier est _SANS_ ligne d'entête.**

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
"clef de ticket" | chaine unique | Requis | ID tarif (lien avec prices.csv)
"date de début de validité" | YYYYMMDD | Requis | Date de début de validité du tarif (inclue)
"date de fin de validité" | YYYYMMDD | Requis | Date de fin de validité du tarif (exclue : le tarif n'est pas appliqué ce jour)
"prix" | Entier | Requis | Valeur en centimes d'euro
"name" | chaine | Requis | Libellé du billet
"champ ignoré" | chaine | Requis |
"commentaire" | chaine | Requis | commentaire
"devise" | chaine | Optionnel | Devise (par défaut: euro)

## od_fares.csv
Ce fichier contient la description des tickets dont le tarif est définit par une Origine-Destination.

> **Attention : Ce fichier est _AVEC_ ligne d'entête.**

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
Origin ID | chaine (1) | Requis | Contient soit une zone tarifaire soit une URI de zone d'arrêt ou de mode. Voir ci-dessous.
Origin name | chaine | Optionnel | Libellé de la zone d'arrêt de départ ou du mode considéré. Ce champ n'est pas lu et sert à la lisibilité du fichier.
Origin mode | chaine | Requis | Type d'objet d'origine pour ce ticket : "zone", "stop" ou "mode".
Destination ID | chaine (1) | Requis | Contient soit une zone tarifaire soit une URI de zone d'arrêt ou de mode. Voir ci-dessous.
Destination name | chaine | Optionnel | Libellé de la zone d'arrêt de départ ou du mode considéré. Ce champ n'est pas lu et sert à la lisibilité du fichier.
Destination mode | chaine | Requis | Type d'objet de destination pour ce ticket : "zone", "stop" ou "mode".
ticket_id | chaine | Requis | Identifiant du ticket dont le prix est référencé dans le fichier price.csv

**(1) Fonctionnement de la description de l'origine (règle identique pour la destination) :**
* Si le champ **Origin mode** contient la valeur **zone** : le champ **Origin ID** contient une zone tarifaire telle que décrite dans le fichier **stops.txt** dans le champ *fare_zone_id*.
Par exemple : pour définir que le tarif par OD fonctionne sur toutes les gares de Paris (qui est en zone 1), il suffit d'indiquer 1 dans la colonne Origin_ID.
* Si le champ **Origin mode** contient la valeur **stop** : le champ **Origin ID** contient une URI de zone d'arrêt (champ **stop_id** de la zone d'arrêt avec le préfixe _"stop_area:"_)
* Si le champ **Origin mode** contient la valeur **mode** : le champ **Origin ID** contient une URI de mode physique (champ **physical_mode_id** du mode physique avec le préfixe _"physical_mode:"_)
Par exemple : si le tarif par OD permet de partir de n'importe quelle station de métro, indiquer "physical_mode:metro" dans la colonne Origin_ID

## fares.csv (optionnel)
Ce fichier contient les spécifications tarifaires des tickets déclarés dans le fichier **prices.csv**.

> **Attention : Ce fichier est _AVEC_ ligne d'entête.**

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
"avant changement" | chaine | Requis | voir § **État avant et après changement** ci-dessous
"après changement" | chaine | Requis | voir § **État avant et après changement** ci-dessous
"début trajet" | chaine | Requis | voir § **Conditions de début et fin de trajet** ci-dessous
"fin trajet" | chaine | Requis | voir § **Conditions de début et fin de trajet** ci-dessous
"condition globale" | chaine | Requis | Condition globale d'utilisation du ticket (vide, "nothing", "exclusive", "with_changes" ou "symetric")
"clef ticket" | chaine | Requis | ID tarif (lien avec prices.csv)

**État avant et aprés changement :**
Un état avant un changement (ou après un changement) est décrit par un objet TC, selon l'une des possibilités suivantes :
* Description par un mode physique : Indiquer une URI de mode physique (champ **physical_mode_id** du mode physique avec le préfixe _"physical_mode:"_)
Par exemple : indiquer "mode=physical_mode:metro" pour indiquer que le voyageur se trouve dans le métro avant le changement.
* Description par un réseau : Indiquer une URI de réseau (champ **network_id** du réseau avec le préfixe _"network:"_)
Par exemple, indiquer "network=network:Filbleu" pour indiquer que le voyageur se trouve sur le réseau Filbleu avant le changement
* Description par une ligne : Indiquer une URI de ligne (champ **line_id** de la ligne avec le préfixe _"line:"_)
* Indiquer **"\*"** pour ne pas fournir de contrainte particulière

**Conditions de début et fin de trajet :**
Ces deux champs permettent d'ajouter des conditions au départ (ou à l'arrivée) du trajet :
* Restriction à une zone tarifaire : préciser dans le champ la valeur _"zone=[fare_zone_id]"_
Par exemple : si le voyageur est sur la commune de Paris, on peut indiquer _"zone=1"_ afin de créer une règle applicable uniquement depuis Paris.
* Restriction à une zone d'arrêt : préciser dans le champ la valeur _"stoparea=[stop_area_id]"_
(champ **stop_id** de l'arrêt ayant pour **location_type** la valeur **1** avec le préfixe _"stoparea:"_)
Par exemple : si le voyageur est sur la zone d'arrêt sa:Orsay , on peut indiquer _"stoparea=sa:Orsay"_
afin de créer une règle applicable uniquement depuis Orsay.
On a donc une autre modélisation des OD, qui permet de combiner avec d'autres choses :
Par exemple : `*;network=network:SNCF;stoparea=stop_area:SNC:Troyes;stoparea=stop_area:SNC:Reims;;1`
appliquera le ticket "1" uniquement pour les sections qui font Troyes-Reims sur le réseau SNCF.
* Restriction à une durée de voyage : préciser dans le champ la valeur _"duration<[nombre de minutes]"_.
Par exemple : indiquer _"duration<60"_ pour préciser que le ticket n'est encore valable que si le voyageur l'utilise depuis moins de 60 minutes.
* Restriction à un nombre de correspondances : préciser dans le champ la valeur _"nb_changes<[nombre de correspondances]"_.
Par exemple : indiquer _"nb_changes<2"_ pour préciser que le ticket n'est utilisable que pour une correspondance.

**Condition globale :**
Ce champ précise la condition globale d'utilisation du ticket :
* vide ou "nothing" : ce ticket n'a aucune condition spécifique
* "exclusive": correspond à un ticket à tarification spéciale sans correspondance (Noctilien, navettes aéroport…)
* "with_changes": correspond à un billet de type Origine-Destination permettant tous les changements.
Par exemple:
```
*;network=network:SNCF;;;with_changes;
network=network:SNCF;network=network:SNCF;;;with_changes;
```
permet d'autoriser les correspondances en gardant le même ticket sur le réseau SNCF (donc d'avoir des tickets OD qui fonctionnent aussi avec correspondance).
* "symetric": spécifie que ce tarif est également disponible en intervertissant l'état de début et de fin (par exemple : s'il est possible de changer du bus au tramway, la réciproque est vraie)

**Fonctionnement grossier :**
_warning : description sous réserve, non-contractuelle_
Pour chaque ligne de ce fichier, le ticket est applicable avec une vérification faite au moment d'emprunter
une nouvelle section de transport.
Avant d'appliquer le ticket, on va donc vérifier la validité de :

* l'état avant changement (ticket déjà présent, mode précédent, réseau précédent, ligne précédente)
* l'état après changement (mode, réseau, ligne)
* la condition sur le debut de la section à emprunter (zone tarifaire, zone d'arrêt, durée, nombre de changements, ticket)
* la condition sur la fin de la section à emprunter (zone tarifaire, zone d'arrêt, durée)
* la condition globale

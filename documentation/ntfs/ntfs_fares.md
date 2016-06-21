Extension tarifaire NTFS version 0.6
================

# Introduction

Ce document vient compléter les spécififcations [ntfs_0.6](./ntfs_0.6.md) pour y ajouter la gestion des tarifs.
Attention, le formatage de ces fichiers est différent.

# Format des données

Les données tarifaires sont ajoutées directement dans l'archive ZIP contenant les données NTFS.
Les fichiers sont formatées de la manière suivante :

* Fichier csv (.csv)
* Séparateur point-virgule ";"
* Les colonnes sont toutes obligatoire et leur ordre doivent etre respectée.
* Avec ou Sans entete

## Liste des fichiers de tarification
Fichier | Contrainte | Commentaire
--- | --- | ---
fare.csv | Optionnel | Tarification : Ce fichier contient les configuration des transitions
prices.csv | Optionnel | Prix des tickets : Ce fichier contient les prix appliqués (plein tarif)
od_fares.csv | Optionnel | Liste des billets Origine-Destination



### fare.csv (optionnel)
Tarification : Ce fichier contient les configuration des transitions
--> fichier CSV, séparé par des points-virgules, encodé en UTF-8, SANS ligne d'entête.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
"avant changement" | chaine | Requis | état avant changement (mode ou network)
" après changement" | chaine | Requis | état aprés changement (mode ou network)
" début trajet" | chaine | Requis | condition du début de trajet. (zone, duration, nb_change)
" fin trajet" | chaine | Requis | condition de fin de trajet. (zone, duration, nb_change)
" condition globale" | chaine | Requis | condition globale du trajet (exclusive, with_changes ou symetric)
"clef ticket" | chaine | Requis | ID tarif (lien avec prices.csv)

#### état avant et aprés changement.
* "mode": l'état avant changement correspond à un physical_mode. Par exemple, indiquer "mode=physical_mode:metro" pour indiquer que le voyageur se trouve dans le métro avant le changement.
* "network": l'état avant changement correspond à un réseau. Par exemple, indiquer "network=network:Filbleu" pour indiquer que le voyageur se trouve sur le réseau Filbleu avant le changement

#### conditions de début et fin de trajet
* "zone": le voyageur se trouve dans une certaine zone tarifaire (fare_zone). Par exemple, s'il est sur la commune de Paris, on peut indiquer "zone=1" afin de créer une règle applicable uniquement depuis Paris.
* "duration": (en minute) indique une règle applicable si le voyageur est au sein d'un système tarifaire depuis moins de "duration".
* "nb_changes": indique une règle applicable si le voyageur est au sein d'un système tarifaire depuis moins de "duration"

#### condition globale
* "exclusive": correspond à un trajet à tarification spéciale sans correspondance (Noctilien, navettes aéroport…)
* "with_changes": correspond à l'achat d'un billet de type Origine-Destination permettant tous les changements
* "symetric": créé la même transition en permutant l'état de début et de fin (s'il est possible de changer du bus au tramway, la réciproque est vraie)

### prices.csv
Prix des tickets : Ce fichier contient les prix appliqués (plein tarif)
--> fichier CSV, séparé par des points-virgules, encodé en UTF-8, SANS ligne d'entête.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
"clef de ticket" | chaine unique | Requis | ID tarif (lien avec prices.csv)
"date de début de validité" | YYYYMMDD | Requis |
"date de fin de validité" | YYYYMMDD | Requis |
"prix" | Entier | Requis | la valeur en centimes d'euro
"name" | chaine | Requis | le libellé du billet

### od_fares.csv
Liste des billets Origine-Destination
--> fichier CSV, séparé par des points-virgules, encodé en UTF-8, AVEC ligne d'entête.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
Origin ID | chaine | Requis | URI navitia 2 du départ
Origin name | chaine | Requis | non lu (debug)
Origin mode | chaine | Requis | zone, stop ou mode
Destination ID | chaine | Requis | URI navitia 2 de la destination
Destination name | chaine | Requis | non lu (debug)
Destination mode | chaine | Requis | zone, stop ou mode
ticket_id | chaine | Requis | code prix référencé dans le fichier price.csv

#### Origin / Destination mode
* "zone": la colonne départ correspond à une zone tarifaire. Indiquer ici l'attribut fare_zone. Par exemple, pour définir que le tarif par OD fonctionne sur toutes les gares de Paris (qui est en zone 1), il suffit d'indiquer 1 dans la colonne Origin_ID et "zone" dans cette colonne.
* "stop": la colonne départ correspond à une zone d'arrêt (donc Origin_ID = stop_area.URI). Correspond au cas le plus classique
* "mode": la colonne départ correspond à un physical_mode. Par exemple si le tarif par OD permet de rejoindre n'importe quelle station de métro, indiquer "physical_mode:metro" dans la colonne Origin_ID et "mode" dans cette colonne

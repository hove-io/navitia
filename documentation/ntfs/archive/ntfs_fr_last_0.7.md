NTFS version 0.7
================

# Introduction

NTFS est le format d'échange de données de Kisio Digital : Navitia Transit Feed Specification.
Il a pour objectif de remplacer le format csv/Fusio afin de combler ses lacunes et permettre la gestion de tout type de données dans un seul format (horaires, TAD, etc.).

Ce nouveau format s'inspire très fortement du format GTFS (https://developers.google.com/transit/gtfs/reference?hl=fr-FR), tout en l'adaptant afin de permettre une description des données la plus exhaustive possible. A ce titre, le format est amené à évoluer (voir le [changelog](./ntfs_changelog.md) ).

# Format des données

Les données sont formatées de la manière suivante :

* Les fichiers sont compressées dans un fichier **ZIP**
* Encodage des fichiers : tous les fichiers sont encodés en **UTF-8**
* Chaque fichier est un fichier **CSV** contenant une ligne d'entête : http://tools.ietf.org/html/rfc4180
* Le séparateur de champ est la **","**
* Le système de coordonnées est le **WGS84**
* Le séparateur décimal est le **"."**
* Le format de date est **YYYYMMDD**
* Le format de l'heure est **HH:MM:SS**
* Le formatage des **URL** doit respecter le format du **W3C** : http://www.w3.org/Addressing/URL/4_URI_Recommentations.html
* Les fuseaux horaires http://en.wikipedia.org/wiki/List_of_tz_zones
* Les langues indiquées doivent respecter la norme **ISO 639-2** : http://www.loc.gov/standards/iso639-2/php/code_list.php
* Les couleurs sont spécifiées en RVB hexadécimal (par exemple **00FFFF**)
* Les géométries sont décrites en utilisant le format **WKT** : http://fr.wikipedia.org/wiki/Well-known_text
* Les identifiants des objets ne doivent pas contenir le type de l'objet. Ce dernier sera ajouté directement dans l'API navitia

# Liste des fichiers du format
## Fichiers spéciaux
    Ces fichiers permettent de décrire précisément d'où viennent les données dans le cas d'un référentiel agrégeant plusieurs sources de données. La notion de _contributor_ correspond à une source de données (un exploitant peut nécessiter l'utilisation de plusieurs contributeurs). La notion de _dataset_ correspond à un jeu de données provenant d'un contributeur.
    Le fichier _trips.txt_ fait une référence à _dataset_id_ afin de lier une circulation à sa donnée source.

Fichier | Contrainte | Commentaire
--- | --- | ---
contributors.txt | Requis | Ce fichier contient les contributeurs.
datasets.txt | Requis | Ce fichier contient les sources de données d'un contributeur.

## Fichiers de base
Fichier | Contrainte | Commentaire
--- | --- | ---
feed_infos.txt | Requis | Ce fichier contient des informations complémentaires sur la plage de validité des données, le fournisseur ou toute autre information complémentaires.
networks.txt | Requis | Ce fichier contient la description des différents réseaux.
commercial_modes.txt | Requis | Ce fichier contient les modes commerciaux (Mode NAViTiA 1)
companies.txt | Requis | Ce fichier contient les compagnies
lines.txt | Requis | Ce fichier contient les lignes
physical_modes.txt  | Requis | Ce fichier contient les modes physiques (ModeType NAViTiA 1)
routes.txt | Requis | Ce fichier contient les parcours
stop_times.txt | Requis | Ce fichier contient les horaires
stops.txt | Requis | Ce fichier contient les arrêts
trips.txt | Requis | Ce fichier contient les circulations
calendar.txt | Requis | Ce fichier contient les jours de fonctionnement
calendar_dates.txt | Optionnel | Ce fichier contient les exceptions sur les jours de fonctionnement décrits dans le fichier calendar.txt
comments.txt | Optionnel | Ce fichier contient les commentaires
comment_links.txt | Optionnel | Ce fichier contient les relations entre chaque commentaire et les objets du référentiels associés

## Fichiers complémentaires (hors calendriers par période)
Fichier | Contrainte | Commentaire
--- | --- | ---
frequencies.txt | Optionnel | Ce fichier contient les propriétés des fréquences
equipments.txt  | Optionnel | Ce fichier contient les propriétés (notamment l’accessibilité) pour les arrêts  et les correspondances
transfers.txt | Optionnel | Ce fichier contient les déclarations des correspondances
trip_properties.txt | Optionnel | Ce fichier contient l’accessibilité au niveau des circulations
geometries.txt | Optionnel | Ce fichier contient la représentation spatiale d'une géometrie au format Well Known Text (WKT). Ces géométries sont référencées dans les fichiers lines.txt, routes.txt, trips.txt.
object_properties.txt | Optionnel | Ce fichier contient la description des propriétés complémentaires sur les différents objets du référentiel.
object_codes.txt | Optionnel | Ce fichier contient la liste des codes d'identification complémentaires dans les systèmes externes des différents objets du référentiel.
admin_stations.txt | Optionnel | Ce fichier contient la liste des arrêts d'accroche des communes pour les itinéraires au départ ou à l'arrivée d'une commune
line_groups.txt | Optionnel | Ce fichier contient la définition de groupes de lignes
line_group_links.txt | Optionnel | Ce fichier contient la liaison entre un groupe de ligne et la liste des lignes qui le compose

## Fichiers des calendriers par période
Fichier | Contrainte | Commentaire
--- | --- | ---
grid_calendars.txt | Optionnel |  Ce fichier contient les jours de fonctionnement des calendriers
grid_exception_dates.txt | Optionnel | Ce fichier contient les exceptions sur les jours de fonctionnement des calendriers
grid_periods.txt | Optionnel | Ce fichier contient les périodes des calendriers
grid_rel_calendar_line.txt | Optionnel | Ce fichier contient les liens entre les lignes et ces calendriers

# Description des fichiers
### networks.txt (requis)
Ce fichier contient la description des différents réseaux.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
network_id | chaine | Requis | Identifiant unique et pérenne du réseau
network_name | chaine | Requis | Nom du réseau
network_url | chaine | Optionnel | Lien vers le site institutionnel
network_timezone | chaine | Optionnel |
network_lang | chaine | Optionnel |
network_phone | chaine | Optionnel | Numéro de téléphone de contact
network_address | chaine | Optionnel | Adresse du réseau.
network_sort_order | entier | Optionnel | Ordre de trie des réseaux, les plus petit sont en premier.

### calendar.txt (requis)
Ce fichier décrit les périodes de circulation associés aux trips.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
service_id | chaine | Requis | Identifiant du calendrier de circulation
monday | entier | Requis | (1)
tuesday | entier | Requis | (1)
wednesday | entier | Requis | (1)
thursday | entier | Requis | (1)
friday | entier | Requis | (1)
saturday | entier | Requis | (1)
sunday | entier | Requis | (1)
start_date | date | Requis | Début du calendrier de circulation
end_date | date | Requis | Fin du calendrier de circulation

(1) Les valeurs possibles sont :

* 0 - Ne circule pas ce jour
* 1 - Circule ce jour

### calendar_dates.txt (optionnel)
Ce fichier décrit des exceptions aux calendriers définis dans le fichier `calendar.txt`. Pour faciliter la description de calendriers pour des circulations très ponctuelles, il est possible de définir un calendrier en n'utilisant que le fichier `calendar_dates.txt`. De ce fait, le `service_id` ne sera pas présant dans le fichier `calendar.txt`.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
service_id | chaine | Requis | Identifiant du calendrier de circulation
date | date | Requis | Date de l'exception
exception_type | entier | Requis | (1)

(1) Les valeurs possibles sont :

* 1 - Le service est ajouté pour cette date
* 2 - Le service est supprimé pour cette date

### comments.txt (optionnel)

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
comment_id | chaine | Requis | Identifiant du commentaire
comment_type | chaine | Optionnel | (1)
comment_label | chaine | Optionnel | Caractère de renvoi associé au commentaire. Si celui-ci n'est pas précisé, il sera généré automatiquement.
comment_name | chaine | Requis | Texte du commentaire
comment_url | chaine | Optionnel | URL associé à la note et permettant d'avoir plus d'info, comme par exemple un lien vers la page de description du service de TAD.

(1) Catégorie de commentaire afin de pouvoir les différentier à l'affichage. Les valeurs possibles sont :

* information (ou non renseigné) : indique une note d'information générale
* on_demand_transport : indique qu'il s'agit d'une note d'information sur le Transport à la demande. Ce type de note doit préciser de manière succinte les conditions et le numéro de téléphone de réservation

### comment_links.txt (optionnel)
Ce fichier fait le lien entre un objet du référentiel (ligne, arrêt, horaire, etc.) et un commentaire afin de permettre d'associer plusieurs notes à un objet. et plusieurs objets à une note.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
object_id | chaine | Requis | Identifiant de l'objet associé à la note
object_type | chaine | Requis | Type de l'objet associé au commentaire. Les valeurs possibles sont stop_area, stop_point, line, route, trip, stop_time ou line_group.
comment_id | chaine | Requis | Identifiant du commentaire (lien vers le fichier comments.txt)

### commercial_modes.txt (requis)
Ce fichier décrit les modes commerciaux, c'est à dire un libellé particulier de mode de transport. Par exemple, BusWay est un nom particulier de BHNS à Nantes.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
commercial_mode_id | chaine | Requis | Identifiant du mode commercial
commercial_mode_name | chaine | Requis | Nom du mode commercial

### companies.txt (requis)
Ce fichier décrit l'opérateur de transport exploitant tout ou partie d'un des réseaux contenus dans les données.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
company_id | chaine | Requis | Identifiant de la compagnie
company_name | chaine | Requis | Nom de la compagnie
company_address | chaine | Optionnel | Adresse complète de société.
company_url | chaine | Optionnel | Url du site institutionnel de la société. A ne pas confondre avec le lien vers le site du réseau.
company_mail | chaine | Optionnel | Adresse mail de contact de la société
company_phone | chaine | Optionnel | Numéro de téléphone de contact

### contributors.txt (requis)
Ce fichier permet d'identifier la ou les sources fournissant les données du présent jeu de données.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
contributor_id | chaine | Requis | Identifiant du contributeur
contributor_name | chaine | Requis | Nom du contributeur
contributor_license | chaine | Optionnel | licence d'utilisation des données du contributeur pour le référentiel
contributor_website | chaine | Optionnel | URL du site web associé au fournisseur de données

### datasets.txt (requis)
Ce fichier liste des jeux de données du contributeur associé contenus dans le référentiel.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
dataset_id | chaine | Requis | Identifiant du jeu de données
contributor_id | chaine | Requis | Identifiant du contributeur (lien vers le fichier contributors)
dataset_start_date | date | Requis | Date de début de prise en compte du jeu de données (peut-être différent de la date de début de validité de l'export source)
dataset_end_date | date | Requis | Date de fin de prise en compte du jeu de données (peut-être différent de la date de fin de validité de l'export source)
dataset_type | entier (1) | Optionnel | Type de données représentant la "fraicheur"
dataset_extrapolation | entier | Optionnel | Indique si les données du service ont été extrapolées (le champ a pour valeur 1) ou non (le champ a pour valeur 0)
dataset_desc | chaine | Optionnel | Note indiquant le contenu du jeu de données
dataset_system | chaine | Optionnel | Nom du système source ayant généré les données ou du format des données

(1) Spécifie le type de données :

* 0 - il s'agit de données théoriques
* 1 - il s'agit de données de grèves ou révisées
* 2 - il s'agit de données de production du jour J

### frequencies.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
trip_id | chaine | Requis | Identifiant de la circulation
start_time | heure | Requis | Heure de début de la fréquence
end_time | heure | Requis | Heure de fin de la fréquence. Spécifier 26:00:00 pour 2h du matin du jour considéré.
headway_secs | entier | Requis | Fréquence de départ en secondes

### lines.txt (requis)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
line_id | chaine | Requis | Identifiant de la ligne commerciale
line_code | chaine | Optionnel | Code de la ligne commerciale
line_name | chaine | Requis | Nom de la ligne commerciale
forward_line_name | chaine | Optionnel | Nom de la ligne en sens aller
forward_direction | chaine | Optionnel | Arrêt de destination principal en sens aller (lien vers un arrêt commercial du fichier stops)
backward_line_name | chaine | Optionnel | Nom de la ligne en sens retour
backward_direction | chaine | Optionnel | Arrêt de destination principal en sens retour (lien vers un arrêt commercial du fichier stops)
line_color | couleur | Optionnel | Couleur de la ligne
line_text_color | couleur | Optionnel | Couleur du code de la ligne
line_sort_order | entier | Optionnel | Clé de trie de la ligne au sein du réseau. Les indices les plus petits sont retournés en premier.
network_id | chaine | Requis | Identifiant du réseau principal de la ligne (lien vers le fichier networks)
commercial_mode_id | chaine | Requis | Identifiant du mode commercial (lien vers le fichier  commercial_modes)
geometry_id | chaine | Optionnel | Identifiant du tracé représentant la ligne (lien vers le fichier geometries)
line_opening_time | heure | Optionnel | Heure de début de service de la ligne (quelque soit le type de jour ou la periode). Si cette information n'est pas fournie, elle sera recalculée.
line_closing_time | heure | Optionnel | Heure de fin de service de la ligne (quelque soit le type de jour ou la periode). Si cette information n'est pas fournie, elle sera recalculée. Spécifier une heure superieure à 24 pour indiquer une heure sur le jour d'après.

### routes.txt (requis)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
route_id | chaine | Requis | Identifiant du parcours
route_name | chaine | Requis | Nom du parcours
direction_type | chaine (1) | Optionnel | Description de la direction de la route. Ce champ est libre, mais il est préconisé d'utiliser un des éléments recommandés ci-dessous.
line_id | chaine | Requis | Identifiant de la ligne commerciale (lien vers le fichier lines)
geometry_id | chaine | Optionnel | Identifiant du tracé représentant le parcours (lien vers le fichier geometries)
destination_id | chaine | Optionnel | Identifiant de la destination principale (lien vers le fichier stops.txt de type zone d'arrêt)

(1) Liste des valeurs recommandées pour le champ _direction_type_ :

* Pour des sens aller et retour : _forward_ et _backward_
* Pour des parcours en boucle : _clockwise_ et _anticlockwise_
* Pour des parcours entrant et sortants : _inbound_ et _outbound_

### physical_modes.txt (requis)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
physical_mode_id | chaine | Requis | Identifiant du mode physique obligatoirement dans la liste ci-dessous.
physical_mode_name | chaine | Requis | Nom du mode physique
co2_emission | décimal | Optionnel | Taux d’émission de CO2 du mode physique par voyageur et par km.

**Liste des modes physique disponible :**

Il existe une hiérarchie des modes permettant de classer les zones d'arrêt par son mode de plus haut niveau (cf. norme [NeTEx](http://www.normes-donnees-tc.org/wp-content/uploads/2014/05/NF_Profil_NeTEx_pour_les_arrets-_F-_-_v2.pdf) , chapitre 6.2.3)

    1. Aérien
    2. Maritime/Fluvial
    3. Ferré
    4. Métro
    5. Tram
    6. Funiculaire/Câble
    7. Bus/Car/Trolley

La liste ci-dessous indique la hierarchie NeTEx associée à chaque mode (sans le préfixe du type d'objet retourné par l'API) :

Code du mode physique | Nom du mode physique | Hierarchie NeTEx
--- | --- | ---
Air | Avion | 1
Boat | Navette maritime/fluviale | 2
Bus | Bus | 7
BusRapidTransit | Bus à haut niveau de service | 7
Coach | Autocar | 7
Ferry | Ferry | 2
Funicular | Funiculaire | 6
LocalTrain | Train régional / TER | 3
LongDistanceTrain | Train grande vitesse | 3
Metro | Métro | 4
RapidTransit | Train de banlieue / RER | 3
RailShuttle | Navette ferrée (VAL) | 3
Shuttle | Navette | 7
SuspendedCableCar | Téléphérique / télécabine | 6
Taxi | Taxi | 7
Train | Train | 3
Tramway | Tramway | 5

La liste ci-dessous complète cette liste de modes physiques avec les modes de rabattement possibles dans un itinéraire, afin de leur associer un taux de CO2 dans l'export.

Code du mode physique | Nom du mode physique
--- | ---
BikeSharingService | Vélo en libre service
Bike | Vélo
Car | Voiture

### equipments.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
equipment_id | chaine | Requis | Identifiant de l'équipement |
wheelchair_boarding | entier (1) | Optionnel | Accès UFR |
sheltered | entier (1) | Optionnel | Abris couvert
elevator | entier (1) | Optionnel | Ascenseur
escalator | entier (1) | Optionnel | Escalier mécanique
bike_accepted | entier (1) | Optionnel | Embarquement vélo
bike_depot | entier (1) | Optionnel | Parc vélo
visual_announcement | entier (1) | Optionnel | Annonce visuelle
audible_announcement | entier (1) | Optionnel | Annonce sonore
appropriate_escort | entier (1) | Optionnel | Accompagnement à l'arrêt
appropriate_signage | entier (1) | Optionnel | Information claire à l'arrêt

    (1) Les valeurs possibles sont :
        0 ou non spécifié - aucune information disponible
        1 - l'équipement est disponible
        2 - l'équipement n'est pas disponible

### stops.txt (requis)
Une ligne du fichier "stops.txt" représente un point ou une zone où un véhicule dépose ou fait monter des voyageurs.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
stop_id | chaine | Requis | Identifiant de l'arrêt
visible | entier | Optionnel | Indique si le stop peut être retourné dans l'autocomplétion (valeur 1) ou s'il est ignoré (valeur 0).
stop_name | chaine | Requis | Nom de l'arrêt
stop_lat | décimal | Requis | Latitude. Ce champ est obligatoire pour les arrêts physiques (location_type = 0) même si le champ geometry_id est renseigné afin de faciliter sa lecture. Il est conseillé pour les zones d'arrêts (location_type = 1), et inutile pour les autres cas.
stop_lon | décimal | Requis | Longitude. Ce champ est obligatoire pour les arrêts physiques (location_type = 0) même si le champ geometry_id est renseigné afin de faciliter sa lecture. Il est conseillé pour les zones d'arrêts (location_type = 1), et inutile pour les autres cas.
fare_zone_id | chaine | Optionnel | Zone tarifaire de l'arrêt
location_type | entier (1) | Requis | Type de l'arrêt ou de la zone
geometry_id | géometrie | Optionnel | Ce champ est un lien vers le fichier geometries.txt qui décrit la géométrie associée à une zone géographique (type 2) afin de permettre au moteur de définir les adresses couvertes en cas de TAD zonal "adresse à adresse". Ce champ peut également être utilisé pour préciser une géométrie pour les zones d'arrêts (type 1) et les communes (type 4) pour enrichir le web service.
parent_station | chaine | Optionnel | Identifiant de la zone d'arrêt, utilisé que sur des arrêts de type 0 (point d'arrêt)
stop_timezone | timezones | Optionnel | Fuseau horaire, se référer à http://en.wikipedia.org/wiki/List_of_tz_zones
equipment_id | chaine | Optionnel | Identifiant de la propriété accessibilité

    (1) Type de l'arrêt ou de la zone :
        0 ou non spécifié - Arrêt physique (objet stop_point)
        1 - Zone d'arrêt (objet stop_area)
        2 - Zone géographique (pour le TAD zonal de type "adressse à adresse", objet stop_zone)

### stop_times.txt (requis)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
stop_time_id | chaine | Optionnel | Identifiant unique de l'horaire dans le jeu de données. Cette information n'est pas pérenne et permet uniquement de faire le lien entre un horaire (fichier stop_times.txt) et un commentaire (fichier comments.txt) en utilisant le fichier comment_links.txt. Si ce champ n'est pas fourni, l'horaire ne pourra pas êter lié à un commentaire.
trip_id | chaine | Requis | Identifiant de la circulation
arrival_time | heure | Requis | Heure d'arrivée. Si l'heure d'arrivée n'est pas connue, elle doit être estimée par le système fournissant les données et le champ *date_time_estimated* doit être spécifié à 1. Si la descente est interdite à cet arrêt, l'heure d'arrivée doit être indiquée et le champ *drop_off_type* doit être spécifié à 1.
departure_time | heure | Requis | Heure de départ. Si l'heure de départ n'est pas connue, elle doit être estimée par le système fournissant les données et le champ *date_time_estimated* doit être spécifié à 1. Si la montée est interdite à cet arrêt, l'heure de départ doit être indiquée et le champ *pickup_type* doit être spécifié à 1.
boarding_duration | entier | Optionnel | Durée nécessaire à l'embarquement en secondes (train, avion, ferry, etc.). Cette valeur est obligatoirement positive ou nulle.
alighting_duration | entier | Optionnel | Durée nécessaire au débarquement en secondes (train, avion, ferry, etc.). Cette valeur est obligatoirement positive ou nulle.
stop_id | chaine | Requis | Identifiant de l'arrêt physique de passage (cas général). Ce champ peut également référencer une "zone géographique" (stop de type 2) ou une commune (stop de type 3) dans le cas de TAD zonal.
stop_sequence | entier | Requis | Ordre de passage de desserte dans la circulation
stop_headsign | chaine | Optionnel |
pickup_type | entier (1) | Optionnel | Indication sur l'horaire (issues du gtfs)
drop_off_type | entier (1) | Optionnel | Indication sur l'horaire (issues du gtfs)
local_zone_id  | entier | Optionnel | identifiant de la zone d'ITL de l'horaire
date_time_estimated | entier (2) | Optionnel | Précise si l'heure de passage est fiable ou si elle est donnée à titre indicative

    (1) Indication sur l'horaire (issues du gtfs) :
        0 (par défaut) - Horaire régulier
        1 - Montée ou descente interdite
        2 - Horaire sur réservation associé à un TAD (si un message est associé au TAD, voir la liaison avec comment_links.txt)

    (2) La fiabilité peut prendre les valeurs suivantes :
        0 - L'heure de passage est fiable
        1 - L'heure de passage est estimée
        non spécifiée :
            s'il s'agit d'un horaire associé à une zone (stop de location_type de valeur 2 ou 3) : l'heure est estimée
            sinon, l'heure de passage est fiable

### transfers.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
from_stop_id | chaine | Requis | Identifiant de l'arrêt de l’origine de la correspondance (lien vers le fichier stops)
to_stop_id | chaine | Requis | Identifiant de l'arrêt de la destination de la correspondance (lien vers le fichier stops)
min_transfer_time | entier | Optionnel | Durée minimale de la correspondance en secondes. Cette valeur correspond à la durée de marche à pied qui sera affichée dans les médias. Si la valeur n'est pas spécifié, le système calcul un temps minimum sur la base de la distance Manhattan entre les deux arrêts. La valeur automatique alors calculée a une valeur minimum de 60 secondes. Note : Il est possible que la valeur fournie soit inferieur à 60 (ex : 0 dans le cas d'une correspondance garantie)
real_min_transfer_time | entier | Optionnel | Durée réelle de correspondance en secondes. Cette valeur correspond à la durée de marche à pied (min_transfer_time) à laquelle on ajoute une durée de tolérance d'exécution (temps minimum de correspondance). Si la valeur n'est pas spécifié, le système utilise (en plus du min_transfer_time) un paramètre par défaut qui est de 120 secondes en général.  La valeur automatique alors calculée sera donc supérieur ou égale à 120 secondes. La valeur saisie ne peut être inférieure à min_transfer_time (mais peut-être égale).
equipment_id | string | Optionnel | Identifiant de description des propriétés (lien vers le fichier equipments)

### trip_properties.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
trip_property_id | chaine | Requis | Identifiant de la propriété |
wheelchair_accessible | entier (1) | Optionnel | Le véhicule est accessible aux UFR |
bike_accepted | entier (1) | Optionnel | Le véhicule permet l'embarquement de vélo
air_conditioned | entier (1) | Optionnel | Le véhicule dispose de l'air conditionné
visual_announcement | entier (1) | Optionnel | Le véhicule dispose d'annonces visuelles
audible_announcement | entier (1) | Optionnel | Le véhicule dispose d'annonces sonores
appropriate_escort | entier (1) | Optionnel | Un service d'accompagnement à bord est possible (à la montée et à la descente)
appropriate_signage | entier (1) | Optionnel | L'affichage à bord est est claire et adapté aux personnes en déficience mentale
school_vehicle_type | entier (2) | Optionnel | Type de transport scolaire

    (1) Les valeurs possibles sont :
        0 ou non spécifié - aucune information disponible
        1 - l'équipement est disponible
        2 - l'équipement n'est pas disponible

    (2) Type de transport scolaire :
        0 ou non spécifié  : transport régulier (non scolaire)
        1 : transport scolaire exclusif
        2 : transport mixte (scolaire et régulier)

### trips.txt (requis)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
route_id | chaine | Requis | Identifiant du parcours (lien vers le fichier routes)
service_id | chaine | Requis | Identifiant dues jours de fonctionnements
trip_id | chaine | Requis | Identifiant de la circulation
trip_headsign | chaine | Optionnel | Nom de la circulation
block_id | chaine | Optionnel | Identifiant du prolongement de service
company_id | chaine | Requis | Identifiant de la compagnie (lien vers le fichier company)
physical_mode_id | chaine | Requis | Identifiant du mode physique (lien vers le fichier physical_modes)
trip_property_id | chaine | Optionnel | Identifiant de la propriété accessibilité (lien vers le fichier trip_properties)
dataset_id | chaine | Requis | Identifiant du jeu de données ayant fourni la circulation (lien vers le fichier datasets).
geometry_id | chaine | Optionnel | Identifiant du tracé représentant la circulation (lien vers le fichier geometries)

    Pour préciser si la circulation est sur réservation (tout ou partie), il faut :
        Indiquer au niveau de l'horaire (fichier stop_times.txt) si la montée et/ou la descente est à réservation
        Indiquer un commentaire (optionnel) de type TAD via les fichiers comments.txt et comment_links.txt

### geometries.txt (optionnel)
Ce fichier contient la représentation spatiale d'une géométrie (pour des lignes, parcours et/ou circulations). Chaque ligne du fichier représente une géométrie complète de l'objet.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
geometry_id | chaine | Requis | Identifiant de la géométrie.
geometry_wkt | géométrie | Requis | Représentation spatiale de la géométrie selon le standard http://fr.wikipedia.org/wiki/Well-known_text.

    Les lignes et parcours peuvent être des LINESTRING ou des MULTILINESTRING.
    Les circulations ne peuvent être que des LINESTRING. Si une MULTILINESTRING est spécifiée, seule la première LINESTRING sera utilisée.
    Les points d'arrêts sont des POINT.
    Les zones d'arrêt peuvent être des POINT, POLYGON ou MULTIPOLYGON.
    Les zones géographiques et communes peuvent être des POLYGON ou MULTIPOLYGON.

    Seules les types de géométries spécifiées sont retenues, les autres types géométries sont ignorées.
    Le format du fichier est volontairement simple, une évolution pourra être envisagée si le besoin est rencontré.

### object_properties.txt (optionnel)
Ce fichier contient la description des propriétés complémentaires sur les différents objets du référentiel.
Ces propriétés sont sous forme de liste de clés / valeurs qui doivent être standardisées par processus.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
object_type | chaine | Requis | Type d'objet sur lequel la propriété porte (line, route, trip, stop_area, stop_point)
object_id | chaine | Requis | Identifiant de l'objet sur lequel la propriété porte
object_property_name | chaine | Requis | Nom de la propriété complémentaire (texte libre)
object_property_value | chaine | Requis | Valeur de la propriété complémentaire (texte libre)

### object_codes.txt (optionnel)
Ce fichier contient la liste des codes d'identification complémentaires dans les systèmes externes des différents objets du référentiel.
Ces propriétés sont sous forme de liste de clés / valeurs qui doivent être standardisées par processus.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
object_type | chaine | Requis | Type d'objet sur lequel la propriété porte (network, line, route, trip, stop_area, stop_point)
object_id | chaine | Requis | Identifiant de l'objet sur lequel la propriété porte
object_system | chaine | Requis | Nom du système d'identification de l'objet  (texte libre). Par exemple : "Timeo" ou "UIC" pour les arrêts, "Reflex" pour les lignes.
object_code | chaine | Requis | Code d'identification de l'objet dans le système considéré.

Kisio Digital fournit dans ce fichier :
* les identifiants des objets dans l'ancien système Navitia pour les objets "network", "line", "route", "trip", "stop_point" et "stop_area" avec pour object_system la chaine **"navitia1"**.
* les identifiants des objets déclarés dans la source d'alimentation (NTFS ou GTFS par exemple) avec pour object_system la chaine **"source"**.

### admin_stations.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
admin_id | chaine | Requis | Identifiant de la commune (ou du quartier) tel que retourné par l'API Navitia
admin_name | chaine | Requis | Nom de la commune (ou quartier).
stop_id | chaine | Requis | Identifiant de la zone d'arrêt utilisée comme accroche de la commune (lien vers le fichier stops). Stop de type 1 oligatoirement.
stop_name | chaine | Optionnel | Nom de la zone d'arrêt (pour faciliter la lisibilité du fichier)

### line_groups.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
line_group_id | chaine | Requis | Identifiant du groupe de ligne
line_group_name | chaine | Requis | Nom du groupe de ligne
main_line_id | chaine | Requis | Identifiant de la ligne principale du groupe de lignes (lien vers le fichier lines.txt)

Un commentaire peut être associé à un groupe de lignes dans les fichiers comments.txt et et comment_links.txt .

### line_group_links.txt (optionnel)
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
line_group_id | chaine | Requis | Identifiant du groupe de ligne
line_id | chaine | Requis | Identifiant de la ligne faisant partie du groupe de lignes (lien vers le fichier lines.txt). Attention, une ligne peut faire partie de plusieurs groupes de lignes.

### feed_infos.txt (requis)
Ce fichier contient des informations sur le jeu de données et le système amont qui l'a généré. Pour faciliter son utiilisation, la structure du fichier est générique, et la liste des informations est listée ci-dessous.

#### Description du format du fichier
Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
feed_info_param | chaine | Requis | Nom du paramètre
feed_info_value | chaine | Requis | Valeur du paramètre

#### Description du contenu du fichier

Ce fichier contient 3 types de paramètres :
* des paramètres obligatoires
* des paramètres recommandés (optionnels)
* des paramètres libres (possibilité d'ajouter autant de paramètre que souhaité)

Le tableau ci-dessous liste les paramètres obligatoires et recommandés.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
ntfs_version | chaine | Requis | Version du format NTFS utilisé dans l'export (par exemple : "0.3")
feed_start_date | date | Optionnel | Date de début de validité du jeu de données
feed_end_date | date | Optionnel | Date de fin de validité du jeu de données
feed_creation_date |  date |  Optionnel | Date de génération du jeu de données
feed_creation_time | heure | Optionnel | Heure de génération du jeu de données

Le tableau ci-dessous indique les paramètres libres renseignés par Kisio Digital (dépend de l'outil qui génère les données).

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
feed_publisher_name | chaine | Libre | Société/Entité fournissant le jeu de données
feed_license | chaine | Libre | Licence d'utilisation des données globale du référentiel
feed_license_url | chaine | Libre | URL associée à la license d'utilisation des données
fusio_url | chaine | Libre | URL du système ayant généré le jeu de données
fusio_version | chaine | Libre | Version du système ayant généré le jeu de données
tartare_platform | chaine | Libre | Tag indiquant la plateforme qui a généré les données
tartare_coverage_id | chaine | Libre | Id du coverage Tartare ayant généré le jeu de données (1)
tartare_contributor_id | chaine | Libre | Id du contributeur Tartare ayant généré le jeu de données (1)

    (1) seul l'un des champs `tartare_coverage_id` et `tartare_contributor_id` sera présent. Il servent a tracer la source de la donnée dans Tartare afin de faciliter les diagnostiques.

### grid_calendars.txt (optionnel)
Ce fichier contient les calendriers.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
grid_calendar_id | chaine | Requis | Identifiant du calendrier
name | chaine | Requis | Nom du calendrier
monday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
tuesday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
wednesday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
thursday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
friday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
saturday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour
sunday | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour

### grid_exception_dates.txt (optionnel)
Ce fichier contient les exceptions sur les calendriers des grilles horaires.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
grid_calendar_id | string | Requis | Identifiant du calendrier de grille horaire
date | date | Requis | Date de l'exception
type | entier | Requis | 0 : Ne circule pas ce jour <br> 1 : Circule ce jour

### grid_periods.txt (optionnel)
Ce fichier contient les périodes des calendriers des grilles horaires.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
grid_calendar_id | chaine | Requis | Identifiant du calendrier de grille horaire
start_date | date | Requis | Date de début
end_date | date | Requis | Date de fin

### grid_rel_calendar_line.txt (optionnel)
Ce fichier contient toutes les relations entre les lignes et les calendriers des grilles horaires.

Colonne | Type | Contrainte | Commentaire
--- | --- | --- | ---
grid_calendar_id | chaine | Requis | Identifiant du calendrier de grille horaire
line_id | chaine | Requis | Identifiant de la ligne associée à ce calendrier (lien vers le fichier lines). Ce champ peut être vide si le champ line_external_code est renseigné.
line_external_code | chaine | Requis | cette colonne contient le code externe NAViTiA 1 de la ligne (lien vers le fichier lines). Ce champ peut être vide si le champ line_id et renseigné

# Évolutions possibles du format
Ce chapitre liste des évolutions du format qui peuvent être intéressantes si elles sont utiles concrètement.
## Modification du champ service_id et du nom du fichier calendar.txt
L'identifiant d'un calendrier ne suit pas la même convention que les autres identifiants (même s'il est cohérent avec le GTFS). Une évolution possible serait de changer le libellé du champ pour indiquer "calendar_id".
De plus, le nom du fichier pourra être changé en "calendars.txt" pour améliorer la cohérence.

## Gestion des données perturbées / de grèves
Afin de limiter la complexité du format, la gestion des données de grève sera effectuée par plusieurs exports :

1. un export contenant toutes les données théoriques du référentiel. L'export NTFS est un export classique, et dont la clé "revised_networks" du fichier "feed_infos.txt" est vide ou non renseignée.
2. un ou plusieurs exports NTFS de grèves, dont chaque export fournit toutes les données (impactées par la grève ou non) :
    * de un ou plusieurs réseaux spécifiés par la clé **revised_networks** du fichier **feed_infos.txt**
    * pour des données valides entre les dates spécifiées par **feed_start_date** et **feed_end_date**

Un champ complémentaire et optionnel "base_trip_id" est à prévoir dans le fichier "trips.txt" afin de permettre d'associer la circulation théorique et la circulation adaptée (en cas de données de grève par exemple).

## Gestion avancée des géométries (tracés des lignes, parcours et circulations)
Afin de ne pas complexifier inutilement le format NTFS et les outils qui vont le manipuler, le fichier "geometries.txt" indique un tracé complet pour une géométrie, comme une ligne en fourche ou une ligne à tiroir.  Afin de pouvoir afficher le tracé réel des bus dans la feuille de route (ie. n'avoir que la portion utilisée de la ligne), un découpage de cette géométrie est réalisé dans Navitia de manière automatique.
Si le besoin d'affiner cette gestion est validé, une évolution du format du fichier "geometries.txt" peut être envisagé de la manière suivante (à confirmer) :
* Une ligne représentera un segment de la ligne/parcours/circulation entre deux points d'arrêts consécutifs (et de manière orientée ?)
* La précision des points d'arrêts d'origine et de destination du segment est faite par l'ajout de deux colonnes optionnelles

## Gestion de l'émission de CO2 par ligne ou véhicule
L'émission de CO2 est gérée en France par mode de transport. Les valeurs sont spécifiées par l'ADEME. Le format NTFS permet d'échanger cette information au niveau du mode physique. En fonction des besoins rencontrés à l'avenir, un ajout de cette informaion au niveau de la ligne et/ou du véhicule est envisagée. Dans ce cas, un système de "surcharge" de l'inforamtion sera à mettre en place pour prendre en compte par priorité :

1. le taux au niveau du véhicule s'il est disponible
2. le taux au niveau de la ligne s'il est disponible
3. le taux au niveau du mode s'il est disponible
4. une indication sur le fait que la valeur est inconnue

## Gestion d'un service (ex : Acces+)
Ajouter la notion de "service" pour un accompagnement sur un réseau de transport, comme par exemple :
* Accès+ TER ou Accès+ TGV sur le réseau SNCF
* OptiBus de Keolis PMR Rhone d'accompagnement sur le réseau de TCL

Ce service permet d'ajouter de l'accessibilité (UFR, Cognitif ou autre) sur un ou plusieurs objets ou directement dans le calcul.

# Exemples de modélisation de TAD
Voici quelques exemples de modélisation de TAD dans les fichiers NTFS. Seuls les fichiers impactés sont représentés (stops.txt et stop_times.txt).
Il est à également à noter qu'il est possible :
* de faire du rabattement vers horaire en spécifiant bien un horaire à un point d'arrivé (et pas une zone) dans le ficheir stop_time,
* de faire de la fréquence sur du TAD zonal en utilisant le fichier "frequencies.txt"

**Attention, les coordonnées et les surfaces des zones ne sont pas cohérentes, c'est l'architecture des données dans les fichiers qui est importante ici.**

Pour résumé, il suffit de connaitre les points suivants:
* Les horaires sont portés par des « stop » (un stop est une ligne issue du fichier stop)
* Les stop peuvent être
    * Des stop_points (type=0)
    * Des stop_area (type=1)
    * Des area (type=2): représentant une surface géographique (commune, quartier, zone...)
* Et ensuite, on peut accrocher des horaires sur ces stops, quelque soient leur type
    * dans les faits, on accrochera des horaires sur les stop_points et les area, mais pas sur les stop_areas: les impacts sur les différentes utilisations (itinéraire, fiche horaire) seraient non maitrisés

Cette modélisation unique traite l'ensemble des possibilité:
* TAD zonal d'adresse à adresse
* TAD zonal d'arrêts à arrêts
* TAD de rabattement d'adresse à arrêts
* TAD de rabattement d'arrêts à arrêts
* TAD "multiple" Zones - Arrêts - Zones, Arrêts - Zones - Arrêts, etc...

### Modélisation NTFS

* Les lignes intégralement TAD, sans horaire, sont déclarée en "fréquence"
* Les stop_point du fichier "stops" de type "shape", ne devraient pas être en correspondance avec d'autres stop_point.
    * A l'intégration des données, les correspondances éventuellement déclarées seront ignorées
* Les correspondances entre 2 circulations ne sont autorisées QUE si un des 2 horaires est fixe
    * Les correspondances entre 2 horaires estimés sont interdites (stop.estimated vers stop.estimated)
* Des lignes peuvent traiter de zones d'adresse à adresse
    * Les itinéraires adresse à adresse au sein de ces zones ne seront pas proposés
* les lignes TAD ne sont plus typées
    * Ce sont les stop_times qui détermine le type de TAD
* Par exemple:
    * Gestion du TAD zonal adresse à adresse et arrêt à arrêt en horaire estimé
        * tous les horaires sont estimés
            * adresse à adresse=depuis un stop_point de type shape vers un stop_point de type shape
                * donc pas en correspondance parce que les hsape ne sotn pas en correspondance
            * arrêt à arrêt
                * correspondance proposée avec des lignes à horaires fixes
    * Gestion des correspondances entre TAD de rabattement, la même règle est appliquée
        * si l'horaire de rabattement est fixe
            * la correspondance sera proposée
        * si l'horaire de rabattement est estimé
            * la correspondance avec un horaire fixe (train) est possible
            * la correspondance avec un autre TAD à rabattement estimé ne sera pas possible


### Exemples
#### Ligne mixte
Le cas de la zone 2 ci-dessous est trivial: on définit cette zone à l'aide d'un shape qui englobe les adresses ayant accès au TAD. Il suffit ensuite d'associer un horaire à ce shape.

![Exemple de ligne mixte](NTFS_image1.png)

Les zones de dessertes ne remontent pas en autocompletion: elles permettent de déterminer l'offre uniquement.
On peut alimenter le fichier stop_times en mettant des horaires précis sur chacun des points d'arrêt pouvant appartenir à des zones d'arrêts différentes, avec des informations ITL manuelles:

**Fichier stop: déclare les "arrêts"**

stop_id | stop_name | stop_lat | stop_lon | location_type | geometry_id | parent_station
--- | --- | --- | --- | --- | --- | ---
stop_point_A | A | 47.01 | 1.01 | 0 |  | stop_area_A
stop_point_B | A | 47.01 | 1.01 | 0 |  | stop_area_B
stop_point_C | A | 47.01 | 1.01 | 0 |  | stop_area_C
stop_point_D | A | 47.01 | 1.01 | 0 |  | stop_area_D
stop_point_E | A | 47.01 | 1.01 | 0 |  | stop_area_E
stop_point_H | A | 47.01 | 1.01 | 0 |  | stop_area_H
stop_area_A | A | 47.01 | 1.01 | 1 |  |
stop_area_B | A | 47.01 | 1.01 | 1 |  |
stop_area_C | A | 47.01 | 1.01 | 1 |  |
stop_area_D | A | 47.01 | 1.01 | 1 |  |
stop_area_E | A | 47.01 | 1.01 | 1 |  |
stop_area_H | A | 47.01 | 1.01 | 1 |  |
zone_2 | Zone 1 | 47.01 | 1.01 | 2 | id_vers_POLYGON((1 1,5 1,5 5,1 5,1 1)) |

**Fichier stop_times: déclare les "horaires", estimés ou non**

trip_id | stop_id | arrival_time | departure_time | stop_sequence | pickup_type | drop_off_type | date_time_estimated | zone_itl
--- | --- | --- | --- | --- | --- | --- | --- | ---
trip_1 | stop_point_A |  | 09:02:00 | 1 | 0 | 0 | 0 | 0
trip_1 | stop_point_B | 09:15:00 | 09:05:00 | 2 | 2 | 2 | 1 | 1
trip_1 | stop_point_C | 09:15:00 | 09:07:00 | 2 | 2 | 2 | 1 | 1
trip_1 | stop_point_D | 09:15:00 | 09:09:00 | 2 | 2 | 2 | 1 | 1
trip_1 | stop_point_E | 09:17:00 | 09:18:00 | 1 | 0 | 0 | 0 | 2
trip_1 | zone_2 | 09:25:00 | 09:20:00 | 2 | 2 | 2 | 1 | 3
trip_1 | stop_point_H | 09:27:00 |  | 1 | 0 | 0 | 0 | 4

#### Ligne arrêt à arrêt sans horaires
![Exemple de ligne arrêt à arrêt](NTFS_image2.png)

Les zones n'ont pas de valeur dans le NTFS: la prise des passagers se fait sur des stop_point. L'idée est de déclarer des "blocs d'horaires" sur chacun des stop_point de chaque zone :

* A>B>C
    * descente interdite
    * 08h00 partout
* A>B>C
    * montée interdite
    * 08h10 partout
* D>E>F
    * descente interdite
    * 09h00 partout
* D>E>F
    * montée interdite
    * 09h10 partout
* G>H>I
    * descente interdite
    * 10h00 partout
* G>H>I
    * montée interdite
    * 10h10 partout

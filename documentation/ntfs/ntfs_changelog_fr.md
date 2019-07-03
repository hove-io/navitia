# Changelog du format NTFS

* Version 0.1 du 15/12/2014 :
    * Ajout de la gestion des codes externes et des propriétés complémentaires génériques par objet
* Version 0.2 du 20/02/2015 :
    * Ajout des heures d'ouverture et de fermeture de la ligne
    * Ajout du taux d'émission de CO2 par mode physique et de la liste exhaustive des modes physiques
    * Ajout du type de circulation (fichier trips) pour permettre de spécifier le type de TAD
    * Ajout du fichier admin_stations.txt
    * Ajout de la gestion du TAD Zonal
        * Ajout d'une URL dans le fichier odt_conditions.txt
        * Ajout dans le fichier stops.txt des zones géographiques et du type de zone
    * Ajout de 2 fichiers sur la gestion des propriétés complémentaires sur les objets
* Version 0.3 du 14/04/2015 :
    * Modification de la gestion du TAD : suppression des zones de TAD arrêt à arrêt
    * Modification du nom de fichier companies.txt
    * Remplacement de la géométrie dans le fichier stops.txt par une liaison vers le fichier de géométries
    * Ajout des modes BSS, car et bike pour la consommation de CO2 des modes de rabattement
    * Ajout d'un fichier feed_infos
    * Création du fichier comment_links pour associer plusieurs commentaires à un objet; inclusion du TAD dans les comment (avec un type) et ajout d'une URL
    * Ajout d'un champ stop_time_id optionnel pour permettre la liaison entre les notes et les horaires
    * Modification des fichiers object_codes et object_properties pour harmoniser le format (séparation de l'id et du type d'objet)
    * Suppression des champs "external_code" car à inclure dans object_codes
* Version 0.4 du 27/04/2015 :
    * Ajout d'une destination principale à la notion de parcours
    * Ajout de la notion de groupe de ligne
* Version 0.5 du 25/10/2015 (version technique, pas de modification effective de format)
    * Gestion du champ line_text_color
    * Gestion du fichier equipments.txt
* Version 0.6 du 24/11/2015
    * Ajout de la gestion des services (fichier frames.txt) et modification des contributeurs pour ajouter une licence et un site web
    * Ajout de la gestion des données adaptées (dans feed_info.txt une clé "revised_networks" listant les réseaux impactés par une grève ou une autre perturbation)
    * Modification de l'identifiant des calendriers et des calendriers de période
    * Modification des commentaires : Ajout d'un label optionnel et modification du nom du champ "comment_name" en "comment_value"
    * Modification du champ "is_forward" en "direction_type" dans le fichier routes.txt et passage en champ de type chaine
* Version 0.6.1 du 25/02/2016
    * "frame" remplacé par "dataset" dans les attributs du fichier frames.txt
    * Fichier frames.txt rennomé à datasets.txt
    * Attribut frame_id remplacé par dataset_id dans le fichier trips.txt
    * Attribut frame_id supprimé dans le fichier stops.txt
* Version 0.6.2 du 20/06/2016
    * ajout de l'exention fares pour la gestion des tarifs
    * ajout des temps d'embarquement et de débarquement dans le fichier stop_times.txt.
* Version 0.6.3 du 25/01/2017
    * ajout du champ dataset_extrapolation dans le fichier datasets.txt
* Version 0.6.4 du 31/03/2017
    * ajout du mode physique RailShuttle pour les VAL
* Version 0.6.5 du 02/01/2018
    * precision sur les arrival/departure times
    * feed_infos.txt est optionnel en 0.6
* Version 0.7.0 du 03/01/2018
    * feed_infos.txt est obligatoire
    * le lien entre "stop" et "contributor" est supprimé (liaison possible via "vehicle_journey")
    * le lien entre "vehicle_journey" et "contributor" est supprimé, et se fait via "dataset"
    * le champ "company_fax" est supprimé et les champs adresses sont fusionnés en un seul champ en texte libre
    * ajout d'un champ "network_address" afin de pouvoir renseigner l'adresse du réseau.
    * les données adaptées (intégrées dans la version 0.6) sont déplacées dans la section "évolutions possibles"
* Version 0.7.1 du 27/02/2018
    * les fichiers contributors.txt et datasets.txt sont obligatoires, ainsi que le lien entre "trip" et "dataset_id"
    * le champ "comment_value" est renommé "comment_name" (conformité avec le code)
    * les valeurs de comment_type changent
* Version 0.7.2 du 30/10/2018
    * dans le fichier stops.txt, suppresion du "location_type = 3" qui n'est pas utilisé
* Version 0.7.3 du 12/11/2018
    * ajout du mode physique SuspendedCableCar pour les Téléphérique / télécabine
* Version 0.7.4 du 21/02/2019
    * modification des propriétés présentes dans le fichier feed_info
* Version 0.8 du 25/03/2019
    * Ajout de la gestion des pathways
* Version 0.9 du 16/05/2019
    * AJout du nouveau modèle tarifaire

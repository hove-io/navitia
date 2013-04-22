Autocompletion
==============

L'autocomplétion permet de retrouver des objets de transport en commun à partir de quelques lettres.

L'utilisation typique est l'autocomplétion pour la saisie d'un point d'entrée pour un itinéraire. Ainsi
lorsque l'utilisateur tape « pyr » on lui proposera aussi bien « métro Pyramides » « métro Pyrénées » ou
« Rue des Pyrénées ».

Du fait de l'utilisation en auto-complétion, le retour est très rapide (inférieur à 10 ms).

Fonctionnement général
----------------------

Lors d'une requête utilisateur, il saisit une chaîne de charactères telle que « 20 bd ponia par ». La requête
est découpée *tokens* correspondant à chaque mot. Ensuite l'algorithme cherche parmi l'ensemble des objets ceux
dont le nom contient des mots qui commencent par ces tokens.

Une note est associée à chaque résultat afin de retourner le résultat le plus pertinent.

La suite de cette documentation présente comment est calculée la pertinence des objets.

Prétraitement du texte
----------------------

Le texte est systématiquement pré-traité afin de prendre en compte plusieurs éléments :

 * Mots ignorés ("bis", "de", "la")
 * Synonymes ("hôtel de ville" ou "mairie")
 * abréviations ("st" à la place de "saint")

Tous ces éléments sont configurés à partir d'un fichier de configuration au moment de la binarisation.

Pertinence des objets en amont
------------------------------

Lors de la binarisation, une pertinence « par défaut » des objets est calculée (ou définie manuellement). Cela permet
de favoriser certains objets par rapport à d'autres. Voici la liste des paramètres pouvant intervenir :

 * Nature de l'objet : admin (quartier, arrondissement ou ville), zone d'arrêt, lieux remarquables (POI), adresse (métro Pyrénées avant rue des Pyrénées), point d'arrêt
 * Commune de l'objet (rue des Pyrénées à Paris avant rue des Pyrénées à Argenteuil)
 * Importance de l'objet (une station de métro avant une station de bus, un boulevard avant une rue)
 * Taille de l'objet (nombre de lignes passant par le stopArea, longueur de la rue…)
 * Priorisation des lieux remarquables (POI) par leur type (lieu touristique avant parc d'attraction)

Note des objets à la requête
----------------------------

La pertinence des résultats dépend bien évidemment de la requête de l'utilisateur. Ce qui influence la pertinence du résultat est :

 * Nombre de mots matchés (en demandant « rue jacques » favoriser « rue saint Jacques » à « rue du faubourg Saint Jacques »
 * Nombre de caractères manquant (en demandant « rue py », favoriser « rue de la py  » à « rue des pyrénées »
 * Adresstype présent dans la requête : adresse, zone d'arrêt (rue des Pyrénées avant métro Pyrénées), POI, point d'arrêt, admin (quartier, arrondissement ou ville)
 * Position géographique (« gare de lyon » n'a pas le même sens si on habite à Lyon ou à Paris)


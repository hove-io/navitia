/** \page navimake Navimake

Navimake est (en attendant de trouver un meilleur nom) l'outil permettant de "binariser" les données.

Pour l'instant on supporte deux formats
 - export CSV de fusio
 - GTFS

L'ensembles des structures de données sont dans le namespace navimake.
Les types de données TC, sont dans le namespace navimake::types

La structure contenant toutes les données TC, est navimake::Data. Cette
structure contient des vecteurs de pointeurs vers chaque type TC.
Les relations entre les éléments sont définis par des pointeurs.
Tout élément inséré dans un tableau sera détruit. Cependant l'allocation reste à la
charge du conncteur. Il devra par exemple faire :

\code
navimake::types::Network * network = new navimake::types::Network();
data.networks.push_back(network);
\endcode

Afin de définir un nouveau constructeur, il faut créer une classe qui remplit la structure
navimake::Data avec toutes les informations possibles (à part le idx).

Les opérations effectuées automatiquement sont :
 - nettoyage des données (navimake::Data::clean)
 - trie les tableaux (navimake::Data::sort). Le critère de tri utilisé est l'operateur < de chaque type navimake
 - indexe les éléments tout de suite après le tri. Le membre "idx" est numéroté de 0 à n-1
 - transforme les données navimake en données navitia. Les données sont desormais stockées sous forme de tableau (et non plus un tableau de pointeurs)
 - construction des relation (navimake::Data::build_relations) à des fins de performance (avoir tous les StopPoint d'un StopArea)

 */


/** \mainpage NAViTiA C++
 *
 * \section orga Organisation du dépot
 *
 * À la racine du dépot se trouvent différents repertoires :
 * - source : contient le code source produit par Canal TP
 * - third_party : modules complémentaires développés par des tiers
 * - Doc : la documentation (que vous êtes en train de lire) qui est générée à partir du code source avec Doxygen
 * - (debug) : conventionnellement, le repertoire de build de la version debug
 * - (release) : conventionnellement, le repertoire de build de la version release
 *
 * \section outils Outils utilisés
 * - GCC pour son compilateur C++ (g++)
 * - Visual Studio pour la compilation sous windows
 * - Doxygen pour générer la documentation à partir du code source
 * - CMake pour gérer le système de build
 *
 * \section libs Bibliothèques utilisées
 * - Boost : très largement utilisé pour de nombreux besoins en C++
 *      - serialize pour "binariser" les données
 *      - graph pour la représentation de graphes et des calculs simples type Dijkstra
 *      - thread
 *      - iostream pour compresser à la volée des flux
 *      - date_time
 *      - spirit pour parser du texte
 * - cURL : pour les requêtes http
 * - protocol buffers : sérialisation pour échanger des données entre différents services
 * - fastlz : compression extrèmement rapide mais pas nécessairement très performante
 *
 * \section cmake_use Utilisation de CMake
 * CMake permet de compiler dans un repertoire différent des sources pour ne pas les mélanger.
 * La convention veut que l'on mette deux repertoires pour chaque type de build à la racine :
 * - mkdir debug; cd debug; cmake -D BUILD_TYPE=Debug ../source
 * - mkdir release; cd release; cmake -D BUILD_TYPE=Release ../source
 *
 * Sous Visual Studio, le fonctionnement est un peu différent puisque VS génère des sous-repertoires pour chaque type de compilation
 *
 * \section modules Modules
 * Chaque module existe dans un sous-repertoire de source
 *  - \ref WS_Commons : squelette pour construire un webservice fcgi/isapi
 *  - \ref fare : calcul des tarifs d'un trajet
 *  - \ref first_letter : indexation de chaînes de caractères pour retrouver rapidement un élément
 *  - \ref gateway : une passerelle plus modulaire adaptée au cloud
 *  - log : expérimentations des logs
 *  - \ref navimake : transformer des exports fusion .csv dans le format interne
 *  - \ref ptreferential : effectuer des requêtes sur le référentiel de données
 *  - \ref type : structures de données liées au référentiel de transports en communs
 *  - \ref street_network : référentiel de données et algorithmes sur le filaire de voirie 
 *  - utils : petits outils qui peuvent servir (lire un CSV, soucis d'encodage…)
 * 
 */

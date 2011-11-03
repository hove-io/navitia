/** \page WS_Commons WS_Commons

\section ws_intro Introduction
Il s'agit d'un squelette permettant de créer des applications web pour un déploient en ISAPI ou FastCGI.
De plus à des fins de débogage, une interface Qt est proposée.

\section data_structures Structure de données
Tout webservice se compose deux classes à définir par le développeur du webservice :
 - La classe spécifique à tout le webservice. Cette classe ne sera instanciée qu'une seule fois.
 - La classe spécifique à chaque thread. Cette classe sera donc instanciée autant de fois qu'il y a de threads.

La dernière ligne du fichier définissant ces deux classes doit être : MAKE_WEBSERVICE(Data, Worker)

Selon la configuration de CMake, cela génèrera soit une application Qt, un executable FastCGI ou une DLL isapi.

\section webserviceclass Classe de webservice

L'unique contrainte sur cette classe est de posséder une variable nb_threads qui indique le nombre de threads à lancer.
Dans la suite de la documentation, nous appellerons cette classe Data.

\section threadclass Classe de thread

La classe doit hériter de BaseWorker<Data> où Data est la classe de webservice.
Cette classe contient au moins autant de fonctions que d'API à implémenter.

Par la suite nous appelleront cette classe Worker. 

\section declareapi Déclarer une nouvelle API

Une API est une fonction de Worker qui prend en entrée un RequestData et Data pour retourner ResponseData.

Chaque API doit être déclarée dans le constructeur de Worker avec la méthode BaseWorker::register_api. On Définit alors le nom
de l'API, la fonction à appeler ainsi que la description de l'API.

\section declareparams Déclarer des paramètres

Afin de permettre de vérifier la validité des paramètres d'un appel à l'API et pour documenter l'API, il est conseillé de renseigner tous
les paramètres possibles. Cela se fait avec la méthode BaseWorker::add_parameter.

L'appel à add_parameter doit être effectué dans le constructeur de Worker.

\section ws_configuration Paramétrage

Le singleton Configuration permet de récupérer des paramètres liée au webservice, en particulier le nom de l'application avec la clef "application" et le chemin où elle se trouve
avec la clef "path".

\section ws_example Exemple
Un exemple de webservice peut être trouvé dans le fichier example.cpp qui compte le nombre de fois que l'API a été appelée et combien de fois chaque thread a été appelé.

\section defaultapi APIs par défaut

En appelant la fonction BaseWorker<Data>::add_default_api dans le constructeur de Worker, plusieurs API seront rajoutées par défaut :
 - help : liste toutes les APIs disponibles et les paramètres
 - analyze : vérifie que tous les paramètres obligatoire d'une API sont renseignés et si les valeurs sont conformes
*/

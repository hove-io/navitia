Guide du développeur
====================

Ce chapitre s'adresse à toute personne devant travailler sur le code source
de NAViTiA.

Architecture générale
*********************

navitia se compose du cœur appelé *kraken* et du frontend appelé *Jörmungandr*.

Le cœur est développé en C++ et le frontend en python.

Les deux composants communiquent entre eux au travers de message *protocol buffers* transmis avec
`ZMQ <http://www.zeromq.org/>`_.

Afin de gérer des requêtes multiples, kraken instancie *n* threads, chacun d'entre eux est indépendant


Tests unitaires
***************

Les tests unitaires visent à être plus serein lors d'une modification du code. Contrairement aux tests fonctionnels (effectués par Artemis), les tests unitaires sont *techniques*.
Ils ont pour but
de tester une fonctionnalité la plus précise au possible et non pas un comportement de bout en bout.

Le framework de test utilisé est boost::test.

Les tests sont typiquement écrits à trois moments différents du cycle de vie du code:

#. À la conception *papier* d'un composant : on réfléchit aux fonctionnalités d'une fonction. Les tests permettent d'illustrer le comportement nominal
#. Pendant l'implémentation du composant afin de tester les conditions aux limites (valeur négative, liste vide, etc.)
#. En cas de remontée de bug : on commence par écrire un test qui rate avant de faire une correction qui fait passer le test

Se baser sur des tests peut, voire doit, influencer sur la manière d'écrire le code : il faut que les fonctionnalités puissent être testées avec le minimum de code « auxiliaire ».
Cela peut nécessiter de :

* rendre les fonction plus autonomes (pas besoin d'un gros objet compliqué à initialiser)
* améliorer les constructeurs pour avoir un objet *utilisable* le plus rapidement
* créer des fonctions annexes permettant de construire rapidement l'objet à tester

Il y a deux pièges classiques à éviter :

* rechigner à tester des choses *évidentes*. C'est pourtant là qu'une faute d'étourderie arrive le plus rapidement (par exemple utiliser i au lieu de i-1)
* vouloir tester un comportement fonctionnel (avec un enchaînement d'actions)

Dépôt de code
*************

Nous utilisons GIT. La branche de développement est ``dev``. Cependant le travail se fait sur une branche à part.


Normes de codage
****************

Les normes ne sont pas à prendre à la lettre. Cependant voici quelques lignes générales.

Règles globales
---------------

.. _`guidelines google`: http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml

Dans l'ensemble les `guidelines google`_ sont très bien. Nous ne les appliquons cependant pas à la lettre
Les grandes différences portent sur :

* Boost : on utilise tout ;)
* Les exception : on utilise
* C++11 : on utilise ce qui est supporté par GCC 4.6

Quant à la mise en forme du code, les règles Qt seront à suivre http://wiki.qt-project.org/Coding_Conventions (par contre il faut penser à ignorer les remarques très spécifiques à Qt)

Quelques sites toujours utiles
------------------------------

* Documentation sur la bibliothèque standard http://www.cplusplus.com/reference/stl/
* http://en.wikipedia.org/wiki/C%2B%2B11 Les nouveautés 2011 du C++, tout n'est pas encore supporté, mais presque tout http://gcc.gnu.org/projects/cxx0x.html
* Boost : la super bibliothèque à presque tout faire http://www.boost.org/doc/libs/1_46_0/doc/html/

Quelques règles sur le pouce pour coder en c++
----------------------------------------------

Pour ceux qui arrivent du delphi
++++++++++++++++++++++++++++++++

Le test d'égalité c'est `==` et non pas `=` comme en Delphi.

.. warning::
    L'opérateur = est chaînable pour pouvoir faire `a = b = 42`. Les conséquences peuvent donc être désatreuses dans du code si on utilise = dans un `if`

::

  if(a=1) // a=1 vaut 1, donc le test est toujours vrais
  if(a=0) // a=0 vaut 0, donc le test est toujours faux

Éviter d'utiliser des fonctions C. Par exemple faire . Cela d'autant plus important pour les accès systèmes (fichiers, réseau) où cela peut entraîner des fuites de mémoire ::

  if(str == "hello") // Bien
  if(strcmp(str.c_str(), “Hello”)==0) // pas bien


Connaître les constructeur par défaut. Pour les types C (int, double, pointeurs...) par défaut la valeur est aléatoire ::

  int a; // a peut valoir tout et n'importe quoi
  std::string str; // Str est une chaîne vide. Il n'y a pas besoin d'initialiser

Penser aux template si vous avez du code très similaire dont uniquement le type des paramètres change

Ne pas utiliser des techniques du C :
 * sizeof
 * casts à la C : Type t = (Type) other; On ne devrait jamais avoir besoin de caster (transtyper). Si on doit le faire, utiliser les casts C++

On peut déclarer et initialiser en une seule fois ::

  std::string hello = "World !";

Passages de paramètres dans les fonctions
+++++++++++++++++++++++++++++++++++++++++

* L'idéal est que l'appel d'une fonction ne puisse pas modifier les paramètres
* Éviter — dans la mesure du possible — les pointeurs
* Penser aux consts

 * Ça s'applique aux références (on ne copie pas le paramètre, mais on ne le modifie pas) : `bool my_fonction(const Objet & obj)`;
 * Ça peut s'appliquer à une méthode (ne modifie aucun membre de l'instance de la classe) : `bool Objet::is_valid() const`;
 * Pour des types primitifs (int, double...) pas besoin de référence (on ne perd rien à copier un entier), donc pas besoin de const

* Si une fonction doit modifier un paramètre, peut-être que la fonction devrait être un membre de l'objet
* *Ne pas avoir peur des copies des paramètres* :P Si on découvre un problème de performances, ça sera le plus simple à résoudre

Programmation objet
+++++++++++++++++++

Une fonction qui ne lit et ne modifie aucun attribut d'un objet, n'a pas besoin d'être une fonction membre de l'objet.

Des choses plus bizarres du C++
+++++++++++++++++++++++++++++++

* Certaines opérations sont atomiques et pas besoin de mutex, par exemple int i = 0; i++;
* Faire très attention au mot clef static ! La déclaration dans un .h ne suffit pas. Il faut l'initialiser dans _un_ .cpp également
* Éviter les tautologies sur les noms de variable : class Moo { int moo_count;} C'est évident que count est dans moo. Il n'y a pas besoin de le répéter. Les classes existent pour ça ;)
* Limitez les #include au possible (temps de compilation !) et essayez de les mettre dans les .cpp au lieu de .h (pour éviter une inclusion accidentelle dans un .cpp où il ne servirait à rien

Des outils d'analyse
--------------------

* Valgrind, boite à outils qui sert à tout (attention, il est génial, mais extrêmement lent)

 * Analyse des fuites de mémoire et violation d'accès; outil par défaut
 * Mesure de performance (profiling); outil callgrind, avec kcachegrind pour la visualisation
 * Problèmes de concurrence (écriture par deux thread simultanément de la même variable…); outil helgrind
 * Consommation de mémoire; outil massif

* Google Perftools : analyse de performance CPU, mémoire. Plus rapide que Valgrind, mais plus compliqué à mettre en œuvre

Documentation
-------------
* Privilégiez la description de ce que font les fonctions dans le .h. Chaque fonction doit être commentée de manière à permettre une extraction par Doxygen.
* Dans le .h, nommez les paramètres. Cela permet à IDE de donner des informations quant à l'appel des fonctions.

::

    string convert(string source, string destination); // bien
    string convert(string, string); // pas bien

Les choses chiantes en C++
--------------------------

Tout n'est pas parfait :)

* Un langage particulièrement complexe
* Un compilateur très lent
* Des messages d'erreur du compilateur imbitable
* Des IDE moyens (à cause des problème pré-cités)
* Les consts c'est bien, mais ça peut vite donner mal à la tête

Divers
------

* http://xkcd.com/303/
* « C++ does not have a compiler, it's got a complainer » (auteur inconnu)


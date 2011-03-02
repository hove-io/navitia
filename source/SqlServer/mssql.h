#pragma once
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <iostream>
#include <sqlfront.h>	/* sqlfront.h always comes first */
#include <sybdb.h>	/* sybdb.h is the only other file you need */

#include <stdint.h>

/** Contient des wrappers pour accéder aux bases SQL*/
namespace Sql {
    /// Tableau de chaînes de charactères
    typedef std::vector<std::string> StringVect;

    /** Exception lancée en cas de problème SQL */
    struct SqlException : public std::exception {
        std::string str; ///< Message d'erreur lié à l'exception
        /// constructeur avec un message d'erreur
        SqlException(const std::string & str) : str(str){};
        /// Accesseur pour connaître le message de l'exception
        virtual const char* what() const throw() {
            return str.c_str();
        }
        virtual ~SqlException() throw() {}
    };

    /** Structure intermédiaire pour construire un appel à une procédure stockée
     *
     * N'est pas destiné à être utilisé directement
     */
    class RegisteredProcedure {
        DBPROCESS * dbproc; ///< Connecteur vers la base de données
        std::vector<int *> integers; ///< Liste des entiers alloués sur le tas à détruire une fois la requête finie
        std::vector<std::string*> strings; ///< Liste des chaînes de charactères alloués sur le tas à détruire une fois la requête finie
        private:
        /// Le constructeur est privé pour éviter un utilisateur trop malin/idiot veuille l'utiliser
        /// La conséquence serait imprévisible (problème de pointeurs invalides dans la nature)
        /// (mais au moins y'a pas de fuite de mémoire)
        RegisteredProcedure(DBPROCESS * dbproc, const std::string & procedure_name);
        RegisteredProcedure(const RegisteredProcedure &){};
        /// L'opérateur est desactivé tout comme les constructeurs;
        void operator=(const RegisteredProcedure &){};
        public:
        ~RegisteredProcedure();
        /// On rajoute un argument de type entier
        RegisteredProcedure & arg(int i);
        /// On rajoute un argument de type chaine de caractères
        RegisteredProcedure & arg(const char * str);

        /// Seul MSSql et Result le droit d'appeler le constructeur et accéder aux membres
        friend class MSSql;
        friend class Result;


    };

    /** Contient les réponses d'une requête */
    class Result {
        public:
        /// Contient une ligne et permet d'accéder par n° de colonne ou nom de colonne
        struct Row {
            /// Pointeur vers la structure de données
            Result * result;

            /// Contient les données
            StringVect data;

            Row(Result * result) : result(result) {}

            /// Retourne la colonne par indexe
            /// Attention, pour des raisons de performances
            std::string operator[](int idx){return data[idx];}

            /// Retourne la colonne par nom. Attention ! Pour des raisons de performances,
            /// la valeur n'est pas vérifiée
            std::string operator[](std::string name)
            {
                return data[result->map[name] - 1];
            }

            /// Retourne l'indexe d'une colonne donnée par le nom
            /// Lève une exception si la clef n'existe pas
            int get_index(const std::string & name);
        };

        /** Permet d'itérer sur les résultats */
        class iterator{
            public:
            typedef Row value_type; ///< Type vers lequel pointe l'itérateur
            typedef Row* pointer; ///< Type pointeur vers lequel pointe l'itérateur
            typedef Row& reference; ///< Type référence vers lequel pointe l'itérateur
            typedef std::forward_iterator_tag iterator_category; ///< Type d'itérateur : on ne peut aller que de l'avant
            typedef int difference_type; ///< Type pour mesurer la distance entre deux itérateurs
            private:
            /// Pour savoir sur quoi pointe l'itérateur
            Result * result;
            /// Données courantes
            Row data;
            /// Nombre de lignes ayant été lues
            int read_rows;
            private:
            /// Permet de récupérer une ligne
            void get_row();
            public:
            /// Par défaut, c'est un itérateur pour lequel il n'y a pas d'autre résultat (aka == .end())
            iterator() : result(NULL), data(NULL), read_rows(0){}
            /// Construteur à partir d'un pointeur de Result
            iterator(Result * result) : result(result), data(result), read_rows(0) {get_row();};
            /// On va chercher l'élément suivant
            iterator & operator++(){get_row(); return *this;};
            /// On teste uniquement les deux itérateurs pointes sur NULL => .end()
            bool operator==(const iterator & it) {return result == NULL && it.result == NULL;};
            /// On teste uniquement les deux itérateurs pointes sur NULL => .end()
            bool operator!=(const iterator & it) {return result != NULL || it.result != NULL;};
            /// On obtient les résultats sous forme de tableau de string
            value_type & operator*() {return data;};
            /// Pour simmuler un pointeur
            value_type * operator->(){return &data;};
        };

        /// Iterateur sur les résultats
        typedef iterator const_iterator;
        /** Structure utilisée pour récupérer les données au niveau de la lib C */
        struct COL {
            char *name; ///< Nom de la colonne
            char *buffer;  ///< Buffer contenant la valeur de la colonne
            int type; ///< Type de la valeur contenue dans buffer
            int size; ///< Taille de la donnée contenue dans buffer
            int status; ///< Indique si on a bien réussi à se binder à la colonne
        };
        COL * columns; ///< Ensemble des colonnes
        COL * pcol; ///< Pointeur vers une colonne
        
        /// Nombre de colonnes
        int ncols;

        /// Connecteur de l'API C vers la base
        DBPROCESS * dbproc;

        /// Initialise la structure : aloue la mémoire pour les structures
        void init();
        /// Initialise à partir d'une procédure stockée
        void init(const RegisteredProcedure & proc);

        /// Fait correspondr les noms de colonnes avec l'indexe
        std::map<std::string, int> map;

        private:
        /// Desaloue les structuers
        void cleanup();
        public:
        /// Il faut passer le DBPROCESS à lire pour construire une réponse
        Result(DBPROCESS * dbproc);
        /// Constructeur à partir d'un RegisteredProcedure
        Result(const RegisteredProcedure & proc);
        /// Constructeur par copie
        Result & operator=(const Result & result);
        /// Constructeur à partir d'un RegisteredProcedure
        Result & operator=(const RegisteredProcedure & proc);
        /// Nombre de colonnes retournées par la requete
        int num_cols() const {return ncols;};
        /// Nom des colonnes
        StringVect col_labels() const;
        /// Itérateur sur le début des réponses. Attention ! Ne peut être appelé qu'une seule fois !
        iterator begin(){return iterator(this);};
        /// Itérateur indiquant qu'il n'y a plus de réponses
        iterator end(){return iterator();};
        /// Destructeur
        ~Result();
    };

    /** Connecteur vers une base Microsoft SQL Server 
     *
     * Très important : modifier le fichier /etc/freetds/freetds.conf pour avoir :
     * [global]
     * tds version = 8.0
     *
     * Merci de regarder l'API de sybase et venez nous remercier après...
     * Tutoriel pour les curieux http://www.freetds.org/userguide/samplecode.htm
     * */
    class MSSql {
        LOGINREC *login;
        DBPROCESS *dbproc;
        public:
        /** Se connecte au serveur et à la base donnée
         * De préférence utiliser le même connecteur tout le long de l'application et non pas une fois par requête
         * 60 octets de fuite de mémoire lié à la lib pour chaque instance.
         *
         * Ce connecteur n'est pas thread-safe. Créer un connecteur par thread
         */
        MSSql(const std::string & host, const std::string & name, const std::string & passwd, const std::string & db);
        ~MSSql();
        /// Exécute une requête, attention à échapper les paramètres !
        Result exec(const std::string & query);
        /** Execute une procédure stockée.
         *
         * Affecter les resultat dans une structure Result
         * Rajouter des paramètres avec la fonction arg
         * Ne pas essayer de garder RegisteredProcedure (on vous aura prévenu)
         *
         * Ex: Result res = conn.exec_proc("ma_procedure_stockee").arg(42).arg("foo");
         */
        RegisteredProcedure exec_proc(const std::string & procedure_name);
    };

}

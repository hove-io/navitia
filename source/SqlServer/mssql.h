#include <string>
#include <vector>
#include <exception>
#include <iostream>
#include <sqlfront.h>	/* sqlfront.h always comes first */
#include <sybdb.h>	/* sybdb.h is the only other file you need */

namespace Sql {
    typedef std::vector<std::string> StringVect;

    /** Exception lancée en cas de problème SQL */
    struct SqlException : public std::exception {
        std::string str;
        SqlException(const std::string & str) : str(str){};
        virtual const char* what() const throw() {
            return str.c_str();
        }
        virtual ~SqlException() throw() {}
    };

    /** Contient les réponses d'une requête */
    class Result {
        public:
        /** Permet d'itérer sur les résultats */

        class iterator{
            public:
            typedef StringVect value_type;
            typedef StringVect* pointer;
            typedef StringVect& reference;
            typedef std::forward_iterator_tag iterator_category;
            typedef int difference_type;
//            private:
            /// Pour savoir sur quoi pointe l'itérateur
            Result * result;
            /// Données courantes
            StringVect data;
            /// Nombre de lignes ayant été lues
            int read_rows;
            private:
            ///Permet de récupérer une ligne
            void get_row();
            public:
            /// Par défaut, c'est un itérateur pour lequel il n'y a pas d'autre résultat (aka == .end())
            iterator() : result(NULL), read_rows(0){}
            iterator(Result * result) : result(result), read_rows(0) {get_row();};
            iterator(const iterator & it) : result(it.result), read_rows(it.read_rows) {};
            /// On va chercher l'élément suivant
//            iterator & operator++(){get_row(); return *this;};
            void operator++(){get_row();};
            /// On teste uniquement les deux itérateurs pointes sur NULL => .end()
            bool operator==(const iterator & it) {std::cout << (result == NULL && it.result == NULL) << std::endl;return result == NULL && it.result == NULL;};
            bool operator!=(const iterator & it) {std::cout << "Opé != " << std::endl;return result != NULL || it.result != NULL;};
            /// On obtient les résultats sous forme de tableau de string
            value_type & operator*() {return data;};
            ~iterator(){std::cout << "Ds le destructeur" << std::endl;};
        };

        typedef iterator const_iterator;
        /** Structure utilisée pour récupérer les données au niveau de la lib C */
        struct COL 						
        { 
            char *name;
            char *buffer; 
            int type, size, status; 
        } *columns, *pcol;
        
        /// Nombre de colones
        int ncols;

        /// Connecteur de l'API C vers la base
        DBPROCESS * dbproc;

        public:
        /// Il faut passer le DBPROCESS à lire pour construire une réponse
        Result(DBPROCESS * dbproc);
        /// Nombre de colonnes retournées par la requete
        int num_cols() const {return ncols;};
        /// Nom des colonnes
        StringVect col_labels() const;
        /// Itérateur sur le début des réponses. Attention ! Ne peut être appelé qu'une seule fois !
        iterator begin(){return iterator(this);};
        /// Itérateur indiquant qu'il n'y a plus de réponses
        iterator end(){return iterator();};
        /// Destructeur
        ~Result();
    };

    /** Connecteur vers une base Microsoft SQL Server */
    class MSSql {
        LOGINREC *login;
        DBPROCESS *dbproc;
        private:
        /// Retourne 
        StringVect get_headers();
        public:
        /// Se connecte au serveur et à la base donnée
        MSSql(const std::string & host, const std::string & name, const std::string & passwd, const std::string & db);
        ~MSSql();
        /// Exécute une requête, attention à échapper les paramètres !
        Result exec(const std::string & query);
    };
};

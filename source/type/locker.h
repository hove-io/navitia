#pragma once

namespace navitia{ namespace type{
    class Data;

    /**
     * structure permettant de simuler un finaly afin
     * de délocké le mutex en sortant de la fonction appelante
     */
    struct Locker{
        /// indique si l'acquisition du verrou a réussi
        bool locked;
        navitia::type::Data* data;

        ///construit un locker par défaut qui lock rien.
        Locker();

        /**
         * @param exclusive défini si c'est un lock exclusif ou pas
         *
         *
         */
        Locker(navitia::type::Data& data, bool exclusive = false);

        ~Locker();

        /**
         * move constructor 
         */
        Locker(Locker&& other);

        private:
        /// on désactive le constructeur par copie
        Locker(const Locker&);
        /// on l'opérateur d'affectation par copie
        Locker& operator=(const Locker&);

        bool exclusive;
    };

}}

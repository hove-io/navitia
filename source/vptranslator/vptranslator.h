/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/type.h"
#include <list>


template<class T>
unsigned int levenshtein_distance(const T &s1, const T & s2) {
        const size_t len1 = s1.size(), len2 = s2.size();
        std::vector<unsigned int> col(len2+1), prevCol(len2+1);

        for (unsigned int i = 0; i < prevCol.size(); i++)
                prevCol[i] = i;
        for (unsigned int i = 0; i < len1; i++) {
                col[0] = i+1;
                for (unsigned int j = 0; j < len2; j++)
                        col[j+1] = std::min( std::min( 1 + col[j], 1 + prevCol[1 + j]),
                                                                prevCol[j] + (s1[i]==s2[j] ? 0 : 1) );
                col.swap(prevCol);
        }
        return prevCol[len2];
}

class MakeTranslation
{
public:
    //structure permettant de stocker la condition de service sous forme de semaine
    struct week{
        int weeknumber;
        boost::gregorian::date startdate;
        std::bitset<7> week_bs;
        bool operator==(week i2);
        bool operator!=(week i2);
    };
    //structure representant un motif
    struct target{
        std::bitset<7> week_bs;
        unsigned long week_ulong;
        int count;
        int firstweeknumber;
        int lastweeknumber;
        std::vector<boost::gregorian::date> periodlist;
        std::vector<boost::gregorian::date> andlist;
        std::vector<boost::gregorian::date> exceptlist;
        bool operator<(target i2) const;
        bool operator!=(target i2) const;
    };
    //date de début effective de la condition de service
    boost::gregorian::date startdate;
    //date de fin effective de la condition de service
    boost::gregorian::date enddate;
    //condition de service
    std::string CS;
    //stockage des semaines
    std::vector<week> week_vector;
    //stockage des motifs
    std::map<int, target> target_map;

    int getnextmonday(boost::gregorian::date datetocompare, short sens);
    std::vector<target>::iterator getbesthost(std::vector<target> &targetvector, const target &targetsrc);

    std::bitset<7> getandpattern(std::bitset<7> &b1, std::bitset<7> &b2);
    std::bitset<7> getexceptpattern(std::bitset<7> &b1, std::bitset<7> &b2);

    //élimination des jours inactifs en début et fin de la condition de services
    //recalcul des dates de début et de fin
    bool initcs(boost::gregorian::date beginningday, std::string requestedcs);
    //découpage de la consition de services en semaines (du lundi au dimanche
    void splitcs();
    //calcul des période d'application de chaque motif
    void translate();
    //inclusion des semaines incomplètes de début et de fin sur leur semaine adjacente
    void bounddrawdown();
    void targetdrawdown();
    void mergetarget(target &targetsrc, target &targetdst);
    MakeTranslation();

};


//class MakeTranslation_test: MakeTranslation
//{
//    boost::gregorian::date startdate;
//    boost::gregorian::date enddate;
//    std::string CS;
//    bool initcs(boost::gregorian::date beginningday, std::string requestedcs);
//    void splitcs();
//    int GetNextMonday(boost::gregorian::date beginningday, std::string requestedcs, short int sens);
//    MakeTranslation();
//};

#pragma once

#include "type/type.h"
#include <list>


struct vptranslator
{
    struct cible{
        int week;
        std::bitset<7> week_bs;
        std::string week_st;
        int count;
        int firstweeknumber;
        int lastweeknumber;

    };
    std::map<std::string, cible> cibles;
    void initcibles();

};

class MakeTranslation
{
public:
    struct week{
        int weeknumber;
        boost::gregorian::date startdate;
        std::bitset<7> week_bs;
    };

    struct target{
        std::bitset<7> week_bs;
        bool used;
        int count;
        int firstweeknumber;
        int lastweeknumber;
        std::vector<boost::gregorian::date> periodlist;
        std::vector<boost::gregorian::date> andlist;
        std::vector<boost::gregorian::date> exceptlist;
    };

    boost::gregorian::date startdate;
    boost::gregorian::date enddate;
    std::string CS;
    std::vector<week> week_vector;
    std::map<int, target> target_map;
    bool initcs(boost::gregorian::date beginningday, std::string requestedcs);
    void splitcs();
    int getnextmonday(boost::gregorian::date datetocompare, short sens);
    int getbesttarget();
    void translate();
    void bounddrawdown();
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

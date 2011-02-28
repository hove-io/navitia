#include <iostream>
#include "mssql.h"
#include <boost/foreach.hpp>

int main(int, char**){
    // Test de la connexion
    Sql::MSSql conn("10.2.0.16", "sa", "159can87*", "testTDS");

    // Test de requête simple
    Sql::Result res = conn.exec("SELECT * from foo");
    BOOST_FOREACH(Sql::Result::Row row, res){
        std::cout << row[0] << " - " << row[1] <<std::endl;
        std::cout << row["id"] << " " << row["name"] << std::endl;
    }

    std::cout << "Test 1, ok" << std::endl;

    // Test de réutilisation d'un Sql::Result
    res = conn.exec("SELECT * from foo");
    std::cout << (*res.begin())[0] << " " << (*res.begin())[1] << std::endl;
    std::cout << "Test 2, ok" << std::endl;

    // Test de réutilisation d'un Sql::Result qui n'a pas été fini
    res = conn.exec("SELECT * from foo");
    BOOST_FOREACH(Sql::Result::Row row, res){
        std::cout << row[0] << " " << row[1] << std::endl;
    }
    std::cout << "Test 3, ok" << std::endl;

    // Test d'une procédure stockée
    Sql::Result res2 = conn.exec_proc("insert_and_ret").arg(42).arg("coucou");
    BOOST_FOREACH(Sql::Result::Row row, res2) {
        std::cout << row[0] << " " << row[1] << std::endl;
    }
    std::cout << "Test 4, ok" << std::endl;

    res = conn.exec("SELECT * from foo");
    // Test d'une procédure stockée en utilisant le même Sql::result
    res = conn.exec_proc("insert_and_ret").arg(43).arg("coucouaiue");
    std::cout << (*res.begin())[0] << " " << (*res.begin())[1] << std::endl;
    std::cout << "Test 5, ok" << std::endl;

    // Test d'une procédure stockée sur un Sql::result mal fini
    res = conn.exec_proc("insert_and_ret").arg(43).arg("coucouaiue");
    res = conn.exec_proc("insert_and_ret").arg(43).arg("coucouaiue");
    res.begin();
    res = conn.exec_proc("insert_and_ret").arg(43).arg("coucouaiue");
    std::cout << "Test 6, ok" << std::endl;

    // Test d'un itérateur sur résultat vide
    Sql::Result res3 = conn.exec("Select * from foo where 1 = 0");
    BOOST_FOREACH(Sql::Result::Row row, res3) {
        std::cout << row[0] << " " << row[1] << std::endl;
    }
    std::cout << "Test 7, ok" << std::endl;
    return 0;
}





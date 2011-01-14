#include "indexes.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign.hpp>
#include <vector>

struct Stop {
    int id;
    std::string name;
    std::string city;
    Stop(int id, const std::string & name, const std::string & city) : id(id), name(name), city(city) {}
};

struct Line {
    std::string name;
    float duration;
    Line(const std::string & name, float duration) : name(name), duration(duration) {}
};

struct City {
    std::string name;
    std::string country;
    City(const std::string & name, const std::string & country) : name(name), country(country) {}
};

struct LineStops {
    int stop_id;
    std::string line_name;
    int order;
    LineStops(int stop_id, const std::string & line_name, int order): stop_id(stop_id), line_name(line_name), order(order) {}
};

struct Job {
    int job_id;
    std::string label;
    float salary;
    Job(int job_id, const std::string & label, float salary) : job_id(job_id), label(label), salary(salary) {}
};

struct Employee {
    std::string name;
    int age;
    bool is_manager;
    int job_idx;
    Employee(const std::string & name, int age, bool is_manager, int job_idx) :
            name(name), age(age), is_manager(is_manager), job_idx(job_idx) {}
};

using namespace boost::assign;

int main(int, char**) {
    std::vector<Job> jobs;
    std::vector<Employee> employees;
    std::vector<Stop> stops;
    std::vector<Line> lines;
    std::vector<City> cities;
    std::vector<LineStops> line_stops;

    jobs += Job(0, "Developper", 42),
            Job(1, "Trainee", 3.14),
            Job(2, "Manager", 66);

    employees += Employee("Alice", 32, false, 2),
                 Employee("Bob", 44, true, 1),
                 Employee("Charlie", 12, false, 0),
                 Employee("Deb", 51, false, 0);

    stops += Stop(0, "Gare de l'Est", "Paris"),
             Stop(1, "Gare d'Austeriltz", "Paris"),
             Stop(2, "Westbahnhof", "Wien"),
             Stop(3, "Hauptbahnof", "Stuttgart"),
             Stop(4, "Centre du monde", "Perpinyà"),
             Stop(5, "Sants", "Barcelona");

    lines += Line("Orient Express", 15.5),
             Line("Joan Miró", 12.1);

    cities += City("Paris", "France"),
              City("Wien", "Österreich"),
              City("Stuttgart", "Deutschland"),
              City("Barcelona", "España"),
              City("Pepinyà", "France");

    line_stops += LineStops(0, "Orient Express", 0),
                  LineStops(3, "Orient Express", 1),
                  LineStops(2, "Orient Express", 2),
                  LineStops(1, "Joan Miró", 1),
                  LineStops(5, "Joan Miró", 66),
                  LineStops(4, "Joan Miró", 42);



    std::cout << "Salaries per job" << std::endl;
    BOOST_FOREACH(Job j, jobs){
        std::cout << j.label << ": " << j.salary << "€" << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    std::cout << "Salaries per job sorted by jobname" << std::endl;
    BOOST_FOREACH(Job j, order(jobs, &Job::label)){
        std::cout << j.label << ": " << j.salary << "€" << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    std::cout << "List of managers" << std::endl;
    BOOST_FOREACH(Employee e, filter(employees, &Employee::is_manager, true)){
        std::cout << e.name << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    std::cout << "List of employees and job" << std::endl;
    auto employees_job = make_join(employees, jobs, attribute_equals(&Employee::job_idx, &Job::job_id));
    BOOST_FOREACH(auto ej, employees_job){
        std::cout << join_get<0>(ej).name << ": " << join_get<1>(ej).label << std::endl;
    }
    std::cout << "-------------------\n" << std::endl;

    std::cout << "=== Cities ordered by name ===" << std::endl;
    BOOST_FOREACH(auto city, order(cities, &City::name)){
        std::cout << "  * " << city.name << " (" << city.country << ")" << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::cout << "=== Stations of Paris ===" << std::endl;
    BOOST_FOREACH(auto stop, filter(stops, &Stop::city, std::string("Paris"))){
        std::cout << "  * " << stop.name << std::endl;
    }
    std::cout << std::endl << std::endl;

  /*  std::cout << "=== Stops of line Orient Express ===" << std::endl;
    auto join = make_join(stops,
                          filter(line_stops, &LineStops::line_name, std::string("Orient Express")),
                          attribute_equals(&Stop::id, &LineStops::stop_id));
    BOOST_FOREACH(auto stop, join){
        std::cout << "  * " << join_get<0>(stop).name << " " << join_get<1>(stop).order << std::endl;
    }
*/
    return 0;
}

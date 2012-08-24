#include <stdint.h>
#include <vector>
#include <string>

#include <geos_c.h>

struct Node {
    uint64_t id;
    double lon;
    double lat;
    Node() : id(0), lon(0), lat(0){}
};

struct Admin{
    uint64_t id;
    int level;
    std::string name;
    Node center;
    GEOSGeometry * geom;

    Admin() : geom(nullptr) {}

    Admin(const Admin & other) : id(other.id), level(other.level), name(other.name), center(other.center) {
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
    }

    Admin(Admin && other) : id(other.id), level(other.level), name(other.name), center(other.center) {
        this->geom = other.geom;
        other.geom = nullptr;
    }

    Admin & operator=(const Admin & other){
        this->id = other.id;
        this->level = other.level;
        this->name = other.name;
        this->center = other.center;
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
        return *this;
    }

    ~Admin() {
        if(geom != nullptr)
            GEOSGeom_destroy(geom);
    }
};

struct Way {
    Node center;
    uint64_t id;
    std::string name;
    std::vector<Node> nodes;
    std::vector<size_t> admin;
    GEOSGeometry * geom;

    Way() : geom(nullptr) {}

    Way(const Way & other) : center(other.center), name(other.name), nodes(other.nodes), admin(other.admin){
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
    }

    Way(Way && other) : center(other.center), name(other.name), nodes(other.nodes), admin(other.admin){
        this->geom = other.geom;
        other.geom = nullptr;
    }

    Way & operator=(const Way & other){
        this->center = other.center;
        this->id = other.id;
        this->name = other.name;
        this->nodes = other.nodes;
        this->admin = other.admin;
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
        return *this;
    }
    ~Way() {
        if(geom != nullptr)
            GEOSGeom_destroy(geom);
    }

};

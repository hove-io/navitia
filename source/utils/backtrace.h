#pragma once

#include <execinfo.h>
#include <string>
#include <vector>

typedef void *ivoid_t;

class BackTrace{
public:
    BackTrace(int nbtrace);
    ~BackTrace();
    std::string get_BackTrace() const;
private:
    /// Nombre de lignes de traces à récuperer
    int nb_trace;
    /// Nombre de lignes de traces retourner
    int nb_trace_returned;
    ivoid_t *buffer;
    std::vector<std::string> trace_list;
    std::string function_name(const std::string& mystr);
};



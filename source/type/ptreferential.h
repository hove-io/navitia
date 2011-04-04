#pragma once

#include "data.h"


/// Exécute une requête sur les données Data
/// Retourne une matrice 2D de chaînes de caractères
std::vector< std::vector<std::string> > query(std::string request, const Data & data);

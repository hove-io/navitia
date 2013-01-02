#pragma once
#include <iostream>
#include <vector>

double str_to_double(std::string);
int str_to_int(std::string str);

/**
  Cette fonction permet de recupérer une chaine qui se trouve à une position donnée
  */
std::vector< std::string > split_string(const std::string&,const std::string & );

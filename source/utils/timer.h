#pragma once
#include <string>
#include <chrono>
#include <iostream>
/// Petit outil tout bête permettant de mesurer et afficher des temps d'exécution
struct Timer {
    std::chrono::time_point<std::chrono::system_clock> start;
    std::string name;
    Timer();
    /// Nom du timer qui sera affiché
    Timer(const std::string & name);
    /// À la destruction il affiche le temps
    ~Timer();
};

/// Affiche le temps écoulé
std::ostream & operator<<(std::ostream & os, const Timer & timer);

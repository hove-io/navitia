#pragma once
#include <string>
#include <chrono>
#include <iostream>
/// Petit outil tout bête permettant de mesurer et afficher des temps d'exécution
struct Timer {
    std::chrono::time_point<std::chrono::system_clock> start;
    std::string name;
    bool print_at_destruction;

    Timer();
    /// Nom du timer qui sera affiché
    Timer(const std::string & name, bool print_at_destruction = true);

    /// À la destruction il affiche le temps
    ~Timer();

    /// Retourne le nombre de secondes depuis la dernière initialisation
    int ms() const;

    /// Remet le compteur à 0
    void reset();
};

/// Affiche le temps écoulé
std::ostream & operator<<(std::ostream & os, const Timer & timer);

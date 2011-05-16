/// Définit l'état courant
struct State {
    /// Dernier ticket utilisé
    std::string ticket;

    /// Durée depuis le dernier ticket acheté (en secondes)
    int duration;

    /// Dernier mode utilisé
    std::string mode;

    /// Dernière zone utilisée
    std::string zone;

    /// Dernier endroit où à eu lieu l'achat
    std::string last_stop;
};

/// Type de comparaison possible entre un arc et une valeur
enum Comp_e { Less, Greater, Equal, Nil};

/// Définit un arc et les conditions pour l'emprunter
struct Edge {
    /// Valeur à que doit respecter la condition
    std::string condition_value;

    /// Ticket à acheter pour prendre cet arc
    /// Chaîne vide si rien à faire
    std::string ticket;
    
    /// Opérateur de comparaison
    /// Nil s'il n'y a pas de restriction
    Comp_e comparaison;

    /// Valeur à comparer
    std::string value;
};

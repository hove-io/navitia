Architecture fonctionnelle
==========================

Description des différents modules
**********************************

.. aafig::
  :textual:
  :foreground: #FF6600
  :scale: 75%

  /--------------+ /-----+ /--------------+ /---------+     /--------------+ /-----+ /--------------+
  |"Données"     | |     | |"Données"     | |"Autres" |     |"Perturbation"| |     | |"Temps réel"  |
  |"Transporteur"| |"..."| |"Transporteur"| |"Données"|     |"Transporteur"| |"..."| |"Transporteur"|
  |   "1"        | |     | |"n"           | |         |     |"1"           | |     | |"n"           |
  +--------------+ +-----+ +--------------+ +---------+     +--------------+ +-----+ +--------------+

.. aafig::
  :textual:
  :foreground: #80CB66
  :scale: 75%

                                   +-----+                              +------------+
                                   |FUSiO|                              |AlerteTrafic|
                                   +-----+                              +------------+
                                      |          +----------------+            |
                                      |         / "Espace de mise" \           |
                                      \------->/ "à disposition des"\<---------/
                                              /"données théorique et"\
                                             /       "temps réel"     \
                                            +--------------------------+

.. aafig::
  :textual:
  :foreground: #4F81BD
  :scale: 75%

                                                         |
                                                         v
                    +---------------+  +---------------+  +---------------+  +---------------+
                    |"Moteur "      |+ |"Moteur"       |+ |"Moteur"       |+ |"Moteurs"      |+
                    |"d'itinéraires"|| |"de requête"   || |"phonétique"   || |"de tarif"     ||
                    +---------------+| +---------------+| +---------------+| +---------------+|
                     +---------------+  +---------------+  +---------------+  +---------------+
                                                         |
                                                         v
                                                  +-------------+     /------------\
                                                  | hub NAViTiA |+ -->|Statistiques|
                                                  +-------------+|    \------------/
                                                   +-------------+


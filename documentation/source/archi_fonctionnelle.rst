Architecture fonctionnelle
==========================

Description des différents modules
**********************************

.. aafig::
  :textual:
  :foreground: #FF6600

  /--------------+ /-----+ /--------------+     /--------------+ /-----+ /--------------+
  |"Données"     | |     | |"Données"     |     |"Données"     | |     | |"Données"     |
  |"Transporteur"| |"..."| |"Transporteur"|     |"Transporteur"| |"..."| |"Transporteur"|
  |   "1"        | |     | |"n"           |     |"1"           | |     | |"n"           |
  +--------------+ +-----+ +--------------+     +--------------+ +-----+ +--------------+
    

.. aafig::
    :textual:
    :foreground: #80CB66

                   /-----\                              /------------\
                   |FUSiO|                              |AlerteTrafic|
                   \-----/                              \------------/
                      |          +----------------+            |
                      |         / "Espace de mise" \           |
                      \------->/ "à disposition des"\<---------/
                              /"données théorique et"\
                             /       "temps réel"     \
                            +--------------------------+
.. aafig::
  :textual:
  :foreground: #4F81BD

                                         |
                                         |
                                         |
                                         v
                                +-----------------+
                                |Moteurs de calcul|+
                                +-----------------+|
                                  +----------------/
                                         |
                                         |
                                         |
                                         |
                                         |
                                         v
                                    /---------\
                                    | NAViTiA |-+
                                    \---------/ |
                                      +---------/


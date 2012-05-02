Architecture fonctionnelle
==========================

Description des différents modules
**********************************

.. aafig::
    :textual:
    :fill: #FF6600

    /--------------+ /-----+ /--------------+     /--------------+ /-----+ /--------------+
    |"Données"     | |     | |"Données"     |     |"Données"     | |     | |"Données"     |
    |"Transporteur"| |"..."| |"Transporteur"|     |"Transporteur"| |"..."| |"Transporteur"|
    |   "1"        | |     | |"n"           |     |"1"           | |     | |"n"           |
    +--------------+ +-----+ +--------------+     +--------------+ +-----+ +--------------+
    

.. aafig::
    :textual:
    :foreground: #00FF66

               /-----\                                  /------------\
               |FUSiO|                                  |AlerteTrafic|
               \-----/                                  \------------/
                  |              +----------------+            |
                  |             / "Espace de mise" \           |
                  \----------->/ "à disposition des"\<---------/
                              /"données théorique et"\
                             /       "temps réel"     \
                            +--------------------------+
                                         |
                                        /|\
                                       | | |
                                       v v v
.. aafig::
  :textual:
  :background: #4F81BD

                                /-----------------\
                                |Moteurs de calcul| \
                                \-----------------/ |
                                  |                 |
                                  \-----------------/
                                        /|\
                                       / | \
                                      |  |  |
                                       \ | /
                                        \|/
                                         v
                                    /---------\
                                    | NAViTiA | \
                                    \---------/ |
                                      \---------/


Architecture fonctionnelle
==========================

Description des différents modules
**********************************

.. aafig::
  :fill:#FF6600

/------------+ /-----+ /------------+     /------------+ /-----+ /------------+
|Données     | |     | |Données     |     |Données     | |     | |Données     |
|Transporteur| | ... | |Transporteur|     |Transporteur| | ... | |Transporteur|
|1           | |     | |n           |     |1           | |     | |n           |
+------------+ +-----+ +------------+     +------------+ +-----+ +------------+
    

.. aafig::

               /-----\                                /------------\
               |FUSiO|                                |AlerteTrafic|
               \-----/                                \------------/
                  |              +--------------+            |
                  |             /Espace de mise  \           |
                  \----------->/à disposition des \<---------/
                              /données théorique et\
                             /     temps réel       \
                            +------------------------+
                                         |
                                        /|\
                                       / | \
                                      v  v  v
.. aafig::
  :fill:#4F81BD
                                /-----------------\
                                |Moteurs de calcul|\
                                \-----------------/|
                                 \-----------------/
                                        /|\
                                       / | \
                                       | | |
                                       \ | /
                                        \|/
                                         v
                                    /---------\
                                    | NAViTiA |\
                                    \---------/|
                                     \---------/


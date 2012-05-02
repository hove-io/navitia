Architecture fonctionnelle
==========================

Description des différents modules
**********************************

.. aafig::
  :textual:
  :fill: #FF6600

/------------+ /-----+ /------------+     /------------+ /-----+ /------------+
|Données     | |     | |Données     |     |Données     | |     | |Données     |
|Transporteur| | ... | |Transporteur|     |Transporteur| | ... | |Transporteur|
|1           | |     | |n           |     |1           | |     | |n           |
+------------+ +-----+ +------------+     +------------+ +-----+ +------------+
    

.. aafig::
  :textual:

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
  :textual:
  :fill: #4F81BD

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


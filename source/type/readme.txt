/** \page type Type

Structure de données pour représenter le référentiel.

La structure principale est Data

Les données sont sérialisées avec boost::serialize et compressées en lz4.
La compression permet des fichiers plus petits avec pratiquement aucune perte de temps à la lecture (voire un gain sur des disques lents).

*/

#ifndef CIRCLE
#define CIRCLE

#endif // CIRCLE

#include "type/geographical_coord.h"
#include <set>

namespace navitia { namespace routing {

constexpr static double N_DEG_TO_RAD = 0.01745329238;
constexpr static double N_RAD_TO_DEG = 57.295779513;
constexpr static double PI = 3.14159265359;

/**
  On construit un point dans une direction donnée comprise entre 0° et 360°
  radius : la distance entre ce point et le centre est donnée en mètres

  On utilise l'équation d'un petit cercle sur la sphère
  http://step.ipgp.fr/images/8/81/Cours1_L3_Greff.pdf

  Ainsi que les triangles rectangles sphériques
  https://fr.wikipedia.org/wiki/Trigonométrie_sphérique

 * @brief project_in_direction
 * @param center
 * @param direction
 * @param radius
 * @return
 */
type::GeographicalCoord project_in_direction(const type::GeographicalCoord& center,
                                             const double& direction,
                                             const double& radius);

/**
  Create a circle
 * @brief circle
 * @param center
 * @param radius
 * @return
 */
type::Polygon circle(const type::GeographicalCoord& center,
                     const double& radius);
}}

#pragma once

#include "type/geographical_coord.h"
#include <set>

namespace navitia { namespace routing {

constexpr static double N_RAD_TO_DEG = 57.295779513;
constexpr static double PI = 3.14159265359;

/**
  Build a point in a given direction wic is between 0° and 360°
  radius : the distance between center and the point in meter

  We use the equation of small circle on sphere
  http://step.ipgp.fr/images/8/81/Cours1_L3_Greff.pdf

  We use spherical trigonometry too
  https://en.wikipedia.org/wiki/Spherical_trigonometry

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

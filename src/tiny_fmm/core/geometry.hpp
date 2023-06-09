/**
 * Fast map matching.
 *
 * Definition of geometry
 * 2020-11-25 Remove linestring2ogr
 *
 * @author: Can Yang
 * @version: 2017.11.11
 */

#ifndef FMM_GEOMTYPES_HPP
#define FMM_GEOMTYPES_HPP

#include <Eigen/Core>

namespace FMM
{
/**
 * Core data types
 */
namespace CORE
{

/**
 *  Point class
 */
using Point = Eigen::Vector3d;
/**
 *  Linestring geometry class
 *
 *  This class wraps a boost linestring geometry.
 */
using LineString = std::vector<Point>;
}; // namespace CORE

}; // namespace FMM

#endif // FMM_GEOMTYPES_HPP

/**
 * Fast map matching.
 *
 * Definition of Data types used in the FMM algorithm
 *
 * @author: Can Yang
 * @version: 2017.11.11
 */

#ifndef FMM_TYPES_HPP
#define FMM_TYPES_HPP

#include <vector>
#include <list>
#include <string>
#include <unordered_map>
#include "core/geometry.hpp"

namespace FMM
{
using RowVectors = Eigen::Matrix<double, Eigen::Dynamic, 3, Eigen::RowMajor>;

namespace NETWORK
{
typedef int64_t NodeID;    /**< Node ID in the network, can be discontinuous
                             int */
typedef int64_t EdgeID;    /**< Edge ID in the network, can be negative to
                             distinguish edges in two directions */
typedef int32_t NodeIndex; /**< Node Index in the network, range
                                 from [0,num_vertices-1 ]*/
typedef int32_t EdgeIndex; /**< Edge Index in the network, range
                                 from [0,num_edges-1 ]*/

/**
 * Vector of node id
 */
typedef std::vector<NodeID> NodeIDVec;
/**
 * Map of node index
 */
typedef std::unordered_map<NodeID, NodeIndex> NodeIndexMap;
/**
 * Map of edge index
 */
typedef std::unordered_map<EdgeID, EdgeIndex> EdgeIndexMap;

/**
 * Road edge class
 */
struct Edge
{
    EdgeID id;                  /**< Edge ID, can be discontinuous integers */
    NodeID source;              /**< source node id */
    NodeID target;              /**< target node id */
    Eigen::Vector3i index;      // edge index, source/target index
    double length;              /**< length of the edge polyline */
    FMM::CORE::LineString geom; /**< the edge geometry */

    Eigen::Map<RowVectors> as_row_vectors()
    {
        return Eigen::Map<RowVectors>(&geom[0][0], geom.size(), 3);
    }
    Eigen::Map<const RowVectors> as_row_vectors() const
    {
        return Eigen::Map<const RowVectors>(&geom[0][0], geom.size(), 3);
    }
};

} // namespace NETWORK
} // namespace FMM
#endif /* MM_TYPES_HPP */

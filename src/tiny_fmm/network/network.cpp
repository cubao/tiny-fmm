#include "network/network.hpp"
#include "util/debug.hpp"
#include "util/util.hpp"
#include "util/cubao_helpers.hpp"
#include "cubao/polyline_ruler.hpp"

#include <math.h>    // Calulating probability
#include <algorithm> // Partial sort copy
#include <stdexcept>

using namespace FMM;
using namespace FMM::CORE;
using namespace FMM::MM;
using namespace FMM::NETWORK;

bool Network::candidate_compare(const Candidate &a, const Candidate &b)
{
    if (a.dist != b.dist) {
        return a.dist < b.dist;
    } else {
        return a.edge->index[0] < b.edge->index[0];
    }
}

int Network::add_edge(EdgeID edge_id, NodeID source, NodeID target,
                      const FMM::CORE::LineString &geom)
{
    return add_edge(edge_id, source, target,
                    Eigen::Map<const RowVectors>(&geom[0][0], geom.size(), 3));
}

int Network::add_edge(EdgeID edge_id, NodeID source, NodeID target,
                      const Eigen::Ref<const RowVectors> &polyline)
{
    if (edge_map.find(edge_id) != edge_map.end()) {
        SPDLOG_ERROR("duplicate edge: {}", edge_id);
        return -1;
    }

    const int N = polyline.rows();
    NodeIndex s_idx, t_idx;
    if (node_map.find(source) == node_map.end()) {
        s_idx = node_id_vec.size();
        node_id_vec.push_back(source);
        node_map.emplace(source, s_idx);
        vertex_points.push_back(polyline.row(0));
    } else {
        s_idx = node_map[source];
    }
    if (node_map.find(target) == node_map.end()) {
        t_idx = node_id_vec.size();
        node_id_vec.push_back(target);
        node_map.emplace(target, t_idx);
        vertex_points.push_back(polyline.row(N - 1));
    } else {
        t_idx = node_map[target];
    }
    EdgeIndex index = edges.size();
    FMM::CORE::LineString geom;
    geom.resize(N);
    Eigen::Map<RowVectors>(&geom[0][0], N, 3) = polyline;
    cubao::PolylineRuler ruler(polyline, is_wgs84);
    edges.push_back({edge_id, source, target,              //
                     Eigen::Vector3i(index, s_idx, t_idx), //
                     ruler.length(), std::move(geom)});
    edge_map.emplace(edge_id, index);
    // tree.add_polyline(ruler, index);
    return index;
}

bool Network::load(const std::string &path)
{
    assert(edges.empty());
    return from_json(cubao::load_json(path));
}
bool Network::dump(const std::string &path) const
{
    return cubao::dump_json(path, to_json(), true);
}

bool Network::loads(const std::string &json)
{
    return from_json(cubao::loads(json));
}
std::string Network::dumps() const { return cubao::dumps(to_json()); }

bool Network::from_json(const RapidjsonValue &json)
{
    if (!json.IsObject()) {
        return false;
    }
    using namespace cubao;
    // json["srid"].GetInt();
    for (auto &e : json["edges"].GetArray()) {
        auto id = e["id"].GetInt64();
        auto source = e["source"].GetInt64();
        auto target = e["target"].GetInt64();
        FMM::CORE::LineString geom;
        for (auto &xy : e["coordinates"].GetArray()) {
            geom.emplace_back(xy[0].GetDouble(), xy[1].GetDouble(),
                              xy.Size() > 2 ? xy[2].GetDouble() : 0.0);
        }
        add_edge((EdgeID)id, (NodeID)source, (NodeID)target, geom);
    }
    num_vertices = node_id_vec.size();
    SPDLOG_INFO("Number of edges {} nodes {}", edges.size(), num_vertices);
    build_rtree_index();
    SPDLOG_INFO("Read network done.");
    return true;
}
RapidjsonValue Network::to_json(RapidjsonAllocator &allocator) const
{
    using namespace cubao;
    RapidjsonValue edges(rapidjson::kArrayType);
    for (auto &e : this->edges) {
        RapidjsonValue edge(rapidjson::kObjectType);
        edge.AddMember("id", RapidjsonValue(e.id), allocator);
        edge.AddMember("source", RapidjsonValue(e.source), allocator);
        edge.AddMember("target", RapidjsonValue(e.target), allocator);
        RapidjsonValue coordinates(rapidjson::kArrayType);
        auto &G = e.geom;
        const int N = G.size();
        coordinates.Reserve(N, allocator);
        for (int i = 0; i < N; ++i) {
            RapidjsonValue xyz(rapidjson::kArrayType);
            xyz.Reserve(3, allocator);
            xyz.PushBack(RapidjsonValue(G[i][0]), allocator);
            xyz.PushBack(RapidjsonValue(G[i][1]), allocator);
            xyz.PushBack(RapidjsonValue(G[i][2]), allocator);
            coordinates.PushBack(xyz, allocator);
        }
        edge.AddMember("coordinates", coordinates, allocator);
        edges.PushBack(edge, allocator);
    }
    RapidjsonValue json(rapidjson::kObjectType);
    json.AddMember("is_wgs84", RapidjsonValue(is_wgs84), allocator);
    json.AddMember("edges", edges, allocator);
    return json;
}

std::string Network::export_geojson() const
{
    return cubao::dumps(__export_geojson());
}

bool Network::export_geojson(const std::string &path) const
{
    return cubao::dump_json(path, __export_geojson(), true);
}

RapidjsonValue Network::__export_geojson() const
{
    RapidjsonAllocator allocator;
    RapidjsonValue features(rapidjson::kArrayType);
    features.Reserve(edges.size() + vertex_points.size(), allocator);
    for (int i = 0, N = vertex_points.size(); i < N; ++i) {
        RapidjsonValue geometry(rapidjson::kObjectType);
        geometry.AddMember("type", "Point", allocator);
        RapidjsonValue coords(rapidjson::kArrayType);
        coords.PushBack(RapidjsonValue(vertex_points[i][0]), allocator);
        coords.PushBack(RapidjsonValue(vertex_points[i][1]), allocator);
        coords.PushBack(RapidjsonValue(vertex_points[i][2]), allocator);
        geometry.AddMember("coordinates", coords, allocator);
        RapidjsonValue properties(rapidjson::kObjectType);
        properties.AddMember("type", "node", allocator);
        properties.AddMember("_id", RapidjsonValue((int64_t)node_id_vec[i]),
                             allocator);
        RapidjsonValue feature(rapidjson::kObjectType);
        feature.AddMember("type", "Feature", allocator);
        feature.AddMember("geometry", geometry, allocator);
        feature.AddMember("properties", properties, allocator);
        features.PushBack(feature, allocator);
    }
    for (auto &e : edges) {
        RapidjsonValue geometry(rapidjson::kObjectType);
        geometry.AddMember("type", "LineString", allocator);
        auto &G = e.geom;
        int N = G.size();
        RapidjsonValue coords(rapidjson::kArrayType);
        for (int i = 0; i < N; ++i) {
            RapidjsonValue xy(rapidjson::kArrayType);
            xy.Reserve(3, allocator);
            xy.PushBack(RapidjsonValue(G[i][0]), allocator);
            xy.PushBack(RapidjsonValue(G[i][1]), allocator);
            xy.PushBack(RapidjsonValue(G[i][2]), allocator);
            coords.PushBack(xy, allocator);
        }
        geometry.AddMember("coordinates", coords, allocator);
        RapidjsonValue properties(rapidjson::kObjectType);
        properties.AddMember("type", "edge", allocator);
        properties.AddMember("_id", RapidjsonValue((int64_t)e.id), allocator);
        properties.AddMember("source", RapidjsonValue((int64_t)e.source),
                             allocator);
        properties.AddMember("target", RapidjsonValue((int64_t)e.target),
                             allocator);
        properties.AddMember("length", RapidjsonValue(e.length), allocator);
        RapidjsonValue feature(rapidjson::kObjectType);
        feature.AddMember("type", "Feature", allocator);
        feature.AddMember("geometry", geometry, allocator);
        feature.AddMember("properties", properties, allocator);
        features.PushBack(feature, allocator);
    }
    RapidjsonValue fc(rapidjson::kObjectType);
    fc.AddMember("type", "FeatureCollection", allocator);
    fc.AddMember("features", features, allocator);
    return fc;
}

int Network::get_node_count() const { return node_id_vec.size(); }

int Network::get_edge_count() const { return edges.size(); }

// Get the edge vector
const std::vector<Edge> &Network::get_edges() const { return edges; }

const Edge &Network::get_edge(EdgeID id) const
{
    return edges[get_edge_index(id)];
};

const Edge &Network::get_edge(EdgeIndex index) const { return edges[index]; };

// Get the ID attribute of an edge according to its index
EdgeID Network::get_edge_id(EdgeIndex index) const
{
    return index < edges.size() ? edges[index].id : -1;
}

EdgeIndex Network::get_edge_index(EdgeID id) const { return edge_map.at(id); }

NodeID Network::get_node_id(NodeIndex index) const
{
    return index < num_vertices ? node_id_vec[index] : -1;
}

NodeIndex Network::get_node_index(NodeID id) const { return node_map.at(id); }

Point Network::get_node_geom_from_idx(NodeIndex index) const
{
    return vertex_points[index];
}

// Construct a Rtree using the vector of edges
void Network::build_rtree_index() {}

Traj_Candidates Network::search_tr_cs_knn(Trajectory &trajectory, std::size_t k,
                                          double radius) const
{
    return search_tr_cs_knn(trajectory.geom, k, radius);
}

Traj_Candidates Network::search_tr_cs_knn(const LineString &geom, std::size_t k,
                                          double radius) const
{
    int NumberPoints = 0; // geom.get_num_points();
    Traj_Candidates tr_cs(NumberPoints);
    unsigned int current_candidate_index = num_vertices;
    for (int i = 0; i < NumberPoints; ++i) {
        // SPDLOG_DEBUG("Search candidates for point index {}",i);
        // Construct a bounding boost_box
        /*
        double px = geom.get_x(i);
        double py = geom.get_y(i);
        Point_Candidates pcs;
        boost_box b(Point(geom.get_x(i) - radius, geom.get_y(i) - radius),
                    Point(geom.get_x(i) + radius, geom.get_y(i) + radius));
        std::vector<Item> temp;
        // Rtree can only detect intersect with a the bounding box of
        // the geometry stored.
        rtree.query(boost::geometry::index::intersects(b),
                    std::back_inserter(temp));
        int Nitems = 0; // temp.size();
        for (unsigned int j = 0; j < Nitems; ++j) {
            // Check for detailed intersection
            // The two edges are all in OGR_linestring
            Edge *edge = temp[j].second;
            double offset;
            double dist;
            double closest_x, closest_y;
            ALGORITHM::linear_referencing(px, py, edge->geom, &dist, &offset,
                                          &closest_x, &closest_y);
            if (dist <= radius) {
                // index, offset, dist, edge, pseudo id, point
                Candidate c = {0, offset, dist, edge,
                               Point(closest_x, closest_y)};
                pcs.push_back(c);
            }
        }
        SPDLOG_DEBUG("Candidate count point {}: {} (filter to k)", i,
                     pcs.size());
        if (pcs.empty()) {
            SPDLOG_DEBUG("Candidate not found for point {}: {} {}", i, px, py);
            return Traj_Candidates();
        }
        // KNN part
        if (pcs.size() <= k) {
            tr_cs[i] = pcs;
        } else {
            tr_cs[i] = Point_Candidates(k);
            std::partial_sort_copy(pcs.begin(), pcs.end(), tr_cs[i].begin(),
                                   tr_cs[i].end(), candidate_compare);
        }
        for (int m = 0; m < tr_cs[i].size(); ++m) {
            tr_cs[i][m].index = current_candidate_index + m;
        }
        current_candidate_index += tr_cs[i].size();
        */
        // SPDLOG_TRACE("current_candidate_index {}",current_candidate_index);
    }
    return tr_cs;
}

const LineString &Network::get_edge_geom(EdgeID edge_id) const
{
    return edges[get_edge_index(edge_id)].geom;
}

LineString Network::complete_path_to_geometry(const LineString &traj,
                                              const C_Path &complete_path) const
{
    LineString line;
    /*
    if (complete_path.empty())
        return line;
    int Npts = traj.get_num_points();
    int NCsegs = complete_path.size();
    if (NCsegs == 1) {
        double dist;
        double firstoffset;
        double lastoffset;
        const LineString &firstseg = get_edge_geom(complete_path[0]);
        ALGORITHM::linear_referencing(traj.get_x(0), traj.get_y(0), firstseg,
                                      &dist, &firstoffset);
        ALGORITHM::linear_referencing(traj.get_x(Npts - 1),
                                      traj.get_y(Npts - 1), firstseg, &dist,
                                      &lastoffset);
        LineString firstlineseg =
            ALGORITHM::cutoffseg_unique(firstseg, firstoffset, lastoffset);
        append_segs_to_line(&line, firstlineseg, 0);
    } else {
        const LineString &firstseg = get_edge_geom(complete_path[0]);
        const LineString &lastseg = get_edge_geom(complete_path[NCsegs - 1]);
        double dist;
        double firstoffset;
        double lastoffset;
        ALGORITHM::linear_referencing(traj.get_x(0), traj.get_y(0), firstseg,
                                      &dist, &firstoffset);
        ALGORITHM::linear_referencing(traj.get_x(Npts - 1),
                                      traj.get_y(Npts - 1), lastseg, &dist,
                                      &lastoffset);
        LineString firstlineseg =
            ALGORITHM::cutoffseg(firstseg, firstoffset, 0);
        LineString lastlineseg = ALGORITHM::cutoffseg(lastseg, lastoffset, 1);
        append_segs_to_line(&line, firstlineseg, 0);
        if (NCsegs > 2) {
            for (int i = 1; i < NCsegs - 1; ++i) {
                const LineString &middleseg = get_edge_geom(complete_path[i]);
                append_segs_to_line(&line, middleseg, 1);
            }
        }
        append_segs_to_line(&line, lastlineseg, 1);
    }
    */
    return line;
}

const std::vector<Point> &Network::get_vertex_points() const
{
    return vertex_points;
}

const Point &Network::get_vertex_point(NodeIndex u) const
{
    return vertex_points[u];
}

LineString Network::route2geometry(const std::vector<EdgeID> &path) const
{
    LineString line;
    if (path.empty())
        return line;
    // if (complete_path->empty()) return nullptr;
    int NCsegs = path.size();
    for (int i = 0; i < NCsegs; ++i) {
        EdgeIndex e = get_edge_index(path[i]);
        const LineString &seg = edges[e].geom;
        if (i == 0) {
            append_segs_to_line(&line, seg, 0);
        } else {
            append_segs_to_line(&line, seg, 1);
        }
    }
    return line;
}

LineString Network::route2geometry(const std::vector<EdgeIndex> &path) const
{
    LineString line;
    if (path.empty())
        return line;
    // if (complete_path->empty()) return nullptr;
    int NCsegs = path.size();
    for (int i = 0; i < NCsegs; ++i) {
        const LineString &seg = edges[path[i]].geom;
        if (i == 0) {
            append_segs_to_line(&line, seg, 0);
        } else {
            append_segs_to_line(&line, seg, 1);
        }
    }
    return line;
}

void Network::append_segs_to_line(LineString *line, const LineString &segs,
                                  int offset)
{
    line->insert(line->end(), segs.begin() + offset, segs.end());
}

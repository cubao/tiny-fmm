//
// Created by Can Yang on 2020/4/1.
//

#include "mm/fmm/ubodt_gen_algorithm.hpp"
#include "mm/fmm/ubodt.hpp"
#include "util/debug.hpp"
#include "util/cubao_helpers.hpp"
#include <omp.h>

using namespace FMM;
using namespace FMM::CORE;
using namespace FMM::NETWORK;
using namespace FMM::MM;

inline bool endswith(const std::string &str, const std::string &text)
{
    if (str.length() < text.length()) {
        return false;
    }
    return 0 == str.compare(str.length() - text.length(), text.length(), text);
}

std::string UBODTGenAlgorithm::generate_ubodt(const std::string &filename,
                                              double delta, bool binary,
                                              bool use_omp) const
{
    std::ostringstream oss;
    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();
    if (endswith(filename, ".json")) {
        dump(filename, delta);
    } else {
        if (use_omp) {
            precompute_ubodt_omp(filename, delta, binary);
        } else {
            precompute_ubodt_single_thead(filename, delta, binary);
        }
    }
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    double time_spent =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count() /
        1000.;
    oss << "Status: success\n";
    oss << "Time takes " << time_spent << " seconds\n";
    return oss.str();
};

void UBODTGenAlgorithm::precompute_ubodt_single_thead(
    const std::string &filename, double delta, bool binary) const
{
    int num_vertices = ng_.get_num_vertices();
    int step_size = num_vertices / 10;
    if (step_size < 10)
        step_size = 10;
    std::ofstream myfile(filename);
    SPDLOG_INFO("Start to generate UBODT with delta {}", delta);
    SPDLOG_INFO("Output format {}", (binary ? "binary" : "csv"));
    if (binary) {
        boost::archive::binary_oarchive oa(myfile);
        for (NodeIndex source = 0; source < num_vertices; ++source) {
            if (source % step_size == 0)
                SPDLOG_INFO("Progress {} / {}", source, num_vertices);
            PredecessorMap pmap;
            DistanceMap dmap;
            ng_.single_source_upperbound_dijkstra(source, delta, &pmap, &dmap);
            write_result_binary(oa, source, pmap, dmap);
        }
    } else {
        myfile << "source;target;next_n;prev_n;next_e;distance\n";
        // TODO, very bad idea to export
        //  Node/Edge_index instead of
        //  Node/Edge_id
        for (NodeIndex source = 0; source < num_vertices; ++source) {
            if (source % step_size == 0)
                SPDLOG_INFO("Progress {} / {}", source, num_vertices);
            PredecessorMap pmap;
            DistanceMap dmap;
            ng_.single_source_upperbound_dijkstra(source, delta, &pmap, &dmap);
            write_result_csv(myfile, source, pmap, dmap);
        }
    }
    myfile.close();
}

// Parallelly generate ubodt using OpenMP
void UBODTGenAlgorithm::precompute_ubodt_omp(const std::string &filename,
                                             double delta, bool binary) const
{
    int num_vertices = ng_.get_num_vertices();
    int step_size = num_vertices / 10;
    if (step_size < 10)
        step_size = 10;
    std::ofstream myfile(filename);
    SPDLOG_INFO("Start to generate UBODT with delta {}", delta);
    SPDLOG_INFO("Output format {}", (binary ? "binary" : "csv"));
    if (binary) {
        boost::archive::binary_oarchive oa(myfile);
        int progress = 0;
#pragma omp parallel
        {
#pragma omp for
            for (int source = 0; source < num_vertices; ++source) {
                ++progress;
                if (progress % step_size == 0) {
                    SPDLOG_INFO("Progress {} / {}", progress, num_vertices);
                }
                PredecessorMap pmap;
                DistanceMap dmap;
                std::stringstream node_output_buf;
                ng_.single_source_upperbound_dijkstra(source, delta, &pmap,
                                                      &dmap);
                write_result_binary(oa, source, pmap, dmap);
            }
        }
    } else {
        myfile << "source;target;next_n;prev_n;next_e;distance\n";
        int progress = 0;
#pragma omp parallel
        {
#pragma omp for
            for (int source = 0; source < num_vertices; ++source) {
                ++progress;
                if (progress % step_size == 0) {
                    SPDLOG_INFO("Progress {} / {}", progress, num_vertices);
                }
                PredecessorMap pmap;
                DistanceMap dmap;
                std::stringstream node_output_buf;
                ng_.single_source_upperbound_dijkstra(source, delta, &pmap,
                                                      &dmap);
                write_result_csv(myfile, source, pmap, dmap);
            }
        }
    }
    myfile.close();
}

bool UBODTGenAlgorithm::dump(const std::string &filename, double delta) const
{
    return cubao::dump_json(filename, to_json(delta), true);
}

std::string UBODTGenAlgorithm::dumps(const std::string &filename,
                                     double delta) const
{
    return cubao::dumps(to_json(delta));
}
RapidjsonValue UBODTGenAlgorithm::to_json(RapidjsonAllocator &allocator,
                                          double delta) const
{
    RapidjsonValue json(rapidjson::kArrayType);
    int num_vertices = ng_.get_num_vertices();
    int step_size = num_vertices / 10;
    if (step_size < 10)
        step_size = 10;
    SPDLOG_INFO("Start to generate UBODT with delta {}", delta);
    SPDLOG_INFO("Output format json");
    for (NodeIndex source = 0; source < num_vertices; ++source) {
        if (source % step_size == 0)
            SPDLOG_INFO("Progress {} / {}", source, num_vertices);
        PredecessorMap pmap;
        DistanceMap dmap;
        ng_.single_source_upperbound_dijkstra(source, delta, &pmap, &dmap);
        write_result_json(json, allocator, source, pmap, dmap);
    }
    return json;
}

/**
 * Write the result of routing from a single source node
 * @param stream output stream
 * @param s      source node
 * @param pmap   predecessor map
 * @param dmap   distance map
 */
void UBODTGenAlgorithm::write_result_csv(std::ostream &stream, NodeIndex s,
                                         PredecessorMap &pmap,
                                         DistanceMap &dmap) const
{
    std::vector<Record> source_map;
    for (auto iter = pmap.begin(); iter != pmap.end(); ++iter) {
        NodeIndex cur_node = iter->first;
        if (cur_node != s) {
            NodeIndex prev_node = iter->second;
            NodeIndex v = cur_node;
            NodeIndex u;
            while ((u = pmap[v]) != s) {
                v = u;
            }
            NodeIndex successor = v;
            // Write the result to source map
            double cost = dmap[successor];
            EdgeIndex edge_index = ng_.get_edge_index(s, successor, cost);
            source_map.push_back({s, cur_node, successor, prev_node, edge_index,
                                  dmap[cur_node], nullptr});
        }
    }
#pragma omp critical
    for (Record &r : source_map) {
        stream << r.source << ";" << r.target << ";" << r.first_n << ";"
               << r.prev_n << ";" << r.next_e << ";" << r.cost << "\n";
    }
}

void UBODTGenAlgorithm::write_result_json(RapidjsonValue &json,
                                          RapidjsonAllocator &allocator,
                                          NodeIndex s, PredecessorMap &pmap,
                                          DistanceMap &dmap) const
{
    std::vector<Record> source_map;
    for (auto iter = pmap.begin(); iter != pmap.end(); ++iter) {
        NodeIndex cur_node = iter->first;
        if (cur_node != s) {
            NodeIndex prev_node = iter->second;
            NodeIndex v = cur_node;
            NodeIndex u;
            while ((u = pmap[v]) != s) {
                v = u;
            }
            NodeIndex successor = v;
            double cost = dmap[successor];
            EdgeIndex edge_index = ng_.get_edge_index(s, successor, cost);
            source_map.push_back({s, cur_node, successor, prev_node, edge_index,
                                  dmap[cur_node], nullptr});
        }
    }
    auto arr = json.GetArray();
#pragma omp critical
    for (Record &r : source_map) {
        RapidjsonValue record(rapidjson::kObjectType);
        record.AddMember("source", RapidjsonValue(r.source), allocator);
        record.AddMember("target", RapidjsonValue(r.target), allocator);
        record.AddMember("first_n", RapidjsonValue(r.first_n), allocator);
        record.AddMember("prev_n", RapidjsonValue(r.prev_n), allocator);
        record.AddMember("next_e", RapidjsonValue(r.next_e), allocator);
        record.AddMember("cost", RapidjsonValue(r.cost), allocator);
        arr.PushBack(record, allocator);
    }
}

/**
 * Write the result of routing from a single source node in
 * binary format
 *
 * @param stream output stream
 * @param s      source node
 * @param pmap   predecessor map
 * @param dmap   distance map
 */
void UBODTGenAlgorithm::write_result_binary(
    boost::archive::binary_oarchive &stream, NodeIndex s, PredecessorMap &pmap,
    DistanceMap &dmap) const
{
    std::vector<Record> source_map;
    for (auto iter = pmap.begin(); iter != pmap.end(); ++iter) {
        NodeIndex cur_node = iter->first;
        if (cur_node != s) {
            NodeIndex prev_node = iter->second;
            NodeIndex v = cur_node;
            NodeIndex u;
            // When u=s, v is the next node visited
            while ((u = pmap[v]) != s) {
                v = u;
            }
            NodeIndex successor = v;
            // Write the result
            double cost = dmap[successor];
            EdgeIndex edge_index = ng_.get_edge_index(s, successor, cost);
            source_map.push_back({s, cur_node, successor, prev_node, edge_index,
                                  dmap[cur_node], nullptr});
        }
    }
#pragma omp critical
    for (Record &r : source_map) {
        stream << r.source << r.target << r.first_n << r.prev_n << r.next_e
               << r.cost;
    }
}

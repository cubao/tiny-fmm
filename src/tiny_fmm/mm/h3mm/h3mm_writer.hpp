/*
#ifndef H3MM_WRITER_HEADER
#define H3MM_WRITER_HEADER
#include "h3_type.hpp"
#include "h3_util.hpp"
#include "util/util.hpp"

namespace FMM
{
namespace MM
{

struct H3MatchResultConfig
{
    H3MatchResultConfig()
    {
        file = "";
        write_geom = false;
    };
    H3MatchResultConfig(const std::string &filename, bool write_geom_arg)
        : file(filename), write_geom(write_geom_arg){};
    std::string file; /**< Output file to write the result */
bool write_geom;
/**
 * Check the validation of the configuration
 * @return true if valid otherwise false
 */
bool validate() const
{
    if (file.empty()) {
        SPDLOG_CRITICAL("Output file not specified");
        return false;
    }
    if (FMM::UTIL::file_exists(file)) {
        SPDLOG_WARN("Overwrite existing result file {}", file);
    };
    std::string output_folder = FMM::UTIL::get_file_directory(file);
    if (!FMM::UTIL::folder_exist(output_folder)) {
        SPDLOG_CRITICAL("Output folder {} not exists", output_folder);
        return false;
    }
    return true;
};
/**
 * Print the configuration information
 */
void print() const
{
    SPDLOG_INFO("H3MatchResultConfig");
    SPDLOG_INFO("File: {}", file);
    SPDLOG_INFO("Write geom: {}", write_geom);
};
std::string to_string() const
{
    std::ostringstream oss;
    oss << "H3MatchResultConfig\n";
    oss << "result file : " << file << "\n";
    oss << "write geom : " << (write_geom ? "true" : "false") << "\n";
    return oss.str();
};
/**
 * Load result configuration from argument data
 * @param arg_data argument data generated by reading arguments
 * @return
 */
static H3MatchResultConfig load_from_arg(const cxxopts::ParseResult &arg_data)
{
    H3MatchResultConfig config;
    config.file = arg_data["output"].as<std::string>();
    if (arg_data.count("write_geom") > 0) {
        config.write_geom = true;
    }
    return config;
};
/**
 * Register arguments to an option object
 */
static void register_arg(cxxopts::Options &options)
{
    options.add_options()("o,output", "Output file name",
                          cxxopts::value<std::string>()->default_value(""))(
        "write_geom", "Write geometry");
};
/**
 * Register help information to a string stream
 */
static void register_help(std::ostringstream &oss)
{
    oss << "-o,--output (required) <string>: Output file name\n";
    oss << "--write_geom: if specified, write geometry output\n";
};
}
;

/**
 * A writer class for writing matche result to a CSV file.
 */
class H3MatchResultWriter
{
  public:
    /**
     * Constructor
     *
     * The output fields are defined only once and later all the match result
     * will be exported according to that configuration.
     *
     * @param result_file the filename to write result
     * @param config_arg the fields that will be exported
     *
     */

    H3MatchResultWriter(const H3MatchResultConfig &config_arg)
        : config_(config_arg), m_fstream(config_.file)
    {
        write_header();
    };

    /**
     * Write match result
     * @param traj Input trajectory
     * @param result Map match result
     */
    void write_result(const FMM::CORE::Trajectory &traj,
                      const FMM::MM::H3MatchResult &result)
    {
        std::ostringstream buf;
        buf << traj.id;
        buf << ";" << result.hexs;
        if (config_.write_geom)
            buf << ";" << hexs2wkt(result.hexs, 12);
        buf << "\n";
#pragma omp critical
        m_fstream << buf.str();
    };

  private:
    void write_header()
    {
        m_fstream << "id;hex";
        if (config_.write_geom) {
            m_fstream << ";geom";
        }
        m_fstream << "\n";
    };
    H3MatchResultConfig config_;
    std::ofstream m_fstream;
}; // CSVMatchResultWriter
} // namespace MM
} // namespace FMM
#endif
* /

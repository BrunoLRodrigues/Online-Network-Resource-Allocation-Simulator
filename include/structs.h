#pragma once

#include "utils.h"
#include "FileManager.h"

#include <memory>
#include <utility>

#pragma region Definitions_and_Enums

/**
 * @brief Type aliases for standard containers used in the simulation.
 */
#define Devices std::vector<Device>   ///< @brief Vector of Device objects.
#define Servers std::vector<Server>   ///< @brief Vector of Server objects.
#define iiP std::pair<int, int>       ///< @brief Pair of integers, typically for index pairs.
#define iVec std::vector<int>         ///< @brief Vector of integers, typically for indices.
#define sVec std::vector<std::string> ///< @brief Vector of strings, typically for data rows.
#define iiPVec std::vector<iiP>       ///< @brief Vector of integer pairs, typically for index pairs.
#define iMatrix std::vector<iVec>     ///< @brief Matrix of integers (vector of vectors), typically for indices.
#define iSet std::set<int>            ///< @brief Set of integers, typically for unique indices.

/**
 * @brief Enumerates the types of simulations available.
 */
enum class SimulationType {
    NONE,
    EXACT,          
    HEURISTIC,      
    METAHEURISTIC   
};

/**
 * @brief Converts a SimulationType enum value to its string representation.
 * @param type The SimulationType to convert.
 * @return String representation of the SimulationType.
 */
inline std::string to_string(SimulationType type) {
    switch (type) {
        case SimulationType::EXACT:         return "Exact";
        case SimulationType::HEURISTIC:     return "Heuristic";
        case SimulationType::METAHEURISTIC: return "Metaheuristic";
        default:                            return "UnknownType";
    }
}

/**
 * @brief Enumerates the available algorithms for resource allocation.
 */
enum class AlgorithmName {
    NONE,
    MIN_COST,   
    RANDOM,         
    GREEDY_AscAsc,
    GREEDY_AscDesc,
    GREEDY_DescAsc,
    GREEDY_DescDesc,         
    SA              
};

/**
 * @brief Converts an AlgorithmName enum value to its string representation.
 * @param name The AlgorithmName to convert.
 * @return String representation of the AlgorithmName.
 */
inline std::string to_string(AlgorithmName name) {
    switch (name) {
        case AlgorithmName::MIN_COST:        return "MinCost";
        case AlgorithmName::RANDOM:          return "Random";
        case AlgorithmName::GREEDY_AscDesc:  return "Greedy_AscDesc";
        case AlgorithmName::GREEDY_AscAsc:   return "Greedy_AscAsc";
        case AlgorithmName::GREEDY_DescAsc:  return "Greedy_DescAsc";
        case AlgorithmName::GREEDY_DescDesc: return "Greedy_DescDesc";
        case AlgorithmName::SA:              return "SA";
        default:                             return "UnknownAlgorithm";
    }
}

/**
 * @brief Enumerates the operational modes of the simulation.
 */
enum class SimulationMode {
    NONE,
    ONLINE,
    OFFLINE
};

/**
 * @brief Converts a SimulationMode enum value to its string representation.
 * @param mode The SimulationMode to convert.
 * @return String representation of the SimulationMode.
 */
inline std::string to_string(SimulationMode mode) {
    switch (mode) {
        case SimulationMode::ONLINE:  return "Online";
        case SimulationMode::OFFLINE: return "Offline";
        default:                      return "UnknownMode";
    }
}
#pragma endregion

#pragma region Devices_and_Servers_Structs

/**
 * @brief Represents a server capable of covering a device and its network timing metrics.
 */
struct server_covering {
    int id = 0;                  ///< @brief The unique identifier of the covering server.
    int id_routing = 0;          ///< @brief The ID of the edge server for routing if this is a cloud server.
    double distance = 0.0;       ///< @brief Geographic distance from the device to the server in km.
    double connectionTime = 0.0; ///< @brief Total network time (propagation + transmission) in ms.
    double processingTime = 0.0; ///< @brief Time for the server to process the device's task in ms.
    double responseTime = 0.0;   ///< @brief Total time (connectionTime + processingTime).

    server_covering() = default;
    explicit server_covering(int id_) : id(id_) {}
    server_covering(int id_, double distance_) : id(id_), distance(distance_) {}
};

/**
 * @brief Tracks the current resource demand and assigned devices for a specific server.
 */
struct server_supply {
    int pcnD = 0;                 ///< @brief Total number of processing cores required.
    double cndD = 0.0;            ///< @brief Sum of non-service costs for all served devices.
    double pccD = 0.0;            ///< @brief Total processing core capacity required.
    double memD = 0.0;            ///< @brief Total memory required.
    double stoD = 0.0;            ///< @brief Total storage required.
    double bwD = 0.0;             ///< @brief Total bandwidth required.
    std::set<int> devices_served; ///< @brief Set of unique IDs of devices currently served.
};

/**
 * @brief Represents a network device requesting resources.
 */
struct Device {
    int id, fileId, pcn, ttl;
    double lat, lon, cnd, pcc, mem, sto, s_d;
    double bw = 0.0;                      ///< @brief Assigned bandwidth based on network technology.
    bool covered = false;                 ///< @brief True if within range of at least one edge server.
    bool served = false;                  ///< @brief True if allocated to a server for processing.
    server_covering server;               ///< @brief The server ultimately assigned to this device.
    std::vector<server_covering> servers; ///< @brief List of potential servers that can serve this device.

    Device() : id(0), lat(0.0), lon(0.0), cnd(0.0), pcc(0.0), pcn(0), mem(0.0), sto(0.0), s_d(0.0), ttl(0), fileId(0) {}
    Device(int id_, double lat_, double lon_, double cnd_, double pcc_, int pcn_, double mem_, double sto_, double s_d_, int ttl_, int fileId_)
        : id(id_), lat(lat_), lon(lon_), cnd(cnd_), pcc(pcc_), pcn(pcn_), mem(mem_), sto(sto_), s_d(s_d_), ttl(ttl_), fileId(fileId_) {}
};

/**
 * @brief Represents an edge or cloud server capable of serving devices.
 */
struct Server {
    int id, fileId, pcn;
    char type;
    double lat, lon, csc, pcc_per_core, pcc_total, mem, sto, t_p;
    double bw = 0.0;      ///< @brief Maximum bandwidth capacity of the server.
    bool on = false;      ///< @brief True if the server is active (serving at least one device).
    server_supply supply; ///< @brief Current aggregated demand on the server's resources.

    Server() : id(0), pcn(0), lat(0.0), lon(0.0), csc(0.0), pcc_per_core(0.0), pcc_total(0.0), mem(0.0), sto(0.0), t_p(0.0), type(' '), fileId(0) {}
    Server(int id_, double lat_, double lon_, double csc_, double pcc_, int pcn_, double mem_, double sto_, double t_p_, char type_, int fileId_)
        : id(id_), pcn(pcn_), lat(lat_), lon(lon_), csc(csc_), pcc_per_core(pcc_), pcc_total(pcc_ * pcn_), mem(mem_), sto(sto_), t_p(t_p_), type(type_), fileId(fileId_) {}
    /**/

    /**
     * @brief Checks if the server has enough capacity to serve a given device.
     * @param device The device to be checked.
     * @return True if the server can accommodate the device, false otherwise.
     */
    inline bool canServe(const Device& device) const {
        return (
            device.bw  <= this->bw        - supply.bwD  &&
            device.mem <= this->mem       - supply.memD &&
            device.pcn <= this->pcn       - supply.pcnD &&
            device.pcc <= this->pcc_total - supply.pccD &&
            device.sto <= this->sto       - supply.stoD
        );
    }

    /**
     * @brief Allocates a device to this server and updates resource demands.
     * @param device The device to add.
     * @return True if the device was successfully added, false if it was already served.
     */
    inline bool addServed(Device& device) { 
        auto result = supply.devices_served.insert(device.id);
        if (!result.second) return false;
        
        this->on = true;
        device.served = true;
        supply.cndD += device.cnd;
        supply.pccD += device.pcc;
        supply.pcnD += device.pcn;
        supply.memD += device.mem;
        supply.stoD += device.sto;
        supply.bwD  += device.bw;
        return true;
    }

    /**
     * @brief Removes a device from this server and releases its resources.
     * @param device The device to remove.
     * @return True if the device was successfully removed, false if it was not found.
     */
    inline bool rmvServed(Device& device) {
        if (supply.devices_served.find(device.id) == supply.devices_served.end()) return false;
        
        supply.devices_served.erase(device.id);
        device.served = false;
        
        supply.cndD -= device.cnd;
        supply.pccD -= device.pcc;
        supply.pcnD -= device.pcn;
        supply.memD -= device.mem;
        supply.stoD -= device.sto;
        supply.bwD  -= device.bw;

        if (supply.devices_served.empty()) {
            this->on = false;
        }
        return true;
    }
};
#pragma endregion

#pragma region Metrics_Struct

/**
 * @brief Represents a formatted entry for displaying or saving metrics.
 */
struct MetricEntry {
    std::string category;
    std::string header;
    std::string label;
    std::string fileValue;
    std::string consoleValue;
};

/**
 * @brief Base structure for tracking, calculating, and saving simulation metrics.
 */
struct Metrics {
    SimulationType simulation_type;
    AlgorithmName algorithm_name;
    SimulationMode simulation_mode;
    int run_number;

    struct CommonInputs {
        int devices = 0;
        int servers_ec = 0;
        int servers_cc = 0;
        int tech = 0;
    } inputs;

    struct CommonOutputs {
        double execution_time_sec = 0.0;
        int    devices_covered_count = 0;
        int    devices_served_count = 0;
        int    devices_served_ec_count = 0;
        int    devices_served_cc_count = 0;
        int    servers_used_count = 0;
        int    servers_used_ec_count = 0;
        int    servers_used_cc_count = 0;
        double cost_of_servers_used = 0.0;
        double cost_of_non_coverage = 0.0;
        double cost_of_non_service = 0.0;
        double total_cost = 0.0;
        double average_response_time = 0.0;
    } outputs;

    struct TimeStep {
        int  time_step;
        iVec devices_idx;
        iVec covered_devices_idx;    
        iVec non_covered_devices_idx;
        iVec served_devices_idx;
        iVec non_served_devices_idx;
        iSet servers_used_idx;
    
        inline std::string toRowString() const {
            return utils::toString(time_step) + ";" + 
                utils::vecToString(devices_idx) + ";" + 
                utils::vecToString(covered_devices_idx) + ";" + 
                utils::vecToString(non_covered_devices_idx) + ";" + 
                utils::vecToString(served_devices_idx) + ";" + 
                utils::vecToString(non_served_devices_idx) + ";" + 
                utils::setToString(servers_used_idx);
        }
    } timeStep;

    Metrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_) :
        simulation_type(simulation_type_), algorithm_name(algorithm_name_), simulation_mode(simulation_mode_), run_number(run_number_) {}
    Metrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, int d, int s_ec, int s_cc, int t) :
        simulation_type(simulation_type_), algorithm_name(algorithm_name_), simulation_mode(simulation_mode_), run_number(run_number_), inputs({d, s_ec, s_cc, t}) {}
    Metrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, const std::shared_ptr<Metrics>& base) :
        simulation_type(simulation_type_), algorithm_name(algorithm_name_), simulation_mode(simulation_mode_), run_number(run_number_), inputs(base->inputs), outputs(base->outputs) {}
    Metrics(int d, int s_ec, int s_cc, int t) :
        simulation_type(SimulationType::NONE), algorithm_name(AlgorithmName::NONE), simulation_mode(SimulationMode::NONE), run_number(1), inputs({d, s_ec, s_cc, t}) {}
    Metrics(const std::shared_ptr<Metrics>& base, int run_number_) :
        simulation_type(base->simulation_type), algorithm_name(base->algorithm_name), 
        simulation_mode(base->simulation_mode), run_number(run_number_), 
        inputs(base->inputs), outputs(base->outputs) {}
    virtual ~Metrics() = default;
    
    /**
     * @brief Creates a deep copy of the current Metrics object.
     * @return Shared pointer to the new Metrics object.
     */
    virtual std::shared_ptr<Metrics> clone() const {
        return std::shared_ptr<Metrics>(new Metrics(*this));
    }

    /**
     * @brief Generates the base file name for metrics files based on scenario properties.
     * @return A string containing the formatted base file name.
     */
    inline std::string getBaseFileName() const {
        return "D" + std::to_string(this->inputs.devices) + "_S" + 
               std::to_string(this->inputs.servers_ec + this->inputs.servers_cc) + "_" + 
               std::to_string(this->inputs.tech) + "G";
    }
    
    /**
     * @brief Constructs the base directory path for storing results.
     * @return std::filesystem::path containing the result base directory.
     */
    virtual std::filesystem::path getBaseDirectoryPath() const {
        return std::filesystem::path("Results") / to_string(this->simulation_type) / to_string(this->algorithm_name) / to_string(this->simulation_mode) / this->getBaseFileName() ;
    }

    /**
     * @brief Constructs the specific run file name.
     * @return A string formatted as "Run_X".
     */
    inline std::string getRunFileName() const {
        return "Run_" + std::to_string(this->run_number);
    }

    /**
     * @brief Constructs the complete run directory path.
     * @return std::filesystem::path targeting the current run directory.
     */
    inline std::filesystem::path getRunDirectoryPath() const {
        return this->getBaseDirectoryPath() / this->getRunFileName();
    }

    /**
     * @brief Compiles a list of metric entries formatting all statistical outputs.
     * @param is_final Indicates if the output corresponds to the final aggregated metrics.
     * @return A vector of formatted MetricEntry structures.
     */
    virtual std::vector<MetricEntry> buildMetrics(bool is_final = false) const {
        int current_total = (is_final || simulation_mode != SimulationMode::ONLINE || timeStep.time_step == 0) ? inputs.devices : timeStep.devices_idx.size();
        int current_covered = outputs.devices_covered_count;
        int total_servers = inputs.servers_ec + inputs.servers_cc;
        double avg_rt = outputs.devices_served_count > 0 ? (outputs.average_response_time / (double)outputs.devices_served_count) : 0.0;

        std::vector<MetricEntry> m;

        m.push_back({"GENERAL", "Tech", "Mobile Technology", utils::toString(inputs.tech), std::to_string(inputs.tech) + "G"});
        m.push_back({"GENERAL", "ExeTime", "Execution Time (s)", utils::toString(outputs.execution_time_sec, 9), utils::toString(outputs.execution_time_sec, 9)});
        
        m.push_back({"DEVICES", "Devices", "Total", utils::toString(current_total), std::to_string(current_total)});
        m.push_back({"DEVICES", "DCovered", "Covered", utils::toPercentageString(current_covered, current_total), std::to_string(current_covered) + " (" + utils::toPercentageString(current_covered, current_total) + "%)"});
        m.push_back({"DEVICES", "DServed", "Served", utils::toPercentageString(outputs.devices_served_count, current_total), std::to_string(outputs.devices_served_count) + " (" + utils::toPercentageString(outputs.devices_served_count, current_total) + "%)"});
        m.push_back({"DEVICES", "DServedEC", "  - on EC", utils::toPercentageString(outputs.devices_served_ec_count, outputs.devices_served_count), std::to_string(outputs.devices_served_ec_count) + " (" + utils::toPercentageString(outputs.devices_served_ec_count, outputs.devices_served_count) + "% of served)"});
        m.push_back({"DEVICES", "DServedCC", "  - on CC", utils::toPercentageString(outputs.devices_served_cc_count, outputs.devices_served_count), std::to_string(outputs.devices_served_cc_count) + " (" + utils::toPercentageString(outputs.devices_served_cc_count, outputs.devices_served_count) + "% of served)"});

        m.push_back({"SERVERS", "Servers", "Total", utils::toString(total_servers), std::to_string(total_servers) + " (" + std::to_string(inputs.servers_ec) + " EC + " + std::to_string(inputs.servers_cc) + " CC)"});
        m.push_back({"SERVERS", "SUsed", "Used", utils::toPercentageString(outputs.servers_used_count, total_servers), std::to_string(outputs.servers_used_count) + " (" + utils::toPercentageString(outputs.servers_used_count, total_servers) + "%)"});
        m.push_back({"SERVERS", "SUsedEC", "  - Used EC", utils::toPercentageString(outputs.servers_used_ec_count, inputs.servers_ec), std::to_string(outputs.servers_used_ec_count) + " (" + utils::toPercentageString(outputs.servers_used_ec_count, inputs.servers_ec) + "% of EC)"});
        m.push_back({"SERVERS", "SUsedCC", "  - Used CC", utils::toPercentageString(outputs.servers_used_cc_count, inputs.servers_cc), std::to_string(outputs.servers_used_cc_count) + " (" + utils::toPercentageString(outputs.servers_used_cc_count, inputs.servers_cc) + "% of CC)"});

        m.push_back({"COSTS", "TotalCost", "TOTAL COST", utils::toString(outputs.total_cost, 6), utils::toString(outputs.total_cost, 6)});
        m.push_back({"COSTS", "CostNCoverage", "  - Cost Non-Coverage", utils::toString(outputs.cost_of_non_coverage, 6), utils::toString(outputs.cost_of_non_coverage, 6)});
        m.push_back({"COSTS", "CostNService", "  - Cost Non-Service", utils::toString(outputs.cost_of_non_service, 6), utils::toString(outputs.cost_of_non_service, 6)});
        m.push_back({"COSTS", "CostS", "  - Cost Servers Used", utils::toString(outputs.cost_of_servers_used, 6), utils::toString(outputs.cost_of_servers_used, 6)});

        m.push_back({"PERFORMANCE", "Avg.RTime", "Avg. Response Time (ms)", utils::toString(avg_rt, 6), utils::toString(avg_rt, 6)});

        m.push_back({"TIME", is_final ? "TotalSteps" : "TimeStep", is_final ? "Total Time Steps" : "Time Step", utils::toString(timeStep.time_step), utils::toString(timeStep.time_step)});

        return m;
    }

    /**
     * @brief Gets the CSV header line for intermediate steps.
     * @return A vector of header strings.
     */
    std::vector<std::string> getStepHeader() const {
        std::vector<std::string> res;
        for (const auto& entry : buildMetrics(false)) res.push_back(entry.header);
        return res;
    }

    /**
     * @brief Gets the CSV header line for final metrics.
     * @return A vector of header strings.
     */
    std::vector<std::string> getFinalHeader() const {
        std::vector<std::string> res;
        for (const auto& entry : buildMetrics(true)) res.push_back(entry.header);
        return res;
    }

    /**
     * @brief Collects the metric values for saving to files.
     * @param is_final Indicates if the data corresponds to the final aggregated metrics.
     * @return A vector of data strings.
     */
    std::vector<std::string> data(bool is_final = false) const {
        std::vector<std::string> res;
        for (const auto& entry : buildMetrics(is_final)) res.push_back(entry.fileValue);
        return res;
    }

    /**
     * @brief Appends the final metrics data to the general result file.
     */
    inline void saveResultFinalToFile() const {
        std::filesystem::path result_path = this->getBaseDirectoryPath() / (this->getBaseFileName() + ".txt");
        std::vector<std::vector<std::string>> content;
        if (!std::filesystem::exists(result_path)) {
            content.push_back(this->getFinalHeader());
        }
        content.push_back(this->data(true));
        FileManager::append(result_path.string(), content, ';');
    }

    /**
     * @brief Appends the intermediate step metrics to the specific run files.
     */
    inline void saveResultStepToFile() const {
        std::filesystem::path step_dir = getRunDirectoryPath();
        std::filesystem::create_directories(step_dir); 

        std::filesystem::path metrics_path = step_dir / (getRunFileName() + ".txt");
        std::filesystem::path vectors_path = step_dir / (getRunFileName() + "_vectors.txt");
        
        std::vector<std::vector<std::string>> contentMetrics;
        if (!std::filesystem::exists(metrics_path)) {
            contentMetrics.push_back(this->getStepHeader());
        }
        contentMetrics.push_back(this->data(false));
        FileManager::append(metrics_path.string(), contentMetrics, ';');
        
        std::vector<std::vector<std::string>> contentVectors;
        if (!std::filesystem::exists(vectors_path)) {
            contentVectors.push_back({"TimeStep", "DevicesIdx", "CoveredDevicesIdx", "NonCoveredDevicesIdx", "ServedDevicesIdx", "NonServedDevicesIdx", "UsedServersIdx"});
        }
        contentVectors.push_back({ timeStep.toRowString() }); 
        FileManager::append(vectors_path.string(), contentVectors, ';');
    }
};

/**
 * @brief Extends Metrics struct to include variables specific to exact mathematical models.
 */
struct ExactMetrics : public Metrics {
    std::string status = "Unknown"; ///< @brief Status of the CPLEX solver.
    double OF = 0.0;                ///< @brief Objective Function value.
    double gap = 1.0;               ///< @brief Final MIP relative gap.

    ExactMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_) : 
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_) {}
    ExactMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, int d, int s_ec, int s_cc, int t) : 
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_, d, s_ec, s_cc, t) {}
    ExactMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, const std::shared_ptr<Metrics>& base) :
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_, base) {}
    ExactMetrics(const std::shared_ptr<Metrics>& base, int run_number_) :
        Metrics(base, run_number_) {}
    
    std::shared_ptr<Metrics> clone() const override {
        return std::shared_ptr<ExactMetrics>(new ExactMetrics(*this));
    }

    std::vector<MetricEntry> buildMetrics(bool is_final = false) const override {
        auto m = Metrics::buildMetrics(is_final);
        m.push_back({"SOLVER STATS", "Status", "Solver Status", status, status});
        m.push_back({"SOLVER STATS", "OF", "Objective Function (OF)", utils::toString(OF, 6), utils::toString(OF, 6)});
        m.push_back({"SOLVER STATS", "GAP", "MIP Gap", utils::toString(gap, 6), utils::toPercentageString(gap, 1.0) + "%"});
        return m;
    }
};

/**
 * @brief Identifies metrics specific to heuristic models.
 */
struct HeuristicMetrics : public Metrics {

    HeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_) : 
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_) {}
    HeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, int d, int s_ec, int s_cc, int t) :
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_, d, s_ec, s_cc, t) {}
    HeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, const std::shared_ptr<Metrics>& base) :
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_, base) {}
    HeuristicMetrics(const std::shared_ptr<Metrics>& base, int run_number_) :
        Metrics(base, run_number_) {}

    std::shared_ptr<Metrics> clone() const override {
        return std::shared_ptr<HeuristicMetrics>(new HeuristicMetrics(*this));
    }
};

/**
 * @brief Extends Metrics struct to include parameters required by meta-heuristic algorithms (e.g., SA).
 */
struct MetaHeuristicMetrics : public Metrics {
    AlgorithmName heuristic = AlgorithmName::NONE; ///< @brief The starting heuristic solution approach.
    double temperature = 0.0;                      ///< @brief Initial simulated annealing temperature.
    double alpha = 0.0;                            ///< @brief Cooling rate.

    MetaHeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_) : 
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_) {}
    MetaHeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, int d, int s_ec, int s_cc, int t, double temp, double alph, AlgorithmName heuristic_) :
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_, d, s_ec, s_cc, t), temperature(temp), alpha(alph), heuristic(std::move(heuristic_)) {}
    MetaHeuristicMetrics(SimulationType simulation_type_, AlgorithmName algorithm_name_, SimulationMode simulation_mode_, int run_number_, double temp, double alph, AlgorithmName heuristic_) :
        Metrics(simulation_type_, algorithm_name_, simulation_mode_, run_number_), temperature(temp), alpha(alph), heuristic(std::move(heuristic_)) {}
    MetaHeuristicMetrics(const std::shared_ptr<Metrics>& base, int run_number_) :
        Metrics(base, run_number_) {}    
    
    std::shared_ptr<Metrics> clone() const override {
        return std::shared_ptr<MetaHeuristicMetrics>(new MetaHeuristicMetrics(*this));
    }

    inline std::filesystem::path getBaseDirectoryPath() const override {
        std::string t_str = utils::toString(temperature, 0);
        std::string a_str = utils::toString(alpha, 2);
        return Metrics::getBaseDirectoryPath() / to_string(heuristic) / t_str / a_str;
    }

    std::vector<MetricEntry> buildMetrics(bool is_final = false) const override {
        auto m = Metrics::buildMetrics(is_final);
        std::string cat = to_string(algorithm_name) + " PARAMETERS";
        m.push_back({cat, "Heuristic", "Initial Solution", to_string(heuristic), to_string(heuristic)});
        m.push_back({cat, "Temperature", "Initial Temperature", utils::toString(temperature, 2), utils::toString(temperature, 2)});
        m.push_back({cat, "Alpha", "Alpha (Cooling Rate)", utils::toString(alpha, 2), utils::toString(alpha, 2)});
        return m;
    }
};
#pragma endregion

/**
 * @brief Encapsulates the complete state and tracked metrics for a single simulation run.
 */
struct Result {
    Devices devices;                          ///< @brief System state containing all devices.
    Servers servers;                          ///< @brief System state containing all servers.
    iVec activeDevicesIdx;                    ///< @brief Identifies currently active devices per time step.
    double serversUsedPercentage = 0.0;       ///< @brief Tracks the percentage of active servers globally.

    std::vector<std::shared_ptr<Metrics>> stepMetrics; ///< @brief Records intermediate step-by-step metrics.
    std::shared_ptr<Metrics> finalMetrics;             ///< @brief Holds the final compiled output metrics.
    
    Result(Devices d, Servers s)
        : devices(std::move(d)), servers(std::move(s)) {}
    Result(const Result& other)
        : devices(other.devices), 
          servers(other.servers),
          activeDevicesIdx(other.activeDevicesIdx),
          serversUsedPercentage(other.serversUsedPercentage)
          {
        
        for (const auto& m : other.stepMetrics) {
            if (m) stepMetrics.push_back(m->clone());
        }
        if (other.finalMetrics) {
            finalMetrics = other.finalMetrics->clone();
        }
    }

    Result& operator=(const Result& other) {
        if (this != &other) {
            devices = other.devices;
            servers = other.servers;
            activeDevicesIdx = other.activeDevicesIdx;
            
            stepMetrics.clear();
            for (const auto& m : other.stepMetrics) {
                if (m) stepMetrics.push_back(m->clone());
            }
            
            if (other.finalMetrics) finalMetrics = other.finalMetrics->clone(); 
            else finalMetrics.reset();
        }
        return *this;
    }

    /**
     * @brief Saves utilized server vectors mapped by run number.
     */
    inline void saveVectorsToFile() const {
        if (!finalMetrics) {
            std::cerr << "Error: finalMetrics is null. Cannot save vectors." << std::endl;
            return;
        }

        std::filesystem::path base_dir = finalMetrics->getBaseDirectoryPath();
        std::filesystem::create_directories(base_dir);
        std::filesystem::path file_path = base_dir / (finalMetrics->getBaseFileName() + "_vectors.txt");

        std::ofstream file(file_path, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << file_path << " for writing vectors." << std::endl;
            return;
        }

        // Write header if file is empty
        if (file.tellp() == 0) {
            file << "RunNumber;UsedServersIdx\n";
        }

        file << finalMetrics->run_number << ";" << utils::setToString(finalMetrics->timeStep.servers_used_idx) << "\n";
        file.close();
    }
};

/**
 * @brief Utility namespace for displaying struct details natively via console or files.
 * @details Implements visualization templates for objects such as Devices, Servers, and complex formatted Metric outputs.
 */
namespace showStructs {

    /**
     * @brief Prints device characteristics and allocation status to the console.
     * @param device The device to evaluate and display.
     */
    inline void showDevice(const Device& device) {
        std::cout << "========== Device ID: " << device.id << " ==========\n"
                  << "  - Location (Lat, Lon):  (" << device.lat << ", " << device.lon << ")\n"
                  << "  - Time To Live (TTL):   " << device.ttl << "\n"
                  << "  - Requirements (CND):   " << device.cnd << "\n"
                  << "  - Requirements (PCC):   " << device.pcc << "\n"
                  << "  - Requirements (PCN):   " << device.pcn << "\n"
                  << "  - Requirements (MEM):   " << device.mem << "\n"
                  << "  - Requirements (STO):   " << device.sto << "\n"
                  << "  - Requirements (S_d):   " << device.s_d << "\n"
                  << "  - State (Bandwidth):    " << device.bw << " Mbps\n"
                  << "  - State (Covered):      " << (device.covered ? "Yes" : "No") << "\n"
                  << "  - State (Served):       " << (device.served ? "Yes" : "No") << "\n";

        if (device.served) {
            std::cout << "  - Assigned Server ID:   " << device.server.id
                      << " (Response Time: " << device.server.responseTime << " ms)\n";
        } else {
            std::cout << "  - Assigned Server ID:   None\n";
        }

        std::cout << "  - Potential Servers (" << device.servers.size() << "):\n";
        if (device.servers.empty()) {
            std::cout << "    - None\n";
        } else {
            for (const auto& s : device.servers) {
                std::cout << "    - Server ID: " << std::setw(3) << s.id
                          << " | Response Time: " << std::fixed << std::setprecision(4) << s.responseTime << " ms\n";
            }
        }
        std::cout << "====================================\n" << std::endl;
    }

    /**
     * @brief Batch prints an array list of all devices sequentially.
     * @param devices The reference to a vector of Devices.
     */
    inline void showDevice(const Devices& devices) {
        std::cout << "\n--- Displaying " << devices.size() -1 << " Devices ---\n";
        for (const auto& device : devices) {
            if (device.id == 0) continue; // Skip placeholder at index 0
            showDevice(device);
        }
    }

    /**
     * @brief Prints server capacities, demands, and served connections to the console.
     * @param server The server to evaluate and display.
     */
    inline void showServer(const Server& server) {
        std::cout << "========== Server ID: " << server.id << " (Type: " << server.type << ") ==========\n"
                  << "  - Location (Lat, Lon): " << server.lat << ", " << server.lon << "\n"
                  << "  - Status (ON):         " << (server.on ? "Yes" : "No") << "\n\n"
                  << "  --- Capacity ---\n"
                  << "  - Cost (CSC):          " << server.csc << "\n"
                  << "  - PCC per Core:        " << server.pcc_per_core << "\n"
                  << "  - Total PCC:           " << server.pcc_total << "\n"
                  << "  - Core Count (PCN):    " << server.pcn << "\n"
                  << "  - Memory (MEM):        " << server.mem << "\n"
                  << "  - Storage (STO):       " << server.sto << "\n"
                  << "  - Bandwidth (BW):      " << server.bw << " Mbps\n"
                  << "  - Proc. Time (T_p):    " << server.t_p << "\n\n"
                  << "  --- Current Demand ---\n"
                  << "  - Demand (PCC):        " << server.supply.pccD << "\n"
                  << "  - Demand (PCN):        " << server.supply.pcnD << "\n"
                  << "  - Demand (MEM):        " << server.supply.memD << "\n"
                  << "  - Demand (STO):        " << server.supply.stoD << "\n"
                  << "  - Demand (BW):         " << server.supply.bwD << "\n"
                  << "  - Devices Served (" << server.supply.devices_served.size() << "): ";

        if (server.supply.devices_served.empty()) {
            std::cout << "None\n";
        } else {
            std::string device_list;
            for (int device_id : server.supply.devices_served) {
                device_list += std::to_string(device_id) + ", ";
            }
            // Remove trailing comma and space
            std::cout << device_list.substr(0, device_list.length() - 2) << "\n";
        }
        std::cout << "===============================================\n" << std::endl;
    }

    /**
     * @brief Batch prints an array list of all servers sequentially.
     * @param servers The reference to a vector of Servers.
     */
    inline void showServer(const Servers& servers) {
        std::cout << "\n--- Displaying " << servers.size() - 1 << " Servers ---\n";
        for (const auto& server : servers) {
            if (server.id == 0) continue; // Skip placeholder at index 0
            showServer(server);
        }
    }

    namespace {
        const int total_width = 62;
        const int label_width = 26; 
       
        /**
         * @brief Outputs a formatted metric row line in a table visual style.
         * @param out The target output stream.
         * @param label Table row descriptor.
         * @param value Processed value as a string.
         */
        inline void print_row (std::ostream& out, const std::string& label, const std::string& value) {
            out << "| " << std::left << std::setw(label_width) << label
                        << " | "<< std::left << std::setw(total_width - label_width - 7) << value
                        << " |" << std::endl;
        }

        /**
         * @brief Standard visual boundary template string printer logic.
         * @param out Target output stream.
         */
        inline void print_header(std::ostream& out) {
            out << "+" << std::string(total_width - 2, '=') << "+" << std::endl;
        };

        /**
         * @brief Standard visual separator template string printer logic.
         * @param out Target output stream.
         */
        inline void print_midle(std::ostream& out) {
            out << "+" << std::string(label_width + 2, '-') << "+"
                    << std::string(total_width - label_width - 5, '-') << "+" << std::endl;
        };
        
        /**
         * @brief Calculates spacing and formats dynamic titles centrally aligned.
         * @param out Target output stream.
         * @param title Title structure representing category chunks.
         */
        inline void print_title(std::ostream& out, const std::string& title) {
            int padding_total = total_width - 3 - title.length();
            int padding_left = padding_total / 2;
            int padding_right = padding_total - padding_left + 1;
            out << "|" << std::string(padding_left, ' ') << title
                    << std::string(padding_right, ' ') << "|" << std::endl;
        };
    }

    /**
     * @brief Organizes structured metrics objects into human-readable visual layouts directed at an output stream.
     * @param metrics The metrics tracker entity struct.
     * @param out Standard output or filestream reference.
     * @param is_final Defines output style (intermediate step vs final results).
     */
    inline void showMetricsConsoleOrFile(const Metrics& metrics, std::ostream& out, bool is_final = false) {
        out << std::endl;
        print_header(out);
        
        std::string title = "SIMULATION " + to_string(metrics.simulation_type) + " " + to_string(metrics.simulation_mode);
        if (metrics.simulation_mode == SimulationMode::ONLINE && !is_final) {
            print_title(out, title + " - STEP " + std::to_string(metrics.timeStep.time_step));
        } else {
            print_title(out, title); 
        }
        print_midle(out);

        print_row(out, "Algorithm", to_string(metrics.algorithm_name));
        print_row(out, "Simulation Run", std::to_string(metrics.run_number));

        auto entries = metrics.buildMetrics(is_final);
        std::string current_category = "";

        for (const auto& entry : entries) {
            if (entry.category != current_category) {
                print_midle(out);
                if (entry.category != "GENERAL" && entry.category != "PERFORMANCE" && entry.category != "TIME") {
                    print_title(out, entry.category);
                    print_midle(out);
                }
                current_category = entry.category;
            }
            
            if (entry.category == "TIME") {
                if (is_final && metrics.simulation_mode == SimulationMode::ONLINE) {
                    print_row(out, entry.label, entry.consoleValue);
                }
                continue; 
            }

            print_row(out, entry.label, entry.consoleValue);
        }
        print_header(out);
    }

    /**
     * @brief Triggers the generation of console and text-based diagrams for generic tracking.
     * @param ptr The metrics instance pointer wrapper.
     * @param is_final Marks if drawing corresponds to aggregated results layout.
     */
    inline void showMetrics(const std::shared_ptr<Metrics>& ptr, bool is_final = false) {
        if (!ptr) return;

        showMetricsConsoleOrFile(*ptr, std::cout, is_final);

        std::filesystem::path file_path;
        if (is_final) {
            std::filesystem::path base_dir = ptr->getBaseDirectoryPath(); 
            std::filesystem::create_directories(base_dir);
            file_path = base_dir / (ptr->getBaseFileName() + "_drawing.txt");
        } else {
            std::filesystem::path run_dir = ptr->getRunDirectoryPath();
            std::filesystem::create_directories(run_dir);
            file_path = run_dir / (ptr->getRunFileName() + "_drawing.txt");
        }
        
        std::ofstream file(file_path, std::ios::app); 
        if (file.is_open()) {
            showMetricsConsoleOrFile(*ptr, file, is_final);
        } else {
            std::cerr << "Error: Could not save drawing to " << file_path << std::endl;
        }
    }
}

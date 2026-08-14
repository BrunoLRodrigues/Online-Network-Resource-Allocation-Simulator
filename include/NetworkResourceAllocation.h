#pragma once

#include "DataGenerator.h"
#include "structs.h"
#include "utils.h"

#include <chrono>

/**
 * @brief Core simulation logic for network resource allocation.
 * @details Handles data loading, coverage calculation, entity sorting, metric tracking, and simulation state management.
 */
namespace NetworkResourceAllocation {

    #pragma region DataLoading

    /**
     * @brief Loads device data from generated files into a vector of Device objects.
     * @param length The total number of devices to load.
     * @param MAX_TTL The maximum time-to-live parameter for the devices.
     * @param numServices The number of distinct services to generate. Defaults to 10.
     * @return A vector of Device objects containing the parsed data.
     */
    inline Devices loadDevices(const int length, const int MAX_TTL, const int numServices = 10) {
        std::vector<std::vector<std::string>> strDevices = DataGenerator::devicesData(length, MAX_TTL, numServices);
        if (strDevices.size() <= 1) {
            std::cerr << "Error: Device data file not found or is empty." << std::endl;
            return {};
        }

        Devices devices;
        devices.reserve(length + 1);
        devices.emplace_back(); // Placeholder for 1-based indexing.
        
        for (const auto& row : strDevices) {
            if (row.at(0) == "#") continue;
            try {
                devices.emplace_back(
                    std::stoi(row.at(0)),  // #: id
                    std::stod(row.at(1)),  // LAT: lat
                    std::stod(row.at(2)),  // LON: lon
                    std::stod(row.at(3)),  // CND: cnd
                    std::stod(row.at(4)),  // PCC: pcc
                    std::stoi(row.at(5)),  // PCN: pcn
                    std::stod(row.at(6)),  // MEM: mem
                    std::stod(row.at(7)),  // STO: sto
                    std::stod(row.at(8)),  // S_d: s_d
                    std::stoi(row.at(9)),  // TTL: ttl
                    std::stoi(row.at(10))  // FILE_ID: fileId
                );
            } catch (const std::exception& e) {
                std::cerr << "Error parsing device data: " << e.what() << std::endl;
                break;
            }
        }
        return devices;
    }
    
    /**
     * @brief Loads edge and cloud server data from generated files.
     * @param ecLength The number of Edge Computing (EC) servers to load.
     * @param ccLength The number of Cloud Computing (CC) servers to load. Defaults to 5.
     * @return A vector of Server objects containing both EC and CC data.
     */
    inline Servers loadServers(const int ecLength, const int ccLength = 5) {
        std::vector<std::vector<std::string>> strServerEC = DataGenerator::ecData(ecLength);
        std::vector<std::vector<std::string>> strServerCC = DataGenerator::ccData(ccLength);
        if (strServerEC.size() <= 1 || strServerCC.size() <= 1) {
            std::cerr << "Error: Server data file(s) not found or are empty." << std::endl;
            return {};
        }

        Servers servers;
        servers.reserve(ecLength + ccLength + 1);
        servers.emplace_back(); // Placeholder for 1-based indexing.

        auto parseAndAdd = [&](const std::vector<std::vector<std::string>>& matrix, char type) {
            for (const auto& row : matrix) {
                if (row.at(0) == "#") continue;
                int id = (type == 'E') ? std::stoi(row.at(0)) : std::stoi(row.at(0)) + ecLength; // CC IDs start after EC IDs
                try {
                    servers.emplace_back(
                        id,                    // id
                        std::stod(row.at(1)),  // LAT: lat
                        std::stod(row.at(2)),  // LON: lon
                        std::stod(row.at(3)),  // CSC: csc
                        std::stod(row.at(4)),  // PCC: pcc
                        std::stoi(row.at(5)),  // PCN: pcn
                        std::stod(row.at(6)),  // MEM: mem
                        std::stod(row.at(7)),  // STO: sto
                        std::stod(row.at(8)),  // T_p: t_p
                        type,
                        std::stoi(row.at(0))   // FILE_ID: fileId
                    );
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing server data: " << e.what() << std::endl;
                }
            }
        };

        parseAndAdd(strServerEC, 'E');
        parseAndAdd(strServerCC, 'C');
        
        return servers;
    }    
    #pragma endregion

    #pragma region Sorting

    /**
     * @brief Sorts a specific subset of device indices by their non-service cost (cnd) in ascending order.
     * @param devices The constant vector of devices.
     * @param indices_to_sort The vector of device indices to sort.
     * @return A new vector containing the sorted device indices.
     */
    inline iVec devicesAsc(const Devices& devices, const iVec& indices_to_sort) {
        return utils::sortEntities<true, Device, const double, &Device::cnd>(devices, indices_to_sort);
    }

    /**
     * @brief Sorts a specific subset of device indices by their non-service cost (cnd) in descending order.
     * @param devices The constant vector of devices.
     * @param indices_to_sort The vector of device indices to sort.
     * @return A new vector containing the sorted device indices.
     */
    inline iVec devicesDesc(const Devices& devices, const iVec& indices_to_sort) {
        return utils::sortEntities<false, Device, const double, &Device::cnd>(devices, indices_to_sort);
    }

    namespace details {

        /**
         * @brief Internal implementation for sorting a device's potential servers by activation cost.
         * @tparam Ascending If true, sorts from lowest to highest cost. Otherwise, highest to lowest.
         * @param device The device whose servers vector will be sorted.
         * @param servers The global vector of servers used to retrieve costs and response times.
         */
        template <bool Ascending>
        inline void sortServersByCostImpl(Device& device, const Servers& servers) {
            auto comparator = [&servers](const server_covering& a, const server_covering& b) {
                const Server& sA = servers.at(a.id);
                const Server& sB = servers.at(b.id);
                           
                double costA = sA.csc;
                double costB = sB.csc;

                // Tie-breaker: Always prefer lower latency if costs are identical
                if (costA == costB) {
                    return a.responseTime < b.responseTime; 
                }
                
                if constexpr (Ascending) {
                    return costA < costB;
                } else {
                    return costA > costB;
                }
            };

            std::sort(device.servers.begin(), device.servers.end(), comparator);
        }
    }

    /**
     * @brief Sorts the potential servers of a device by activation cost in ascending order.
     * @param device The device to modify.
     * @param servers The global vector of servers.
     */
    inline void sortServersAsc(Device& device, const Servers& servers) {
        details::sortServersByCostImpl<true>(device, servers);
    }

    /**
     * @brief Sorts the potential servers of a device by activation cost in descending order.
     * @param device The device to modify.
     * @param servers The global vector of servers.
     */
    inline void sortServersDesc(Device& device, const Servers& servers) {
        details::sortServersByCostImpl<false>(device, servers);
    }
    #pragma endregion

    #pragma region Coverage, Bandwidth and ResponseTime

    /**
     * @brief Calculates the geographic distance between two entities.
     * @tparam T1 The type of the first geographic entity.
     * @tparam T2 The type of the second geographic entity.
     * @param pointA The first geographic entity.
     * @param pointB The second geographic entity.
     * @return The distance between the two points in kilometers.
     */
    template <typename T1, typename T2>
    inline double calculateDistance(const T1& pointA, const T2& pointB) {
        return utils::calculateDistance(pointA.lat, pointA.lon, pointB.lat, pointB.lon);
    }

    /**
     * @brief Retrieves network parameters for a given technology generation.
     * @param tech The technology generation identifier (1 to 6).
     * @return A pair containing the coverage radius (km) and data rate (Mbps).
     */
    inline std::pair<double, double> techParams(int tech) {
        switch (tech) {
            case 1: return {20.0, 0.0024};   // 1G+
            case 2: return {10.0, 0.064};    // 2G
            case 3: return {5.0, 2.0};       // 3G
            case 4: return {3.0, 100.0};     // 4G
            case 5: return {0.6, 1000.0};    // 5G
            case 6: return {0.32, 10000.0};  // 6G
            default:
                std::cerr << "Error: Unknown technology type: " << tech << std::endl;
                return {-1.0, -1.0};
        }
    }

    /**
     * @brief Assigns bandwidth capacities to all provided devices and servers.
     * @param devices The vector of devices to update.
     * @param servers The vector of servers to update.
     * @param dataRate The network data rate in Mbps assigned to devices.
     */
    inline void bandwidth(Devices& devices, Servers& servers, double dataRate) {
        /**
         * @brief High-speed backbone data rate capacity between Edge and Cloud servers in Mbps.
         */
        constexpr double DATA_RATE_EC_TO_CC_MBPS = 100000.0;
        for (size_t i = 1; i < devices.size(); ++i) {
            devices[i].bw = dataRate;
        }
        for (size_t i = 1; i < servers.size(); ++i) {
            servers[i].bw = DATA_RATE_EC_TO_CC_MBPS;
        }
    }

    /**
     * @brief Identifies which devices are within the coverage radius of edge servers.
     * @details Also computes the cost of non-coverage for out-of-range devices.
     * @param devices The vector of devices to check.
     * @param servers The vector of available servers.
     * @param coverageRadius The maximum allowable distance (km) for coverage.
     * @param metrics The metrics object to update with non-coverage costs.
     * @return A vector of indices representing the covered devices.
     */
    inline iVec findCovering(Devices& devices, Servers& servers, double coverageRadius, Metrics& metrics) {
        iVec coveredDeviceIds;
        for (size_t i = 1; i < devices.size(); ++i) {
            Device& device = devices[i];
            
            for (size_t j = 1; j < servers.size(); ++j) {
                const Server& server = servers[j];
                if (server.type != 'E') continue;

                double distance = calculateDistance(device, server);
                if (distance <= coverageRadius) {
                    device.servers.emplace_back((int) j, distance);
                    device.covered = true;
                }
            }

            if (device.covered) {
                coveredDeviceIds.push_back(device.id);
                for (size_t j = 1; j < servers.size(); ++j) {
                    const Server& server = servers[j];
                    if (server.type == 'C') {
                        device.servers.emplace_back((int) j);
                    }
                }
            } else {
                metrics.outputs.cost_of_non_coverage += device.cnd;
            }
        }
        metrics.outputs.devices_covered_count = coveredDeviceIds.size();
        return coveredDeviceIds;
    }

    /**
     * @brief The speed of light in a vacuum, measured in kilometers per second (km/s).
     */
    constexpr long double SPEED_OF_LIGHT = 299792.458L;

    /**
     * @brief Base latency between AWS servers from Milan to Ohio in milliseconds (verified on 2024-12-18).
     */
    constexpr double INTER_DC_LATENCY_MS = 111.86;

    /**
     * @brief Calculates connection, processing, and total response times for covered device-server pairs.
     * @param devices The vector of devices to update.
     * @param servers The vector of servers used to calculate timing parameters.
     */
    inline void timeCalculation(Devices& devices, Servers& servers) {
        for (size_t i = 1; i < devices.size(); ++i) {
            Device& device = devices[i];
            if (!device.covered) continue;

            std::pair<int, double> closestEdge = {0, utils::EARTH_RADIUS_KM};
            for (const auto& s_info : device.servers) {
                if (servers.at(s_info.id).type == 'E' && s_info.distance < closestEdge.second) {
                    closestEdge = {s_info.id, s_info.distance};
                }
            }

            for (auto& s : device.servers) {
                Server& server = servers.at(s.id);
                s.processingTime = device.s_d * server.t_p;
                double transmission_time_ms = (device.s_d / device.bw) * 1000.0;
                
                if (server.type == 'C') {
                    s.id_routing = closestEdge.first;
                    double propagation_dist = closestEdge.second + calculateDistance(servers.at(closestEdge.first), server);
                    double propagation_delay_ms = (propagation_dist / SPEED_OF_LIGHT) * 1000.0;
                    s.connectionTime = transmission_time_ms + propagation_delay_ms + INTER_DC_LATENCY_MS;
                } else {
                    double propagation_delay_ms = (s.distance / SPEED_OF_LIGHT) * 1000.0;
                    s.connectionTime = transmission_time_ms + propagation_delay_ms;
                }
                s.responseTime = (2.0 * s.connectionTime) + s.processingTime;
            }
        }
    }

    /**
     * @brief Orchestrates the complete network coverage and timing calculation phase.
     * @param devices The vector of devices.
     * @param servers The vector of servers.
     * @param metrics The metrics object to track the coverage state.
     * @return A vector containing the indices of all successfully covered devices.
     */
    inline iVec coverage(Devices& devices, Servers& servers, Metrics& metrics) {
        std::pair<double, double> techProps = techParams(metrics.inputs.tech);
        if (techProps.first < 0) {
            std::cerr << "Error: Invalid technology ID provided." << std::endl;
            return {};
        }
        bandwidth(devices, servers, techProps.second);
        iVec coveredDevices = findCovering(devices, servers, techProps.first, metrics);
        timeCalculation(devices, servers);
        return coveredDevices;
    }
    #pragma endregion

    #pragma region Startup

    /**
     * @brief Initializes and prepares the simulation state before execution.
     * @details Loads data, applies Poisson scheduling for online modes, and runs initial coverage algorithms.
     * @param numDevices_or_numSteps Total devices (offline) or total steps (online).
     * @param numServersEC Number of Edge Computing servers.
     * @param numServersCC Number of Cloud Computing servers.
     * @param tech Mobile technology generation (1-6).
     * @param simulation_mode The mode of the simulation (ONLINE or OFFLINE).
     * @param MAX_TTL Maximum time-to-live for devices.
     * @param lambda The Poisson distribution mean for arrival rates.
     * @param isDevice If true, treats the first parameter as total devices; otherwise, as steps.
     * @return An optional Result struct containing the initialized simulation state, or nullopt on failure.
     */
    inline std::optional<Result> pre_calculation(int numDevices_or_numSteps, int numServersEC, int numServersCC, int tech, SimulationMode simulation_mode, int MAX_TTL, int lambda, bool isDevice = true) {
        iVec distribution;
        if (simulation_mode == SimulationMode::ONLINE) {
            if (isDevice) {
                distribution = DataGenerator::poissonSchedule(numDevices_or_numSteps, lambda, true);
            } else {
                distribution = DataGenerator::poissonSchedule(numDevices_or_numSteps, lambda, false);
            }
        } else {
            distribution = {numDevices_or_numSteps};
        }

        if (numDevices_or_numSteps <= 0 || numServersEC <= 0 || numServersCC <= 0) {
            std::cerr << "Error: Number of devices and servers must be positive." << std::endl;
            return std::nullopt;
        }
        
        int numDevices = 0;
        for (auto i : distribution) {
            numDevices += i;
        }

        Devices devices = loadDevices(numDevices, MAX_TTL);
        Servers servers = loadServers(numServersEC, numServersCC);

        if (devices.empty() || servers.empty()) {
            std::cerr << "Error: Failed to load device or server data." << std::endl;
            return std::nullopt;
        }

        iMatrix stepMatrix = utils::sliceBySchedule(1, numDevices, distribution);

        auto baseMetrics = std::make_shared<Metrics>(SimulationType::NONE, AlgorithmName::NONE, simulation_mode, 0, numDevices, numServersEC, numServersCC, tech);
        baseMetrics->timeStep.time_step = 0; 
        baseMetrics->timeStep.devices_idx = stepMatrix[0];
        baseMetrics->timeStep.covered_devices_idx = coverage(devices, servers, *baseMetrics);

        Result result(std::move(devices), std::move(servers));
        result.stepMetrics.push_back(baseMetrics);

        for (int i = 1; i < stepMatrix.size(); ++i) {
            auto stepM = std::make_shared<Metrics>(*baseMetrics);
            stepM->timeStep.covered_devices_idx.clear();
            stepM->timeStep.non_covered_devices_idx.clear();
            stepM->outputs.devices_covered_count = 0;
            stepM->outputs.cost_of_non_coverage = 0.0;
            
            stepM->timeStep.time_step = i;
            stepM->timeStep.devices_idx = stepMatrix[i];
            for (auto d_idx : stepMatrix[i]) {
                if (result.devices[d_idx].covered) {
                    stepM->timeStep.covered_devices_idx.push_back(result.devices[d_idx].id);
                } else {
                    baseMetrics->timeStep.non_covered_devices_idx.push_back(result.devices[d_idx].id);
                    stepM->timeStep.non_covered_devices_idx.push_back(result.devices[d_idx].id);
                    stepM->outputs.cost_of_non_coverage += result.devices[d_idx].cnd;
                }
            }
            stepM->outputs.devices_covered_count = stepM->timeStep.covered_devices_idx.size();
            result.stepMetrics.push_back(stepM);
        }
        
        return result;
    }

    /**
     * @brief Decrements the Time-To-Live (TTL) of active devices and removes expired ones.
     * @param state The unified simulation result state containing devices and servers.
     */
    inline void manageTTL(Result& state) {
        for (int i = state.activeDevicesIdx.size() - 1; i >= 0; --i) {
            int d_idx = state.activeDevicesIdx[i];
            Device& device = state.devices.at(d_idx);
            
            device.ttl--;

            if (device.ttl <= 0) {
                state.servers.at(device.server.id).rmvServed(device);
                
                state.activeDevicesIdx[i] = state.activeDevicesIdx.back();
                state.activeDevicesIdx.pop_back();
            }
        }
    }

    /**
     * @brief Sorts the available servers for a specific device based on activation cost.
     * @param device The device whose servers will be sorted.
     * @param servers The global list of servers to query costs.
     * @param sortAsc If true, sorts in ascending order; otherwise, descending order.
     */
    inline void sortServersByCost(Device& device, const Servers& servers, bool sortAsc) {
        auto comparator = [&servers, sortAsc](const server_covering& a, const server_covering& b) {
            const Server& sA = servers.at(a.id);
            const Server& sB = servers.at(b.id);
                       
            double costA = sA.csc;
            double costB = sB.csc;

            // Tie-breaker: Always prefer lower latency if costs are identical
            if (costA == costB) {
                return a.responseTime < b.responseTime; 
            }
            
            return sortAsc ? (costA < costB) : (costA > costB);
        };

        std::sort(device.servers.begin(), device.servers.end(), comparator);
    }
    #pragma endregion

    #pragma region MetricsCalculation

    /**
     * @brief Computes and populates intermediate metrics for a specific simulation step.
     * @param devices The current state of all devices.
     * @param servers The current state of all servers.
     * @param state The simulation result structure holding the metric history.
     */
    inline void calculateMetrics(const Devices& devices, const Servers& servers, Result& state) {
        auto& metrics = state.stepMetrics.back();
        auto& out = metrics->outputs;
        auto& timeStep = metrics->timeStep;

        out.devices_served_count = 0;
        out.devices_served_ec_count = 0;
        out.devices_served_cc_count = 0;
        out.servers_used_count = 0;
        out.servers_used_ec_count = 0;
        out.servers_used_cc_count = 0;
        out.cost_of_servers_used = 0.0;
        out.cost_of_non_service = 0.0;
        out.total_cost = 0.0;
        out.average_response_time = 0.0;       
        
        timeStep.served_devices_idx.clear();
        timeStep.non_served_devices_idx.clear();
        timeStep.servers_used_idx.clear();

        for (int d_idx : metrics->timeStep.covered_devices_idx) {
            const Device& device = devices.at(d_idx);
            if (device.id == 0) continue;

            if (device.served) {
                if (std::find(state.activeDevicesIdx.begin(), state.activeDevicesIdx.end(), device.id) == state.activeDevicesIdx.end()) {
                    state.activeDevicesIdx.push_back(device.id);
                }
                timeStep.served_devices_idx.push_back(device.id);
                out.devices_served_count++;
                out.average_response_time += device.server.responseTime;
                if (servers.at(device.server.id).type == 'E') {
                    out.devices_served_ec_count++;
                } else {
                    out.devices_served_cc_count++;
                }
            } else {
                timeStep.non_served_devices_idx.push_back(device.id);
                out.cost_of_non_service += device.cnd;
            }
        }

        for (const auto& server : servers) {
            if (server.id == 0) continue;
            if (server.on) {
                out.cost_of_servers_used += server.csc;
                if (server.type == 'E') {
                    out.servers_used_ec_count++;
                } else { // 'C'
                    out.servers_used_cc_count++;
                }
                timeStep.servers_used_idx.insert(server.id);
            }
        }

        out.servers_used_count = out.servers_used_ec_count + out.servers_used_cc_count;
        out.total_cost = out.cost_of_non_service + out.cost_of_servers_used + out.cost_of_non_coverage;
        state.serversUsedPercentage = out.servers_used_count/(servers.size() - 1);
    }

    /**
     * @brief Aggregates intermediate step metrics into the final comprehensive simulation metrics.
     * @param state The simulation result structure containing all step data.
     */
    inline void calculateMetrics(Result& state) {
        if (state.stepMetrics.empty()) return;

        state.finalMetrics = state.stepMetrics.front()->clone();
        auto& out = state.finalMetrics->outputs;
        auto& in = state.finalMetrics->inputs;
        auto& timeStep = state.finalMetrics->timeStep;     
        
        out.execution_time_sec = 0.0;
        out.devices_served_count = 0;
        out.devices_served_ec_count = 0;
        out.devices_served_cc_count = 0;
        out.servers_used_count = 0;
        out.servers_used_ec_count = 0;
        out.servers_used_cc_count = 0;
        out.cost_of_non_service = 0.0;
        out.cost_of_servers_used = 0.0;
        out.cost_of_non_coverage = 0.0;
        out.total_cost = 0.0;
        out.average_response_time = 0.0;        

        timeStep.time_step = state.stepMetrics.size() - 1;

        // Prepara a conversão caso seja um modelo Matemático (CPLEX)
        auto finalExact = std::dynamic_pointer_cast<ExactMetrics>(state.finalMetrics);
        if (finalExact){
            finalExact->OF = 0.0;
            finalExact->gap = -1.0;
        } 

        for (const auto& m : state.stepMetrics) {
            if (m->timeStep.time_step == 0) continue; 
            
            out.execution_time_sec += m->outputs.execution_time_sec; 
            out.devices_served_count += m->outputs.devices_served_count;
            out.devices_served_ec_count += m->outputs.devices_served_ec_count;
            out.devices_served_cc_count += m->outputs.devices_served_cc_count;
            out.cost_of_non_service += m->outputs.cost_of_non_service;
            out.cost_of_non_coverage += m->outputs.cost_of_non_coverage;
            out.cost_of_servers_used += m->outputs.cost_of_servers_used;
            out.average_response_time += m->outputs.average_response_time;

            if (finalExact) {
                if (auto stepExact = std::dynamic_pointer_cast<ExactMetrics>(m)) {
                    finalExact->OF += stepExact->OF;
                    if (stepExact->gap > finalExact->gap) {
                        finalExact->gap = stepExact->gap; 
                        finalExact->status = stepExact->status;
                    }
                }
            }
        }

        for (int i = 1; i < state.stepMetrics.size(); ++i) {
            auto& step_servers_used = state.stepMetrics[i]->timeStep.servers_used_idx;
            for (auto s_idx : step_servers_used) {
                timeStep.servers_used_idx.insert(s_idx);
            }
        }

        for (int s_idx : timeStep.servers_used_idx) {
            if (state.servers[s_idx].type == 'E') {
                out.servers_used_ec_count++;
            } else {
                out.servers_used_cc_count++;
            }
        }

        out.servers_used_count = timeStep.servers_used_idx.size();
        out.total_cost = out.cost_of_non_coverage + out.cost_of_non_service + out.cost_of_servers_used;
    }

    /**
     * @brief Saves the initial heuristic solution to the file system.
     * @param path_source A state used to extract file paths and simulation mode info.
     * @param value_source A state used to extract the actual metrics to be saved.
     */
    inline void saveInitialSolutionToFile(const Result& path_source, const Result& value_source) {
        if (path_source.stepMetrics.empty() || value_source.stepMetrics.empty()) return;

        const auto& p_metrics = path_source.stepMetrics.back();
        const auto& v_metrics = value_source.stepMetrics.back();

        std::filesystem::path dir_path = (p_metrics->simulation_mode == SimulationMode::ONLINE) ? p_metrics->getRunDirectoryPath() : p_metrics->getBaseDirectoryPath();
        std::string base_name = (p_metrics->simulation_mode == SimulationMode::ONLINE) ? p_metrics->getRunFileName() : p_metrics->getBaseFileName();

        std::filesystem::create_directories(dir_path);

        std::filesystem::path stats_path = dir_path / (base_name + "_heuristic.txt");
        std::ofstream stats_file(stats_path, std::ios::app);
        if (stats_file.is_open()) {
            if (stats_file.tellp() == 0) {
                const auto& headers = v_metrics->getStepHeader();
                for (size_t i = 0; i < headers.size(); ++i) {
                    stats_file << headers[i] << (i < headers.size() - 1 ? ";" : "");
                }
                stats_file << "\n";
            }
            const auto& row_data = v_metrics->data(false);
            for (size_t i = 0; i < row_data.size(); ++i) {
                stats_file << row_data[i] << (i < row_data.size() - 1 ? ";" : "");
            }
            stats_file << "\n";
        }

        std::filesystem::path vec_path = dir_path / (base_name + "_heuristic_vectors.txt");
        std::ofstream vec_file(vec_path, std::ios::app);
        if (vec_file.is_open()) {
            if (vec_file.tellp() == 0) vec_file << "TimeStep;DevicesIdx;CoveredDevicesIdx;NonCoveredDevicesIdx;ServedDevicesIdx;NonServedDevicesIdx;UsedServersIdx\n";
            vec_file << v_metrics->timeStep.toRowString() << "\n";
        }

        std::filesystem::path draw_path = dir_path / (base_name + "_heuristic_drawing.txt");
        std::ofstream draw_file(draw_path, std::ios::app);
        if (draw_file.is_open()) {
            // Polymorphic call, no cast needed
            showStructs::showMetricsConsoleOrFile(*v_metrics, draw_file, false);
        }
    }
    #pragma endregion

    /**
     * @brief Creates a resource bottleneck scenario by artificially modifying device demands and server capacities.
     * @param state The current simulation state to modify.
     */
    void resourceSaturation (Result& state) {
        Metrics& metrics = *std::dynamic_pointer_cast<Metrics>(state.stepMetrics[1]);
        
        iVec& devicesIds = metrics.timeStep.covered_devices_idx;
        int sizeEC = metrics.inputs.servers_ec;
        int sizeCC = metrics.inputs.servers_cc;
        for (int i = 0; i < sizeCC; i++) {
            int id = devicesIds[utils::randomNumber(0, (int) devicesIds.size() - 1)];
            Device& device = state.devices.at(id);
            device.cnd = 10 + sizeCC - i - 1;
            Server& server = state.servers.at(sizeEC + sizeCC - i);
            device.pcn = server.pcn;
            device.mem = server.mem - 0.001;
            device.sto = server.sto - 0.001;
            device.ttl = 1800;
        }
    }

} // namespace NetworkResourceAllocation

/**
 * @brief Short alias for the NetworkResourceAllocation namespace.
 */
namespace NRA = NetworkResourceAllocation;

#pragma once

#include "NetworkResourceAllocation.h"

/**
 * @brief Contains heuristic algorithms for network resource allocation.
 * @details This namespace provides fast, approximate methods such as random and greedy 
 * approaches to efficiently allocate devices to servers based on varying criteria.
 */
namespace Heuristics {

    namespace {

        /**
         * @brief Allocates devices to servers randomly.
         * @details Iterates through the covered devices and assigns each one to a randomly 
         * selected server from its list of potential servers, provided the server has enough capacity.
         * Tracks the execution time and updates the step metrics.
         * 
         * @param state The current simulation result state containing devices, servers, and metrics.
         */
        inline void randomHeuristic(Result& state) {
            Devices& devices = state.devices;
            Servers& servers = state.servers;
            HeuristicMetrics& metrics = *std::dynamic_pointer_cast<HeuristicMetrics>(state.stepMetrics.back());
            const iVec& demandIdx = metrics.timeStep.covered_devices_idx;

            auto startChrono = std::chrono::high_resolution_clock::now();
            
            if (!demandIdx.empty()) {
                for (const auto& d_idx : demandIdx) {
                    Device& device = devices.at(d_idx);
                    if (device.servers.empty()) continue;
                    
                    int s_idx = utils::randomNumber(0, (int)device.servers.size()-1);
                    
                    server_covering& potential_server = device.servers.at(s_idx);
                    Server& server = servers.at(potential_server.id);
                    
                    if (server.canServe(device)) {
                        server.addServed(device);
                        device.server = potential_server;
                    }
                }
            }

            auto endChrono = std::chrono::high_resolution_clock::now();
            metrics.outputs.execution_time_sec += std::chrono::duration<double>(endChrono - startChrono).count();
        }
        
        /**
         * @brief Allocates devices to servers using a greedy approach based on specific sorting criteria.
         * @details Sorts the covered devices and their potential servers based on the provided boolean flags, 
         * then iterates through the devices, assigning each to the first capable server in its sorted list.
         * Tracks the execution time and updates the step metrics.
         * 
         * @param state The current simulation result state containing devices, servers, and metrics.
         * @param sortDevicesAsc If true, sorts devices in ascending order based on their non-service cost (cnd); otherwise, descending.
         * @param sortServersAsc If true, sorts potential servers in ascending order based on their activation cost (csc); otherwise, descending.
         */
        inline void greedyHeuristic(Result& state, bool sortDevicesAsc, bool sortServersAsc) {
            Devices& devices = state.devices;
            Servers& servers = state.servers;
            HeuristicMetrics& metrics = *std::dynamic_pointer_cast<HeuristicMetrics>(state.stepMetrics.back());
            const iVec& demandIdx = metrics.timeStep.covered_devices_idx;

            auto startChrono = std::chrono::high_resolution_clock::now();
            
            if (!demandIdx.empty()) {
                iVec sortedCoveredIdx;
                if (sortDevicesAsc) {
                    sortedCoveredIdx = NRA::devicesAsc(devices, demandIdx);
                } else {
                    sortedCoveredIdx = NRA::devicesDesc(devices, demandIdx);
                }

                for (const auto& d_idx : sortedCoveredIdx) {
                    Device& device = devices.at(d_idx);
                    if (sortServersAsc) {
                        NRA::sortServersAsc(device, servers);
                    } else {
                        NRA::sortServersDesc(device, servers);
                    }

                    for (const auto& potential_server : device.servers) {
                        Server& server = servers.at(potential_server.id);
                        
                        if (server.canServe(device)) {
                            server.addServed(device);
                            device.server = potential_server;
                            break; 
                        }
                    }
                }
            }

            auto endChrono = std::chrono::high_resolution_clock::now();
            metrics.outputs.execution_time_sec += std::chrono::duration<double>(endChrono - startChrono).count();
        }
    }

    /**
     * @brief Bootstraps and executes the configured heuristic algorithm.
     * @details Reads the specified algorithm name from the current step's metrics and dispatches 
     * the execution to the corresponding heuristic function (Random or a specific Greedy variation).
     * 
     * @param state The unified simulation state to be processed.
     */
    inline void boot(Result& state) {
        if (state.stepMetrics.empty()) return;

        if (AlgorithmName::RANDOM == state.stepMetrics.back()->algorithm_name) {
            randomHeuristic(state);
        } else if (AlgorithmName::GREEDY_AscAsc == state.stepMetrics.back()->algorithm_name) {
            greedyHeuristic(state, true, true);
        } else if (AlgorithmName::GREEDY_AscDesc == state.stepMetrics.back()->algorithm_name) {
            greedyHeuristic(state, true, false);
        } else if (AlgorithmName::GREEDY_DescAsc == state.stepMetrics.back()->algorithm_name) {
            greedyHeuristic(state, false, true);
        } else if (AlgorithmName::GREEDY_DescDesc == state.stepMetrics.back()->algorithm_name) {
            greedyHeuristic(state, false, false);
        } else {
            std::cerr << "Error: Unknown heuristic algorithm type." << std::endl;
        }
    }
}

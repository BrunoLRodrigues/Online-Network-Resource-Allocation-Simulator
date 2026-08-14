#pragma once

#include "NetworkResourceAllocation.h"

/**
 * @brief Encapsulates meta-heuristic algorithms for network resource allocation.
 * @details Provides advanced optimization techniques, such as Simulated Annealing, 
 * to iteratively improve initial heuristic solutions by exploring neighborhood structures.
 */
namespace MetaHeuristics {

    namespace {

        /**
         * @brief Represents a state transition (move) within the neighborhood search.
         * @details Tracks the changes required to reallocate a device from one server to another, 
         * including cost differentials, allowing for quick evaluation and potential reversion.
         */
        struct Move {
            int deviceIdx = -1;      ///< @brief The index of the device being reassigned.
            int oldServerIdx = -1;   ///< @brief The ID of the server originally serving the device.
            int newServerIdx = -1;   ///< @brief The ID of the proposed new server for the device.
            bool served = false;     ///< @brief Indicates whether the device was already served prior to this move.
            double cns = 0.0;        ///< @brief The change in the Cost of Non-Service (delta CNS).
            double csu = 0.0;        ///< @brief The change in the Cost of Servers Used (delta CSU).
        };

        /**
         * @brief Attempts to find and apply a valid neighborhood move for a random device.
         * @details Selects a random device from the demand list and tries to reallocate it 
         * to a different capable server. Calculates the cost differentials for the move.
         * 
         * @param devices The global state vector of devices.
         * @param servers The global state vector of servers.
         * @param demandIdx The vector of device indices currently requesting service.
         * @param maxTries The maximum number of alternate servers to evaluate before giving up.
         * @return A Move struct detailing the applied transition. If no valid move is found, newServerIdx remains -1.
         */
        inline Move applyNeighbor(Devices& devices, Servers& servers, const iVec& demandIdx, const int maxTries) {
            Move move;
            move.deviceIdx = demandIdx.at(utils::randomNumber(0, (int)demandIdx.size() - 1));
            Device& device = devices.at(move.deviceIdx);
            
            iVec serversShuffled = utils::shuffledRange(0, (int) device.servers.size() - 1);
            
            int tries = 0;
            for (auto idxServers : serversShuffled) {
                if (tries++ >= maxTries) break;
                auto& potential_server = device.servers.at(idxServers);
                if (potential_server.id == device.server.id) continue;

                Server& new_server = servers.at(potential_server.id);
                if (new_server.canServe(device)) {
                    move.served = device.served;
                    move.newServerIdx = potential_server.id;
                    
                    if (!new_server.on) {
                        move.csu += new_server.csc;
                    }

                    if (device.served) {
                        move.oldServerIdx = device.server.id;
                        Server& old_server = servers.at(move.oldServerIdx);
                        old_server.rmvServed(device);
                        if (!old_server.on) move.csu -= old_server.csc;
                    } else move.cns -= device.cnd;

                    new_server.addServed(device);
                    device.server = potential_server;
                    return move;
                }
            }
            return move;
        }
    
        /**
         * @brief Reverts a previously applied neighborhood move.
         * @details Restores the device and server states to their exact configurations 
         * prior to the execution of the specified move.
         * 
         * @param devices The global state vector of devices.
         * @param servers The global state vector of servers.
         * @param move The Move structure containing the transition details to be undone.
         */
        inline void revertNeighbor(Devices& devices, Servers& servers, const Move& move) {
            if (move.newServerIdx == -1) return;

            Device& device = devices.at(move.deviceIdx);
            servers.at(move.newServerIdx).rmvServed(device);

            if (move.served) {
                servers.at(move.oldServerIdx).addServed(device);
                for (const auto& s : device.servers) {
                    if (s.id == move.oldServerIdx) {
                        device.server = s;
                        break;
                    }
                }
            } else device.served = false;
        }

        /**
         * @brief Executes the Simulated Annealing (SA) optimization algorithm.
         * @details Iteratively explores the solution space by applying random neighborhood moves. 
         * It accepts worse solutions with a probability dependent on the current temperature 
         * to escape local optima, cooling down progressively based on the alpha factor.
         * 
         * @param state The unified simulation state containing devices, servers, and metrics.
         */
        inline void simulatedAnnealing(Result& state) {
            MetaHeuristicMetrics& metrics = *std::dynamic_pointer_cast<MetaHeuristicMetrics>(state.stepMetrics.back());
            iVec demandIdx = metrics.timeStep.covered_devices_idx;

            auto startChrono = std::chrono::high_resolution_clock::now();
          
            if (!demandIdx.empty()) {

                Devices bestDevices = state.devices;
                Servers bestServers = state.servers;
                
                double bestCNS = metrics.outputs.cost_of_non_service, currentCNS = bestCNS;
                double bestCSU = metrics.outputs.cost_of_servers_used, currentCSU = bestCSU;
                double bestCost = bestCNS + bestCSU, currentCost = bestCost;
                
                double T = metrics.temperature;
                double alpha = metrics.alpha;

                while (T > 1e-3) {
                    for (int i = 0; i < 10; ++i) {                 
                        Move move = applyNeighbor(state.devices, state.servers, demandIdx, metrics.inputs.servers_cc);
                        if (move.newServerIdx == -1) continue;

                        double neighborCost = (currentCNS + move.cns) + (currentCSU + move.csu);
                        double delta = neighborCost - currentCost;

                        if (delta < 0 || (utils::randomNumber(0.0, 1.0) < std::exp(-delta / T))) {
                            currentCNS += move.cns;
                            currentCSU += move.csu;
                            currentCost = neighborCost;

                            if (currentCost < bestCost) {
                                if (state.serversUsedPercentage > 90.0) i = 0;
                                bestCNS = currentCNS;
                                bestCSU = currentCSU;
                                bestCost = currentCost;
                                bestDevices = state.devices;
                                bestServers = state.servers;
                            }
                        } else revertNeighbor(state.devices, state.servers, move);
                    }
                    T *= alpha;
                }
                
                state.devices = std::move(bestDevices);
                state.servers = std::move(bestServers);    
            }

            auto endChrono = std::chrono::high_resolution_clock::now();
            metrics.outputs.execution_time_sec += std::chrono::duration<double>(endChrono - startChrono).count();
        }   
    }

    /**
     * @brief Initializes and launches the selected meta-heuristic algorithm.
     * @details Generates an initial solution using a specified base heuristic, saves its state, 
     * and then applies the configured meta-heuristic (e.g., SA) to optimize the allocation further.
     * 
     * @param state The unified simulation state to be optimized.
     */
    inline void boot(Result& state) { 
        if (state.stepMetrics.empty()) return;
        MetaHeuristicMetrics& metrics = *std::dynamic_pointer_cast<MetaHeuristicMetrics>(state.stepMetrics.back());

        Result initialState = Result(state.devices, state.servers);
        initialState.stepMetrics.push_back(std::make_shared<HeuristicMetrics>(SimulationType::HEURISTIC, metrics.heuristic, metrics.simulation_mode, metrics.run_number));
        auto initialMetricsPtr = std::dynamic_pointer_cast<HeuristicMetrics>(initialState.stepMetrics.back());
        HeuristicMetrics& initialMetrics = *initialMetricsPtr;

        initialMetrics.inputs = metrics.inputs;
        initialMetrics.outputs = metrics.outputs;
        initialMetrics.timeStep = metrics.timeStep;

        Heuristics::boot(initialState);
        NRA::calculateMetrics(initialState.devices, initialState.servers, initialState);

        state.devices    = initialState.devices;
        state.servers    = initialState.servers;
        metrics.inputs   = initialMetrics.inputs;
        metrics.outputs  = initialMetrics.outputs;
        metrics.timeStep = initialMetrics.timeStep;

        NRA::saveInitialSolutionToFile(state, initialState);

        if (metrics.algorithm_name == AlgorithmName::SA) {
            simulatedAnnealing(state);
        } else {
            std::cerr << "Error: Unknown simulation or algorithm type." << std::endl;
            return;
        }
    }
}
//*/
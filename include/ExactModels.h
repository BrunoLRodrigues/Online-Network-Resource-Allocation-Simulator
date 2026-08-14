#pragma once

#include "NetworkResourceAllocation.h"

#include <ilcplex/ilocplex.h>

/**
 * @brief Type alias for a 2D array of CPLEX numerical variables.
 */
typedef IloArray<IloNumVarArray> NumVar2D;

/**
 * @brief Type alias for a 3D array of CPLEX numerical variables.
 */
typedef IloArray<NumVar2D>       NumVar3D;


/**
 * @brief Contains mathematical programming models for exact optimization.
 * @details This namespace utilizes the IBM ILOG CPLEX optimizer to solve the network 
 * resource allocation problem to optimality or within a specified time limit, providing 
 * an exact baseline for cost minimization.
 */
namespace ExactModels {
    
    namespace {

        /**
         * @brief Formulates and solves the exact Min-Cost network resource allocation model using CPLEX.
         * @details Sets up decision variables for server activation and device assignment. Defines the 
         * objective function to minimize total server activation costs and non-service penalties. 
         * Applies constraints to ensure devices are assigned to at most one server and that server 
         * resource capacities (bandwidth, memory, cores, processing, storage) are not exceeded. 
         * Parses the optimized variable values back into the simulation state.
         * 
         * @param state The unified simulation result state containing devices, servers, and metrics.
         * @param time_limit_sec The maximum execution time allowed for the CPLEX solver in seconds.
         */
        inline void minCost(Result& state, int time_limit_sec = 1200) {
            Devices& devices = state.devices;
            Servers& servers = state.servers;
            ExactMetrics& metrics = *std::dynamic_pointer_cast<ExactMetrics>(state.stepMetrics.back());
            iVec demandIdx = utils::mergeVectors(metrics.timeStep.covered_devices_idx, state.activeDevicesIdx);

            IloEnv env;
            try {
                IloModel model(env);
                
                #pragma region VariableDeclaration
                //=========================================================================
                // 1. VARIABLE DECLARATION
                //=========================================================================

                // w^{d}: 1 if device d is NOT served, 0 otherwise.
                IloNumVarArray w(env, devices.size(), 0, 1, ILOBOOL);
                for (size_t d_idx = 1; d_idx < devices.size(); ++d_idx) {
                    std::string name = "w_d(" + std::to_string(d_idx) + ")";
                    w[d_idx].setName(name.c_str());
                }

                // x_{i}^{d}: 1 if device d is allocated to server i, 0 otherwise.
                NumVar2D x(env, servers.size());
                for (size_t i = 1; i < servers.size(); ++i) {
                    x[i] = IloNumVarArray(env, devices.size(), 0, 1, ILOBOOL);
                    for (size_t d_idx = 1; d_idx < devices.size(); ++d_idx) {
                        std::string name = "x_s(" + std::to_string(i) + ")_d(" + std::to_string(d_idx) + ")";
                        x[i][d_idx].setName(name.c_str());
                    }
                }

                // z_i: 1 if server i is active, 0 otherwise.
                IloNumVarArray z(env, servers.size(), 0, 1, ILOBOOL);
                for (size_t i = 1; i < servers.size(); ++i) {
                    std::string name = "z_s(" + std::to_string(i) + ")";
                    z[i].setName(name.c_str());
                }
                #pragma endregion
                
                #pragma region ObjectiveFunction
                //=========================================================================
                // 2. OBJECTIVE FUNCTION
                // Minimize total cost: server activation costs + non-service penalties.
                //=========================================================================

                IloExpr obj(env);
                for (size_t i = 1; i < servers.size(); ++i) {
                    obj += servers[i].csc * z[i];
                }
                for (int d_idx : demandIdx) {
                    obj += devices[d_idx].cnd * w[d_idx];
                }
                model.add(IloMinimize(env, obj));
                obj.end();
                #pragma endregion

                #pragma region Constraints
                //=========================================================================
                // 3. CONSTRAINTS
                //=========================================================================

                for (int d_idx : demandIdx) {
                    Device& device = devices[d_idx];
                    // Constraint (1): Each device is served by at most one server.
                    IloExpr c1(env);
                    for (const auto& s_info : device.servers) {
                        c1 += x[s_info.id][d_idx];
                    }
                    model.add(c1 == 1 - w[d_idx]);
                    c1.end();
                }

                // Constraints (2-6): Server resource capacity limits.
                for (size_t i = 1; i < servers.size(); ++i) {
                    IloExpr c2_bw(env), c3_mem(env), c4_pcn(env), c5_pcc(env), c6_sto(env);
                    
                    for (int d_idx : demandIdx) {
                        // Check if server 'i' is a potential server for device 'd_idx'
                        bool is_potential = false;
                        for (const auto& s_info : devices[d_idx].servers) {
                            if (s_info.id == i) {
                                is_potential = true;
                                break;
                            }
                        }
                        if (is_potential) {
                            c2_bw  += devices[d_idx].bw  * x[i][d_idx]; // Bandwidth
                            c3_mem += devices[d_idx].mem * x[i][d_idx]; // Memory
                            c4_pcn += devices[d_idx].pcn * x[i][d_idx]; // Num. Cores
                            c5_pcc += devices[d_idx].pcc * x[i][d_idx]; // Proc. Capacity
                            c6_sto += devices[d_idx].sto * x[i][d_idx]; // Storage
                        }
                    }
                    model.add(c2_bw  <= z[i] * (servers[i].bw        /*- servers[i].supply.bwD*/)); 
                    model.add(c3_mem <= z[i] * (servers[i].mem       /*- servers[i].supply.memD*/)); 
                    model.add(c4_pcn <= z[i] * (servers[i].pcn       /*- servers[i].supply.pcnD*/)); 
                    model.add(c5_pcc <= z[i] * (servers[i].pcc_total /*- servers[i].supply.pccD*/)); 
                    model.add(c6_sto <= z[i] * (servers[i].sto       /*- servers[i].supply.stoD*/)); 
                    c2_bw.end(); c3_mem.end(); c4_pcn.end(); c5_pcc.end(); c6_sto.end();
                }

                // Constraint 7: Forces previously active devices to remain served.
                if (!state.activeDevicesIdx.empty()) {
                    for (int d_idx : state.activeDevicesIdx) {
                        if (devices[d_idx].served) {
                            servers[devices[d_idx].server.id].rmvServed(devices[d_idx]);
                        }                    
                        w[d_idx].setBounds(0, 0); // w^d = 0
                    }
                }
                
                #pragma endregion

                #pragma region SolverConfiguration
                //=========================================================================
                // 4. SOLVER CONFIGURATION AND EXECUTION
                //=========================================================================

                IloCplex cplex(model);
                cplex.setParam(IloCplex::Param::Threads, 1);
                cplex.setParam(IloCplex::Param::TimeLimit, time_limit_sec);
                //cplex.setParam(IloCplex::Param::MIP::Tolerances::Integrality, 1e-9);

                std::filesystem::path runDir = metrics.getRunDirectoryPath();
                std::filesystem::path logDir = runDir / "Logs";
                std::filesystem::path modelDir = runDir / "Models";
                std::filesystem::create_directories(logDir);
                std::filesystem::create_directories(modelDir);
                std::string stepFileName = "Step_" + std::to_string(metrics.timeStep.time_step);
                std::filesystem::path logPath = logDir / (stepFileName + ".log");
                std::filesystem::path modelPath = modelDir / (stepFileName + ".lp");

                std::ofstream logFile(logPath);
                if (logFile.is_open()) {
                    cplex.setOut(logFile);
                } else {
                    cplex.setOut(env.getNullStream());
                }

                cplex.exportModel(modelPath.c_str());

                auto startChrono = std::chrono::high_resolution_clock::now();
                cplex.solve();
                auto endChrono = std::chrono::high_resolution_clock::now();
                metrics.outputs.execution_time_sec = std::chrono::duration<double>(endChrono - startChrono).count();

                std::stringstream status;
                status << cplex.getStatus();
                metrics.status = status.str();
                metrics.OF = (double)cplex.getObjValue() + metrics.outputs.cost_of_non_coverage;
                metrics.gap = cplex.getMIPRelativeGap();

                logFile.close();
                #pragma endregion

                #pragma region Result
                //=========================================================================
                // 5. PARSE RESULTS
                //=========================================================================

                for (int d_idx : demandIdx) {
                    if (cplex.getValue(w[d_idx]) < 0.5) {
                        for (const auto& s_info : devices[d_idx].servers) {
                            if (cplex.getValue(x[s_info.id][d_idx]) > 0.5) {
                                servers[s_info.id].addServed(devices[d_idx]);
                                devices[d_idx].server = s_info;
                                break; // Move to the next device
                            }
                        }
                    } 
                }
                #pragma endregion

            } catch (const IloException& e) {
                std::cerr << "CPLEX Error: " << e.getMessage() << std::endl;
            } catch (...) {
                std::cerr << "An unknown error occurred in the CPLEX model." << std::endl;
            }
            env.end();
        }
    }

    /**
     * @brief Bootstraps and executes the configured exact mathematical model.
     * @details Reads the specified algorithm name from the current step's metrics and dispatches 
     * the execution to the corresponding exact solver (e.g., MinCost).
     * 
     * @param state The unified simulation state to be processed.
     * @param time_limit_sec The maximum execution time allowed for the exact solver in seconds.
     */
    inline void boot(Result& state, int time_limit_sec = 1200) {
        if (state.stepMetrics.empty()) return;

        if (AlgorithmName::MIN_COST == state.stepMetrics.back()->algorithm_name) {
            minCost(state, time_limit_sec);
        } else {
            std::cerr << "Error: Unknown mathematical model algorithm type." << std::endl;
        }
    }
}
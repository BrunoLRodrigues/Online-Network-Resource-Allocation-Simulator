#include "NetworkResourceAllocation.h"
#include "Heuristics.h"
#include "MetaHeuristics.h"
#include "ExactModels.h"

/**
 * @name Simulation Parameters
 * @brief Global configuration variables governing the simulation environment.
 * @{
 */
int MAX_TTL = 120;                            ///< Maximum Time-To-Live for active devices in the network.
int TECHNOLOGY = 4;                           ///< Cellular technology generation indicator (e.g., 4 = 4G).
int NUM_SERVERS_EC = 100;                     ///< Total number of Edge Computing (EC) servers.
int NUM_SERVERS_CC = 5;                       ///< Total number of Cloud Computing (CC) servers.
int RUN_SIMULATIONS = 35;                     ///< Total number of stochastic simulation runs to execute.
int POISSON_LAMBDA = 20;                      ///< Mean arrival rate (lambda) for the Poisson distribution.
int CPLEX_TIME_LIMIT_SEC = 2;                 ///< Maximum execution time allowed for the exact mathematical solver.

double TEMPERATURE = 10.0;                    ///< Initial temperature parameter for the Simulated Annealing meta-heuristic.
double ALPHA = 0.99;                          ///< Cooling rate (alpha) for the Simulated Annealing meta-heuristic.

SimulationMode SIMULATION_MODE = SimulationMode::ONLINE; ///< Operational mode determining dynamic or static arrivals.
/** @} */

/**
 * @brief Executes a specific simulation scenario across all time steps.
 * @details Iterates through the baseline state's time steps, applying the selected allocation algorithm 
 * (Exact, Heuristic, or Meta-Heuristic). Manages the active devices' TTL and computes step-by-step 
 * and final evaluation metrics.
 * 
 * @param baseState A pre-calculated baseline result state containing initialized devices, servers, and coverage.
 * @param scenario An integer mapping to a specific algorithm configuration (0: Exact, 1: Greedy, 2: Random, 3: SA+Greedy, 4: SA+Random).
 * @param run_number The identifier for the current execution iteration, used for tracking and file output.
 * @throws std::invalid_argument If an undefined scenario number or simulation type is encountered.
 */
inline void runSimulation(const std::optional<Result>& baseState, int scenario, int run_number) {

    SimulationType simulationType;
    AlgorithmName algorithm, initialSolution; 
    
    if (scenario == 0) {
        simulationType = SimulationType::EXACT;
        algorithm = AlgorithmName::MIN_COST;
    } else if (scenario == 1) {
        simulationType = SimulationType::HEURISTIC;
        algorithm = AlgorithmName::GREEDY_DescAsc;
    } else if (scenario == 2) {
        simulationType = SimulationType::HEURISTIC;
        algorithm = AlgorithmName::RANDOM;
    } else if (scenario == 3) {
        simulationType = SimulationType::METAHEURISTIC;
        algorithm = AlgorithmName::SA;
        initialSolution = AlgorithmName::GREEDY_DescAsc;
    } else if (scenario == 4) {
        simulationType = SimulationType::METAHEURISTIC;
        algorithm = AlgorithmName::SA;
        initialSolution = AlgorithmName::RANDOM;
    } else {
        throw std::invalid_argument("Invalid scenario number");
    }

    // 1. Initialize the single working state
    Result state = *baseState;
    state.stepMetrics.clear();

    // 3. Run the time steps
    for (int ts = 0; ts < baseState->stepMetrics.size(); ++ts) {
        std::shared_ptr<Metrics> currentMetrics;
        if (simulationType == SimulationType::EXACT) {
            currentMetrics = std::make_shared<ExactMetrics>(simulationType, algorithm, SIMULATION_MODE, run_number);
        } else if (simulationType == SimulationType::HEURISTIC) {
            currentMetrics = std::make_shared<HeuristicMetrics>(simulationType, algorithm, SIMULATION_MODE, run_number);
        } else if (simulationType == SimulationType::METAHEURISTIC) {
            currentMetrics = std::make_shared<MetaHeuristicMetrics>(simulationType, algorithm, SIMULATION_MODE, run_number, TEMPERATURE, ALPHA, initialSolution);
        } else {
            throw std::invalid_argument("Invalid simulation type");
        }
        currentMetrics->inputs = baseState->stepMetrics[ts]->inputs;
        currentMetrics->outputs = baseState->stepMetrics[ts]->outputs;
        currentMetrics->timeStep = baseState->stepMetrics[ts]->timeStep;
        
        state.stepMetrics.push_back(currentMetrics->clone());
        
        if (ts > 1) {
            NRA::manageTTL(state); 
        }
        if (ts >= 1) {
            if (SimulationType::EXACT == simulationType) {
                ExactModels::boot(state, CPLEX_TIME_LIMIT_SEC); 
            } else if (SimulationType::HEURISTIC == simulationType) {
                Heuristics::boot(state);
            } else if (SimulationType::METAHEURISTIC == simulationType) {
                MetaHeuristics::boot(state);
            } else {
                std::cerr << "Error: Unknown simulation type." << std::endl;
            }
        }
        NRA::calculateMetrics(state.devices, state.servers, state);
        showStructs::showMetrics(state.stepMetrics.back(), false);
        state.stepMetrics.back()->saveResultStepToFile();
    }
    
    NRA::calculateMetrics(state);
    state.finalMetrics->saveResultFinalToFile();
    state.saveVectorsToFile();
    showStructs::showMetrics(state.finalMetrics, true);
}

/**
 * @brief Initializes the environment and orchestrates the execution of all designated scenarios.
 * @details Computes the required pre-calculation baseline and systematically triggers the exact model, 
 * baseline heuristics, and stochastic meta-heuristics over the configured number of iterations.
 * 
 * @param numDevices_or_numSteps The target amount of devices (offline) or the total number of simulated steps (online).
 * @param isDeviceSize Boolean flag determining if the first parameter represents a device count (true) or a step count (false).
 */
inline void startSimulation(int numDevices_or_numSteps, bool isDeviceSize) {
    auto resultBase = NRA::pre_calculation(numDevices_or_numSteps, NUM_SERVERS_EC, NUM_SERVERS_CC, TECHNOLOGY, SIMULATION_MODE, MAX_TTL, POISSON_LAMBDA, isDeviceSize);
    if (!resultBase) return;

    //NRA::resourceSaturation(resultBase.value());
    
    runSimulation(resultBase, 0, 1);
    runSimulation(resultBase, 1, 1);
    for (int i = 1; i <= RUN_SIMULATIONS; i++) {
        runSimulation(resultBase, 2, i);
        runSimulation(resultBase, 3, i);
        runSimulation(resultBase, 4, i);
    }//*/
}

/**
 * @brief Application entry point.
 * @details Loops through predefined workload sizes and starts the simulation pipeline for each configuration.
 * @return Execution status code (0 for success).
 */
int main() {    
    for (int d : {1000, 2000, 3000, 4000, 5000}) {
        startSimulation(d, false);
    }
    return 0;
}
<div align="right">
Read this in other languages: <a href="README-pt-br.md">Português (BR)</a> 🇧🇷
</div>

# Online Network Resource Allocation Simulator

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

A C++ simulation framework to optimize resource allocation in a real-time network of devices, edge servers (EC), and cloud servers (CC). This project evaluates different strategies, from mathematical models to meta-heuristics, to minimize operational costs while ensuring Quality of Service (QoS).

## Key Features

-   **Data Generation:** Dynamically generates datasets for devices and servers (Edge and Cloud).
-   **Pre-calculation Phase:** Performs network analysis, including device coverage, latency, and response time calculations.
-   **Multiple Optimization Approaches:**
    -   **Mathematical Model:** Implements an Integer Linear Programming (ILP) model using IBM ILOG CPLEX to attempt to find the optimal solution within a time limit.
    -   **Heuristics:** Includes fast allocation algorithms like Random and Greedy variations.
    -   **Meta-heuristics:** Utilizes *Simulated Annealing* (SA) to attempt to find better solutions in a reasonable time.
-   **Detailed Metrics:** Collects and saves comprehensive metrics for each simulation run, allowing for detailed performance analysis.

## Project Structure

```
.
├── include/              # Header files (.h)
├── src/                  # Source code files (.cpp)
├── data/                 # Base data files for generation
├── Analysis/             # Analysis files (e.g., spreadsheets with charts)
├── Results/              # Output directory for results 
├── build/                # Build directory (ignored by git)
├── .gitignore            # Git ignore file
└── README.md             # This file
```

## Prerequisites

Before you begin, ensure you have met the following requirements:

* **C++17 Compiler:** A modern C++ compiler (like GCC or Clang) with C++17 standard support.
* **CMake:** Version 3.10 or higher is recommended to build the project.
* **IBM ILOG CPLEX:** This project depends on CPLEX optimization libraries. You must have CPLEX installed on your system.
    * Ensure that the CPLEX environment variables (`CPLEX_DIR`, etc.) are configured correctly, or that the installer integrated it into your system's path.

## How to Compile and Run

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/BrunoLRodrigues/Online-Network-Resource-Allocation-Simulator.git
    cd Online-Network-Resource-Allocation-Simulator
    ```

2.  **Configure the project with CMake:**
    ```bash
    cmake -S . -B build
    ```
    *If CMake does not find CPLEX automatically, you might need to provide the installation path.*

3.  **Build the project:**
    ```bash
    cmake --build build
    ```

4.  **Run the simulation:**
    The `main.cpp` file is configured to run a default set of simulations. To execute them, run:
    ```bash
    ./build/main_app
    ```

## How It Works

The simulation follows a simple and multi-phase process:

1.  **Data Loading & Generation:** The program first loads or generates the necessary data for devices and servers.
2.  **Pre-calculation:** Next, it determines which devices are within the coverage area of edge servers (EC) and pre-calculates essential metrics like connection and processing times for all potential device-server pairs.
3.  **Execution:** The simulation state is passed to one of the selected algorithms:
    * **Mathematical:** Solves the problem seeking optimality using CPLEX within the time limit.
    * **Heuristic:** Applies a fast, rule-based method to find a solution quickly.
    * **Meta-heuristic:** Starts with a solution from a heuristic and iteratively attempts to improve it.
4.  **Results:** After each execution, a `Result` object is populated, displayed on the console, and saved to a `.txt` file in the `Results/` directory.

## Implemented Algorithms

-   **Mathematical Model:**
    -   `Minimize_Cost`: An ILP model that minimizes total operational costs.
-   **Heuristics:**
    -   `Random`: Allocates devices to available servers randomly.
    -   `Greedy`: Allocates devices based on sorted lists of devices (by penalty) and servers (by activation cost). Variations include `Greedy_AscAsc`, `Greedy_AscDesc`, `Greedy_DescAsc`, and `Greedy_DescDesc`.
-   **Meta-heuristic:**
    -   `SA` (Simulated Annealing): A probabilistic method to attempt to improve initial solutions.

## Selected Parameters

Since this is an *online* simulation (real-time resource allocation), IoT device demands arrive in discretized time steps. At each time step, the implemented algorithms operate statically (similar to an *offline* simulation) to find a solution. The dynamic aspect of the system lies in the demand packets, which continuously enter and leave the network at each time step.

---

### 1. Communication Technologies

The simulator supports mobile technologies from 1G to 6G. However, for realistic experimentation purposes, using 2G to 5G is recommended. **The default technology used in this experiment was 4G.**

The bandwidth ($bw^d$) allocated for each demand depends on the transmission rate of the selected technology:

| Technology | Coverage Radius (km) | Transfer Rate (Mbps) |
| :--- | :---: | :---: |
| **1G** | 20.00 | 0.0024 |
| **2G** | 10.00 | 0.0640 |
| **3G** | 5.00 | 2.0000 |
| **4G** | 3.00 | 100.0000 |
| **5G** | 0.60 | 1000.0000 |
| **6G** | 0.32 | 10000.0000 |

---

### 2. Demand Profile (IoT Devices)

Demand profiles are generated from a random draw among 10 base services. The characteristics of each request are built as follows:

* **Tasks:** 1 to 4 integer tasks.
* **Cores ($nc^d$):** [1, 4] cores, defining the parallelism requirement.
* **Data Size ($s^d$):** 0.00484 Mb or 12.0 Mb.
* **Processing ($p^d$):** Drawn from the interval [0.000001, 2.5] GHz (multiplied by the number of tasks).
* **Memory ($m^d$):** Drawn from the interval [0.000001, 2.5] GB (multiplied by the number of tasks).
* **Storage (${st}^d$):** Drawn from the interval [0.000001, 15.0] GB (multiplied by the number of tasks).
* **Time-To-Live ($tl^d$):** The demand's permanence in the network is drawn from the interval [1, 30] multiplied by the number of tasks, resulting in a minimum time of 1 time step and a maximum of 120 time steps.

#### Penalty Costs (Non-Service) $c^d$
The penalty cost for not serving a device ($cnd$) is calculated based on the number of cores (${nc}^d$) and the required processing capacity ($p^d$):

| ${nc}^d$ | Base Cost | $p^d$ (GHz) | Additional Cost |
| :---: | :---: | :--- | :---: |
| 1 | 3.0 | [0, 2.5) | 0.3 |
| 2 | 5.0 | [2.5, 5.0) | 0.5 |
| 3 | 7.0 | [5.0, 7.5) | 0.7 |
| 4 | 9.0 | [7.5, 10.0] | 0.9 |

> **Note:** The total penalty cost of a demand $(c^d)$ is the sum of the Base Cost and the Additional Cost associated with its processing.

---

### 3. Server Infrastructure

The infrastructure is divided between Edge Computing (EC) and Cloud Computing (CC) servers. 
The backbone network bandwidth (wired servers side) has a fixed value of **100,000 Mbps**.

#### Edge Computing (EC)
Edge servers are created from a uniform draw among 5 base configurations of processing and cost:

| Cores (${NC}_i$) | Processing ($P_i$) | Activation Cost ($C_i$) |
| :---: | :---: | :---: |
| 2 | 1.6 GHz | € 0.00085 |
| 4 | 2.3 GHz | € 0.00097 |
| 6 | 2.9 GHz | € 0.00121 |
| 8 | 3.0 GHz | € 0.00138 |
| 10 | 3.0 GHz | € 0.00153 |

The other EC specifications are drawn dynamically:
* **Memory:** [2.5, 125.0] GB.
* **Storage:** [15.0, 1000.0] GB.
* **Base Processing Time (${TP}_i$):** Calculated by $TP_i = 12.5/P_i$ ms.

#### Cloud Computing (CC)
Cloud datacenters have pre-defined high-performance configurations:

| Activation Cost ($C_i$) | Memory ($M_i$) | Processing ($P_i$) | Cores (${NC}_i$) | Storage ($ST_i$) | Processing Time ($TP_i$) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| € 0.05818 | 6000 GB | 2.3 GHz | 96 | 100000 GB | 5.681818 ms |
| € 0.04279 | 4000 GB | 2.3 GHz | 64 | 25000 GB | 5.681818 ms |
| € 0.03210 | 3000 GB | 2.3 GHz | 48 | 20000 GB | 5.681818 ms |
| € 0.02140 | 2000 GB | 2.3 GHz | 32 | 12000 GB | 5.681818 ms |
| € 0.01070 | 1000 GB | 2.3 GHz | 16 | 10000 GB | 5.681818 ms |

---

### 4. Physical Constants and Network Formulas

The calculation of latencies and routes takes into account the following physical parameters and mathematical equations:

* **Earth's Radius:** 6371.0088 km
* **Pi ($\pi$):** 3.141592653589793
* **Speed of Light ($c$):** 299792.458 km/s
* **Inter-Datacenter Latency (EC $\leftrightarrow$ CC):** 111.86 ms

The equations determine the total response time for each request:

1. **Processing Time:**
   $$T_{proc} = s^d \cdot TP_i$$
2. **Transmission Time:**
   $$T_{tran} = \left( \frac{s^d}{bw^d} \right) \cdot 1000$$
3. **Propagation Time (where $dist$ is the distance in km from the device to the covering EC):**
   $$T_{prop} = \left( \frac{dist}{c} \right) \cdot 1000$$
4. **Connection Time ($T_{cone}$):**
   * If served at the **Edge:** $T_{cone} = T_{tran} + T_{prop}$
   * If served in the **Cloud:** $T_{cone} = T_{tran} + T_{prop} + 111.86$
5. **Final Response Time:**
   $$T_{resp} = (2 \cdot T_{cone}) + T_{proc}$$

---

### 5. Global Experiment Parameters

For the testing battery and result collection, the simulation was fixed with the following control variables:

* **Server Topology:** 100 Edge Servers (EC) and 5 Cloud Servers (CC).
* **Repetitions:** 35 executions for stochastic algorithms (Random Heuristics and SA).
* **Arrival Distribution:** Poisson process with $\lambda = 20$.
* **Exact Model (CPLEX):** Maximum execution time limit of 2 seconds per time step.
* **Simulated Annealing (SA):** Initial temperature $T_0 = 10.0$ and cooling factor $\alpha = 0.99$.
* **Evaluated Scenarios:** Execution volumes varying between 1000, 2000, 3000, 4000, and 5000 time steps.
<div align="right">
Leia isto em outros idiomas: <a href="README.md">English</a> 🇬🇧🇺🇸
</div>

# Simulador de Alocação de Recursos em Redes Online

![Linguagem](https://img.shields.io/badge/Linguagem-C%2B%2B-blue.svg)
![Padrão](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

Uma estrutura de simulação em C++ para otimizar a alocação de recursos em uma rede de dispositivos em tempo real, servidores de borda (EC) e servidores em nuvem (CC). Este projeto avalia diferentes estratégias, desde modelos matemáticos a meta-heurísticas, para minimizar os custos operacionais enquanto garante a qualidade do serviço (QoS).

## Principais Funcionalidades

-   **Geração de Dados:** Gera dinamicamente conjuntos de dados para dispositivos e servidores (Edge e Cloud).
-   **Fase de Pré-cálculo:** Realiza análises de rede, incluindo cobertura de dispositivos, latência e cálculos de tempo de resposta.
-   **Múltiplas Abordagens de Otimização:**
    -   **Modelo Matemático:** Implementa um modelo de Programação Linear Inteira (ILP) usando o IBM ILOG CPLEX para tentar encontrar a solução ótima dentro do tempo limite.
    -   **Heurísticas:** Inclui algoritmos rápidos de alocação como Aleatório (Random) e variações do Guloso (Greedy).
    -   **Meta-heurísticas:** Utiliza *Simulated Annealing* (SA) para tentar encontrar soluções melhores em tempo razoável.
-   **Métricas Detalhadas:** Coleta e salva métricas abrangentes para cada execução da simulação, permitindo uma análise de desempenho detalhada.

## Estrutura do Projeto

```
.
├── include/              # Arquivos de cabeçalho (.h)
├── src/                  # Arquivos de código-fonte (.cpp)
├── data/                 # Arquivos de dados base para geração
├── Analysis/             # Arquivos de análise (ex: planilhas com gráficos)
├── Results/              # Diretório de saída para os resultados 
├── build/                # Diretório de compilação (ignorado pelo git)
├── .gitignore            # Arquivo do Git para ignorar arquivos
└── README.md             # Este arquivo
```

## Pré-requisitos

Antes de começar, garanta que você possui os seguintes requisitos:

* **Compilador C++17:** Um compilador C++ moderno (como GCC ou Clang) com suporte ao padrão C++17.
* **CMake:** Versão 3.10 ou superior é recomendada para compilar o projeto.
* **IBM ILOG CPLEX:** Este projeto depende das bibliotecas de otimização do CPLEX. Você precisa ter o CPLEX instalado no seu sistema.
    * Garanta que as variáveis de ambiente do CPLEX (`CPLEX_DIR`, etc.) estão configuradas corretamente, ou que o instalador o integrou ao path do seu sistema.

## Como Compilar e Executar

1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/BrunoLRodrigues/Online-Network-Resource-Allocation-Simulator.git
    cd Online-Network-Resource-Allocation-Simulator
    ```

2.  **Configure o projeto com o CMake:**
    ```bash
    cmake -S . -B build
    ```
    *Se o CMake não encontrar o CPLEX automaticamente, você talvez precise fornecer o caminho para a instalação.*

3.  **Compile o projeto:**
    ```bash
    cmake --build build
    ```

4.  **Execute a simulação:**
    O arquivo `main.cpp` está configurado para rodar um conjunto padrão de simulações. Para executá-las, rode:
    ```bash
    ./build/main_app
    ```

## Como Funciona

A simulação segue um processo simples e multifásico:

1.  **Carregamento e Geração de Dados:** O programa primeiro carrega ou gera os dados necessários para dispositivos e servidores.
2.  **Pré-cálculo:** Em seguida, determina quais dispositivos estão dentro da área de cobertura dos servidores de borda (EC) e pré-calcula métricas essenciais como tempos de conexão e processamento para todos os pares potenciais de dispositivo-servidor.
3.  **Execução:** O estado da simulação é passado para um dos algoritmos selecionados:
    * **Matemático:** Resolve o problema buscando a otimalidade usando o CPLEX dentro do tempo limite.
    * **Heurístico:** Aplica um método rápido e baseado em regras para encontrar uma solução rapidamente.
    * **Meta-heurístico:** Começa com uma solução de uma heurística e a tenta melhorar iterativamente.
4.  **Resultados:** Após cada execução, um objeto `Result` é preenchido, exibido no console e salvo em um arquivo `.txt` no diretório `Results/`.

## Algoritmos Implementados

-   **Modelo Matemático:**
    -   `Minimize_Cost`: Um modelo ILP, baseado no modelo matemático, que minimiza os custos operacionais totais.
-   **Heurísticas:**
    -   `Random`: Aloca dispositivos a servidores disponíveis de forma aleatória.
    -   `Greedy`: Aloca dispositivos com base em listas ordenadas de dispositivos (por penalidade) e servidores (por custo de ativação). Variações incluem `Greedy_AscAsc`, `Greedy_AscDesc`, `Greedy_DescAsc` e `Greedy_DescDesc`.
-   **Meta-heurística:**
    -   `SA` (Simulated Annealing): Um método probabilístico para tentar melhorar as soluções iniciais.

## Parâmetros Selecionados

Por se tratar de uma simulação *online* (alocação de recursos em tempo real), as demandas dos dispositivos IoT chegam em passos de tempo (*time steps*) discretizados. Em cada *time step*, os algoritmos implementados operam de maneira estática (semelhante a uma simulação *offline*) para encontrar uma solução. O aspecto dinâmico do sistema reside nos pacotes de demandas, que entram e saem da rede continuamente a cada passo de tempo.

---

### 1. Tecnologias de Comunicação

O simulador suporta tecnologias móveis do 1G ao 6G. No entanto, para fins de experimentação realista, recomenda-se o uso de 2G a 5G. **A tecnologia padrão utilizada neste experimento foi o 4G.**

A largura de banda ($bw^d$) alocada para cada demanda depende da taxa de transmissão da tecnologia selecionada:

| Tecnologia | Raio de Cobertura (km) | Taxa de Transferência (Mbps) |
| :--- | :---: | :---: |
| **1G** | 20.00 | 0.0024 |
| **2G** | 10.00 | 0.0640 |
| **3G** | 5.00 | 2.0000 |
| **4G** | 3.00 | 100.0000 |
| **5G** | 0.60 | 1000.0000 |
| **6G** | 0.32 | 10000.0000 |

---

### 2. Perfil das Demandas (Dispositivos IoT)

Os perfis de demanda são gerados a partir de um sorteio entre 10 serviços base. As características de cada requisição são construídas da seguinte forma:

* **Tarefas (tasks):** 1 a 4 tarefas inteiras.
* **Cores $(nc^d)$:** [1, 4] cores, definindo a exigência de paralelismo.
* **Tamanho dos Dados ($s^d$):** 0.00484 Mb ou 12.0 Mb.
* **Processamento $(p^d)$:** Sorteado no intervalo [0.000001, 2.5] GHz (multiplicado pela quantidade de tarefas).
* **Memória $(m^d)$:** Sorteada no intervalo [0.000001, 2.5] GB (multiplicada pela quantidade de tarefas).
* **Armazenamento $({st}^d)$:** Sorteado no intervalo [0.000001, 15.0] GB (multiplicado pela quantidade de tarefas).
* **Tempo de Vida $(tl^d)$:** A permanência da demanda na rede é sorteada no intervalo [1, 30] multiplicada pela quantidade de tarefas, resultando em um tempo mínimo de 1 *time step* e máximo de 120 *time steps*.

#### Custos de Penalidade (Não-Atendimento) $c^d$
O custo de penalidade por não atender a um dispositivo ($cnd$) é calculado com base na quantidade de cores (${nc}^d$) e na capacidade de processamento ($p^d$) exigida:

| ${nc}^d$ | Custo Base | $p^d$ (GHz) | Custo Adicional |
| :---: | :---: | :--- | :---: |
| 1 | 3.0 | [0, 2.5) | 0.3 |
| 2 | 5.0 | [2.5, 5.0) | 0.5 |
| 3 | 7.0 | [5.0, 7.5) | 0.7 |
| 4 | 9.0 | [7.5, 10.0] | 0.9 |

> **Nota:** O custo total de penalidade de uma demanda $(c^d)$ é a soma do Custo Base com o Custo Adicional associado ao seu processamento.

---

### 3. Infraestrutura de Servidores

A infraestrutura é dividida entre servidores de Borda (Edge Computing, EC) e de Nuvem (Cloud Computing, CC). 
A largura de banda da rede de *backbone* (lado dos servidores cabeados) possui um valor fixo de **100.000 Mbps**.

#### Edge Computing (EC)
Os servidores de borda são criados a partir de um sorteio uniforme entre 5 configurações base de processamento e custo:

| Cores (${NC}_i$) | Processamento ($P_i$) | Custo Ativação ($C_i$) |
| :---: | :---: | :---: |
| 2 | 1.6 GHz | € 0.00085 |
| 4 | 2.3 GHz | € 0.00097 |
| 6 | 2.9 GHz | € 0.00121 |
| 8 | 3.0 GHz | € 0.00138 |
| 10 | 3.0 GHz | € 0.00153 |

As demais especificações dos ECs são sorteadas dinamicamente:
* **Memória:** [2.5, 125.0] GB.
* **Armazenamento:** [15.0, 1000.0] GB.
* **Tempo Base de Processamento (${TP}_i$):** Calculado por $TP_i = 12.5/P_i$ ms.

#### Cloud Computing (CC)
Os datacenters em nuvem possuem configurações de alto desempenho pré-definidas:

| Custo Ativação ($C_i$) | Memória ($M_i$) | Processamento ($P_i$) | Cores (${NC}_i$) | Armazenamento ($ST_i$) | Tempo Processamento ($TP_i$) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| € 0.05818 | 6000 GB | 2.3 GHz | 96 | 100000 GB | 5.681818 ms |
| € 0.04279 | 4000 GB | 2.3 GHz | 64 | 25000 GB | 5.681818 ms |
| € 0.03210 | 3000 GB | 2.3 GHz | 48 | 20000 GB | 5.681818 ms |
| € 0.02140 | 2000 GB | 2.3 GHz | 32 | 12000 GB | 5.681818 ms |
| € 0.01070 | 1000 GB | 2.3 GHz | 16 | 10000 GB | 5.681818 ms |

---

### 4. Constantes Físicas e Fórmulas de Rede

O cálculo das latências e rotas leva em consideração os seguintes parâmetros físicos e equações matemáticas:

* **Raio da Terra:** 6371.0088 km
* **Pi ($\pi$):** 3.141592653589793
* **Velocidade da Luz ($c$):** 299792.458 km/s
* **Latência Inter-Datacenter (EC $\leftrightarrow$ CC):** 111.86 ms

As equações determinam o tempo de resposta total de cada requisição:

1. **Tempo de Processamento:**
   $$T_{proc} = s^d \cdot TP_i$$
2. **Tempo de Transmissão:**
   $$T_{tran} = \left( \frac{s^d}{bw^d} \right) \cdot 1000$$
3. **Tempo de Propagação (onde $dist$ é a distância em km do dispositivo até o EC que o cobre):**
   $$T_{prop} = \left( \frac{dist}{c} \right) \cdot 1000$$
4. **Tempo de Conexão ($T_{cone}$):**
   * Se atendido no **Edge:** $T_{cone} = T_{tran} + T_{prop}$
   * Se atendido na **Cloud:** $T_{cone} = T_{tran} + T_{prop} + 111.86$
5. **Tempo de Resposta Final:**
   $$T_{resp} = (2 \cdot T_{cone}) + T_{proc}$$

---

### 5. Parâmetros Globais do Experimento

Para a bateria de testes e coleta de resultados, a simulação foi fixada com as seguintes variáveis de controle:

* **Topologia de Servidores:** 100 Servidores Edge (EC) e 5 Servidores Cloud (CC).
* **Repetições:** 35 execuções para algoritmos estocásticos (Heurísticas Aleatórias e SA).
* **Distribuição de Chegadas:** Processo de Poisson com $\lambda = 20$.
* **Modelo Exato (CPLEX):** Tempo limite máximo de execução de 2 segundos por passo de tempo.
* **Simulated Annealing (SA):** Temperatura inicial $T_0 = 10.0$ e fator de resfriamento $\alpha = 0.99$.
* **Cenários Avaliados:** Volumes de execução variando entre 1000, 2000, 3000, 4000 e 5000 passos de tempo.
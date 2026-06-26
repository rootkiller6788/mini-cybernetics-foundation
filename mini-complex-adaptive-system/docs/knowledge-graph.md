# Knowledge Graph — mini-complex-adaptive-system

## L1: Definitions (Complete ✅)

| # | Entry | C Type | Lean | Source |
|---|-------|--------|------|--------|
| 1.1 | Agent | `cas_agent_t` | `Agent` | cas_agent.h |
| 1.2 | Internal Model | `cas_internal_model_t` | `InternalModel` | cas_agent.h |
| 1.3 | Building Block | `cas_building_block_t` | `BuildingBlock` | cas_agent.h |
| 1.4 | Tag | `cas_agent_tag_t` | `Tag` | cas_agent.h |
| 1.5 | Fitness | `double fitness` | `Agent.fitness` | cas_agent.h |
| 1.6 | Sensor | `cas_sensor_t` | - | cas_agent.h |
| 1.7 | Effector | `cas_effector_t` | - | cas_agent.h |
| 1.8 | NK Landscape | `cas_nk_landscape_t` | `NKLandscape` | cas_landscape.h |
| 1.9 | Genotype | `cas_landscape_point_t` | `Genotype N` | cas_landscape.h |
| 1.10 | Population | `cas_population_t` | `Population` | cas_dynamics.h |
| 1.11 | Adaptation Strategy | `cas_adapt_strategy_t` | `AdaptStrategy` | cas_agent.h |
| 1.12 | Agent Type | `cas_agent_type_t` | `AgentType` | cas_agent.h |

## L2: Core Concepts (Complete ✅)

| # | Entry | Implementation | Source |
|---|-------|---------------|--------|
| 2.1 | OODA Loop | `cas_agent_step()` | cas_agent.c |
| 2.2 | Bounded Rationality | Sensor noise + range limits | cas_agent.c |
| 2.3 | Emergence | `EmergenceLevel`, `cas_emergence_detect()` | cas_emergence.h/cas_dynamics.c |
| 2.4 | Co-evolution | `cas_coevolve_*()` | cas_dynamics.c |
| 2.5 | Self-Organization | `cas_self_org_compute()` | cas_emergence.c |
| 2.6 | Phase Transition | `cas_phase_transition_detect()` | cas_emergence.c |
| 2.7 | Social Learning | `cas_agent_interact()` | cas_agent.c |
| 2.8 | Diversity | Simpson index in `cas_pop_compute_stats()` | cas_dynamics.c |
| 2.9 | Robustness vs Fragility | Network cascade model | cas_network.c |
| 2.10 | Path Dependence | Adaptive walk on NK landscape | cas_landscape.c |
| 2.11 | Edge of Chaos | `edgeOfChaos` theorem | cas_lean.lean |

## L3: Mathematical Structures (Complete ✅)

| # | Entry | Implementation | Source |
|---|-------|---------------|--------|
| 3.1 | State Space R^n | `double state[16]` | cas_agent.h |
| 3.2 | Boolean Hypercube {0,1}^N | NK genotype bit operations | cas_landscape.c |
| 3.3 | Graph/Adjacency Matrix | `cas_network_t.adjacency[][]` | cas_network.h |
| 3.4 | Graph Laplacian | `cas_net_laplacian_spectrum()` | cas_network.c |
| 3.5 | Probability Distribution | `cas_master_eq_t` | cas_dynamics.h |
| 3.6 | Hamming Distance | `cas_nk_hamming_distance()` | cas_landscape.c |
| 3.7 | Fitness Landscape | `FitnessLandscape` type | cas_lean.lean |
| 3.8 | Correlation Function | `cas_nk_fitness_correlation()` | cas_landscape.c |
| 3.9 | Schema Template | `Schema N` type, `cas_schema_t` | cas_lean.lean/cas_evolution.h |

## L4: Fundamental Laws (Complete ✅)

| # | Theorem | C Implementation | Lean Statement | Source |
|---|---------|-----------------|----------------|--------|
| 4.1 | NK Correlation (Weinberger 1990) | `cas_nk_fitness_correlation()` | `nkCorrelation` | cas_landscape.c |
| 4.2 | Complexity Catastrophe (Kauffman 1993) | `cas_nk_adaptation_velocity()` | `adaptiveWalkLengthLog` | cas_landscape.c |
| 4.3 | Schema Theorem (Holland 1975) | `cas_schema_lower_bound()` | `schemaTheoremBound` | cas_evolution.c |
| 4.4 | Price Equation (Price 1970) | `cas_price_eq_compute()` | - | cas_evolution.c |
| 4.5 | Fisher's Fundamental Theorem | `cas_price_eq_fundamental_theorem()` | `fisherTheorem` | cas_evolution.c |
| 4.6 | Sandpile SOC (BTW 1987) | `cas_sandpile_powerlaw_exponent()` | - | cas_dynamics.c |
| 4.7 | Adaptive Walk Termination | `cas_nk_adaptive_walk_step()` | `adaptiveWalkTerminates` | cas_landscape.c |
| 4.8 | Causal Emergence (Hoel 2013) | `cas_causal_emergence_compute()` | - | cas_emergence.c |
| 4.9 | Selection Non-decreasing | - | `selectionNonDecreasing` | cas_lean.lean |
| 4.10 | NK Random Energy Equivalence | - | `nkRandomEnergyEquivalence` | cas_lean.lean |

## L5: Algorithms/Methods (Complete ✅)

| # | Algorithm | Function | Complexity |
|---|-----------|----------|------------|
| 5.1 | NK Adaptive Walk | `cas_nk_adaptive_walk_step()` | O(N²) |
| 5.2 | Genetic Algorithm | `cas_ga_generation()` | O(pop·genome_bits) |
| 5.3 | Evolution Strategy (μ+λ) | `cas_es_step()` | O(μ·dim) |
| 5.4 | TD(0) Learning | `cas_agent_adapt()` | O(state_dim) |
| 5.5 | Louvain Communities | `cas_net_louvain_communities()` | O(n·m) |
| 5.6 | Brandes Betweenness | `cas_net_betweenness_centrality()` | O(n·(n+m)) |
| 5.7 | PageRank | `cas_net_pagerank()` | O(iter·m) |
| 5.8 | Transfer Entropy | `cas_transfer_entropy()` | O(n·bins³) |
| 5.9 | Largest Lyapunov Exponent | `cas_lyapunov_exponent()` | O(n²) |
| 5.10 | Hurst Exponent (R/S) | `cas_hurst_exponent()` | O(n·log n) |
| 5.11 | False Nearest Neighbors | `cas_false_nearest_neighbors()` | O(n·dim·k) |
| 5.12 | Master Equation | `cas_master_eq_step()` | O(n_states²) |
| 5.13 | Langevin Dynamics | `cas_langevin_step()` | O(1) |

## L6: Canonical Problems (Complete ✅)

| # | Problem | Example File | Description |
|---|---------|-------------|-------------|
| 6.1 | NK Landscape Navigation | example_nk_navigation.c | Adaptive walk on rugged landscapes |
| 6.2 | El Farol Bar Problem | example_el_farol.c | Emergent coordination without central control |
| 6.3 | Bak-Tang-Wiesenfeld Sandpile | example_sandpile_soc.c | Self-organized criticality |
| 6.4 | GA on NK Landscape | example_ga_optimization.c | Evolutionary optimization vs greedy walk |
| 6.5 | Minority Game | `cas_minority_game_*()` | Self-organized resource allocation |

## L7: Applications (Partial+)

| # | Application | Notes |
|---|------------|-------|
| 7.1 | Economic markets as CAS | El Farol / Minority Game implementations |
| 7.2 | Ecosystem dynamics | Co-evolution and population dynamics |
| 7.3 | Organizational behavior | Social learning, tag-based interaction |
| 7.4 | Supply chain networks | Network cascade and robustness analysis |

## L8: Advanced Topics (Partial+)

| # | Topic | Implementation |
|---|-------|---------------|
| 8.1 | Causal Emergence | `cas_causal_emergence_compute()` |
| 8.2 | Integrated Information (Phi) | `cas_integrated_information()` |
| 8.3 | Nonlinear Dynamics Detection | `cas_nonlinearity_detect()` |
| 8.4 | Transfer Entropy | `cas_transfer_entropy()` |
| 8.5 | Self-Organized Criticality | `cas_sandpile_*()` |

## L9: Research Frontiers (Partial)

| # | Topic | Status |
|---|-------|--------|
| 9.1 | Quantum CAS | Documented |
| 9.2 | Deep Multi-Agent RL for CAS | Documented |
| 9.3 | Meta-complexity in CAS | Documented |

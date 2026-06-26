# Knowledge Graph — mini-von-neumann-automata

## L1: Definitions (Complete)

| # | Concept | C Implementation | Lean Formalization |
|---|---------|-----------------|-------------------|
| 1 | Cellular Automaton (L,S,N,f) | `VnaLattice` struct | `CAConfig` structure |
| 2 | von Neumann Neighborhood | `VNA_VON_NEUMANN` enum, offset arrays | `vonNeumannOffsets` |
| 3 | Moore Neighborhood | `VNA_MOORE` enum | `mooreOffsets` |
| 4 | Transition Rule | `VnaTransitionRule` struct | `TransitionRule` structure |
| 5 | Universal Constructor | `VnaUniversalConstructor` | `SelfReplicatingRule` |
| 6 | 29-State Automaton | `VN29State` enum | `VN29State` inductive |
| 7 | Quiescent State | `vna_find_quiescent_state()` | `isQuiescentConfig` |
| 8 | Probabilistic Gate | `VnaProbabilisticGate` | — (continuous) |
| 9 | Bundle (Multiplexing) | `VnaBundle` | — (continuous) |
| 10 | Pattern | `VnaPattern` struct | — |
| 11 | State Histogram | `VnaStateHistogram` | — |
| 12 | CA Statistics | `VnaCAStatistics` | — |

## L2: Core Concepts (Complete)

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Self-Replication | `vna_rule_replication_score()`, `VnaReplicationPhase` |
| 2 | Emergent Complexity | Damage spreading, entropy analysis |
| 3 | Fault-Tolerant Computation | Multiplexed NAND, TMR/NMR |
| 4 | Quiescent Configuration | `vna_is_quiescent_config()` |
| 5 | Signal Propagation | Replication score analysis |
| 6 | Configuration Space | Lattice operations, `vna_sublattice_extract` |
| 7 | Synchronous Update | `vna_lattice_evolve()` |
| 8 | Asynchronous Update | `vna_lattice_evolve_async()` |
| 9 | Block Synchronous | `vna_block_get/set` for Margolus |
| 10 | Configurations | `vna_configuration_is_finite()` |

## L3: Mathematical Structures (Complete)

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Lattice (Z^2, finite) | `VnaLattice` with width/height/cells |
| 2 | State Space (S^k) | `num_states`, `VnaStateType` enum |
| 3 | Neighborhood as offsets | `VnaNeighborhood` offset arrays |
| 4 | Rule as lookup table | `VnaTransitionRule.rule_table[]` |
| 5 | Mixed-radix encoding | `tuple_to_index()`, `vna_neighbor_tuple_to_index` |
| 6 | De Bruijn Diagram | `VnaDeBruijnDiagram`, `vna_rule_debruijn_diagram` |
| 7 | Subset Diagram | `VnaSubsetDiagram` |
| 8 | Neighborhood Graph | `VnaNeighborhoodGraph` |
| 9 | Block Partition | `vna_block_partition_count()` |
| 10 | Rule Space | `vna_rule_space_size()` |

## L4: Fundamental Laws (Complete)

| # | Theorem | Verification |
|---|---------|-------------|
| 1 | Von Neumann Error Threshold | `vna_critical_error_threshold()`, tests |
| 2 | Multiplexing Reliability | `vna_multiplexing_reliability()`, Lean statement |
| 3 | Rule 110 Universality (Cook) | `vna_rule110_create()`, Lean statement |
| 4 | Conway's Game of Life Completeness | `vna_rule_game_of_life()`, pattern library |
| 5 | Self-Reproduction Existence | `vna_langton_loop_pattern()`, `VnaUniversalConstructor` |
| 6 | Codd's 8-State CA | `Codd8State` in Lean |
| 7 | Langton's Loops | `VnaLangtonLoop` struct |
| 8 | Reversibility (1D) | `vna_rule_is_reversible_1d()` |
| 9 | Number Conservation | `vna_rule_is_number_conserving()` |
| 10 | Additivity | `vna_rule_is_additive()` |

## L5: Algorithms (Complete)

| # | Algorithm | Implementation |
|---|-----------|---------------|
| 1 | Synchronous CA Evolution | `vna_lattice_evolve()` |
| 2 | Asynchronous Evolution | `vna_lattice_evolve_async()` |
| 3 | Still Life Detection | `vna_is_still_life()` |
| 4 | Oscillator Detection | `vna_detect_oscillator()` |
| 5 | Glider Detection | `vna_detect_gliders()`, `vna_gol_count_gliders()` |
| 6 | Damage Spreading | `vna_damage_spreading()`, Lyapunov estimate |
| 7 | Block Entropy | `vna_block_entropy()` |
| 8 | Mean-Field Theory | `vna_mean_field_density()` |
| 9 | Wolfram Classification | `vna_rule_classify_wolfram()` |
| 10 | Rule Synthesis | `vna_synthesize_rule_for_pattern()` |
| 11 | Pattern Matching | `vna_pattern_match()` |
| 12 | HashLife (specification) | `Quadtree` in Lean |

## L6: Canonical Problems (Complete)

| # | Problem | Implementation |
|---|---------|---------------|
| 1 | Game of Life Simulation | `example_game_of_life.c`, GoL rule + patterns |
| 2 | Self-Replicating Loops | `example_self_replication.c` |
| 3 | Fault-Tolerant NAND | `example_error_correction.c` |
| 4 | Traffic Flow (Nagel-Schreckenberg) | `example_traffic_model.c` |
| 5 | Rule 110 Gliders | `vna_rule110_glider_create()`, collision simulation |
| 6 | Game of Life Pattern Library | `gol_lexicon[]` with 9 canonical patterns |

## L7: Applications (Complete)

| # | Application | Implementation |
|---|-------------|---------------|
| 1 | Fault-Tolerant Computing | `vna_tmr_system_reliability()`, multiplexed gates |
| 2 | NASA Flight Computer (TMR) | `vna_tmr_system_reliability()` with reliability analysis |
| 3 | Artificial Life | `VnaPopulation`, `vna_population_update()` |
| 4 | Traffic Flow Modeling | `example_traffic_model.c`, fundamental diagram |
| 5 | Reliable Memory | `VnaReliableMemory`, `vna_reliable_memory_write()` |
| 6 | PRNG from Rule 110 | `vna_rule110_random_01()` |

## L8: Advanced Topics (Complete)

| # | Topic | Implementation |
|---|-------|---------------|
| 1 | Damage Spreading / Lyapunov | `VnaDamageSpread`, `vna_damage_spreading()` |
| 2 | Circular Neighborhood | `vna_neighborhood_circular()` |
| 3 | Gaussian Neighborhood | `vna_neighborhood_gaussian()` |
| 4 | Stochastic CA | `vna_rule_stochastic_perturb()` |
| 5 | Block Renormalization | `vna_rule_block_renormalize()` |
| 6 | Reversible CA | `isReversible` in Lean |
| 7 | Garden of Eden Detection | `vna_detect_garden_of_eden()` |

## L9: Research Frontiers (Partial — Documented)

| # | Topic | Status |
|---|-------|--------|
| 1 | Meta-Complexity of CA Classification | Documented |
| 2 | Quantum Cellular Automata | Documented (Lean notes) |
| 3 | CA-Based Cryptography | Documented (Rule 30, etc.) |
| 4 | Morphogenesis via CA | Documented |
| 5 | Self-Organizing Criticality in CA | Documented |

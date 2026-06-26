# Course Alignment — mini-complex-adaptive-system

## Nine-School Mapping

### MIT — 6.883 Complex Adaptive Systems
| Topic | Implementation |
|-------|---------------|
| Holland's 7 Basics | `cas_agent_t` integrates all 7 |
| Tags | `cas_agent_tag_t`, `cas_agent_add_tag()` |
| Building Blocks | `cas_building_block_t`, `cas_agent_add_building_block()` |
| Internal Models | `cas_internal_model_t`, TD learning in `cas_agent_adapt()` |
| Aggregation | `cas_population_t`, hierarchical agent types |
| Nonlinearity | `cas_nonlinearity_detect()`, NK ruggedness |
| Flows | Energy model in `cas_agent_act()` |
| Diversity | Simpson index in `cas_pop_compute_stats()` |

### Stanford — MS&E 338 Multi-Agent Systems
| Topic | Implementation |
|-------|---------------|
| Agent Architectures | 5 agent types in `cas_agent_type_t` |
| Multi-agent Interaction | `cas_agent_interact()`, `cas_coevolve_*()` |
| Game Theory in MAS | El Farol, Minority Game |
| Learning in MAS | TD learning, social learning |

### SFI — Complexity Explorer
| Topic | Implementation |
|-------|---------------|
| NK Fitness Landscapes | Full implementation in `cas_landscape.*` |
| Self-Organized Criticality | BTW sandpile in `cas_dynamics.c` |
| Emergence | `cas_emergence_detect()`, info-theoretic measures |
| Edge of Chaos | Lambda parameter, phase transition detection |

### Cambridge — Part III Complex Systems
| Topic | Implementation |
|-------|---------------|
| Fitness Landscapes | NK model, correlation function |
| Complexity Catastrophe | `cas_nk_adaptation_velocity()` |
| Power Laws | Sandpile power-law exponent |
| Network Theory | Small-world, scale-free, community detection |

### Oxford — B14 Agent-Based Modeling
| Topic | Implementation |
|-------|---------------|
| Agent Design | `cas_agent_init()`, 5 agent types |
| Population Dynamics | `cas_pop_step()`, selection, reproduction, mutation |
| Model Validation | Test suite with 15 assertions |

### Caltech — CDS 240 Nonlinear Dynamics
| Topic | Implementation |
|-------|---------------|
| Langevin Dynamics | `cas_langevin_step()`, Kramers escape rate |
| Lyapunov Exponents | `cas_lyapunov_exponent()` |
| Phase Transitions | `cas_phase_transition_detect()` |
| Chaos Detection | `cas_nonlinearity_detect()` |

### ETH — 263-4650 Advanced Complexity
| Topic | Implementation |
|-------|---------------|
| Master Equation | `cas_master_eq_step()`, steady state |
| Mean-Field Theory | `cas_mean_field_compute()` |
| Critical Phenomena | Binder cumulant, susceptibility |

### CMU — 24-654 Systems Thinking
| Topic | Implementation |
|-------|---------------|
| Feedback Loops | OODA loop, TD learning error feedback |
| System Dynamics | Population-level statistics |
| Network Effects | Cascade model, robustness analysis |

### Princeton — ELE 530 Estimation
| Topic | Implementation |
|-------|---------------|
| Information Theory | Entropy, MI, conditional entropy, transfer entropy |
| Causal Inference | Transfer entropy, causal emergence |
| Time Series Analysis | Hurst, Lyapunov, autocorrelation, detrending |

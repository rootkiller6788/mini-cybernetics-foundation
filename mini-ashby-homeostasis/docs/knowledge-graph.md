# Knowledge Graph — Mini Ashby Homeostasis

## L1: Definitions
| Entry | C Type | Status |
|-------|--------|--------|
| Homeostasis | `HomeostasisStatus` (enum) | ✅ Complete |
| Essential Variable | `EssentialVariable` (struct) | ✅ Complete |
| Survival Zone | `EssentialVariableSet` (struct) | ✅ Complete |
| Homeostat | `HomeostatUnit`, `HomeostatSystem` | ✅ Complete |
| Ultrastability | `UltrastableSystem` (struct) | ✅ Complete |
| Variety | `VarietySpace` (struct) | ✅ Complete |
| Parameter Field | `ParameterField` (struct) | ✅ Complete |
| Regulator | `RegulatorType` (enum) | ✅ Complete |
| Environment | `AshbyEnvironment` (struct) | ✅ Complete |
| Disturbance | `DisturbanceType` (enum) | ✅ Complete |
| Internal Model | `InternalModelType` (enum) | ✅ Complete |
| Bayesian Belief | `BayesianParameterBelief` (struct) | ✅ Complete |

## L2: Core Concepts
| Concept | Implementation | Status |
|---------|---------------|--------|
| Negative Feedback | `REGULATOR_ERROR_DRIVEN` | ✅ Complete |
| Two-Level Control | `UltrastableSystem` (reacting + step-function) | ✅ Complete |
| Trial-and-Error Adaptation | `param_field_step()`, `homeostat_unit_step_parameter()` | ✅ Complete |
| Variety Absorption | `VarietyChannel`, `variety_channel_evaluate()` | ✅ Complete |
| Model-Based Regulation | `AdaptiveRegulator`, `adaptive_regulator_total_action()` | ✅ Complete |
| Homeostatic Plateau | `homeostat_system_stability_index()` | ✅ Complete |

## L3: Mathematical Structures
| Structure | Type | Status |
|-----------|------|--------|
| State-Space Model | `AshbySystem::state_vector` + `internal_coupling` | ✅ Complete |
| Coupling Matrix | `AHMatrix` | ✅ Complete |
| Survival Zone (N-box) | `EssentialVariableSet::survival_volume` | ✅ Complete |
| Shannon Entropy (Variety) | `VarietySpace::total_variety` | ✅ Complete |
| Linear Dynamical Model | `LinearDynamicalModel` | ✅ Complete |
| Time Series | `AHTimeSeries` | ✅ Complete |
| Feedback Matrix | `env_coupling`, `internal_coupling` | ✅ Complete |

## L4: Fundamental Laws
| Law/Theorem | Statement | Verification |
|-------------|-----------|-------------|
| Law of Requisite Variety | V(R) >= V(D) - V(E) | `variety_channel_evaluate()` + test |
| Conant-Ashby Theorem | Regulator must model system | `conant_ashby_isomorphism_measure()` + test |
| Ultrastability Convergence | Random search finds stable config | `ultrastable_search_complexity()` + example |
| Homeostat Equilibrium | |θ| < threshold for all units | `homeostat_unit_check_stability()` |

## L5: Algorithms/Methods
| Algorithm | Function | Status |
|-----------|----------|--------|
| Homeostat Physics | `homeostat_unit_dynamics()` | ✅ Complete |
| Step-Function Search | `param_field_step()` | ✅ Complete |
| PID Control | `adaptive_regulator_pid_action()` | ✅ Complete |
| Adaptive Gain | `REGULATOR_ADAPTIVE` in `ashby_system_apply_regulation()` | ✅ Complete |
| Predictive Regulation | `REGULATOR_PREDICTIVE` in `ashby_system_apply_regulation()` | ✅ Complete |
| Learning (Integral) | `REGULATOR_LEARNING` in `ashby_system_apply_regulation()` | ✅ Complete |
| Bayesian Updating | `bayesian_belief_update()` | ✅ Complete |
| Variety Estimation | `variety_estimate_from_samples()` | ✅ Complete |

## L6: Canonical Problems
| Problem | Example | Status |
|---------|---------|--------|
| 4-Unit Homeostat Stabilization | `examples/example_homeostat.c` | ✅ Complete |
| Ultrastable Parameter Search | `examples/example_ultrastability.c` | ✅ Complete |
| Multi-Stage Variety Pipeline | `examples/example_variety.c` | ✅ Complete |

## L7: Applications
| Application | Implementation | Keywords |
|-------------|---------------|----------|
| Biological Thermoregulation | `BodyThermoregulation` in `homeostasis_apps.c` | body temperature, homeostasis |
| Organizational Cybernetics | `OrganizationalHomeostat` in `homeostasis_apps.c` | VSM, resource, adaptation |
| Building Climate Control | `ClimateHomeostat` in `homeostasis_apps.c` | HVAC, temperature, humidity |
| Robot Navigation | `RobotHomeostat` in `homeostasis_apps.c` | battery, obstacle, goal |
| Supply Chain | `SupplyChainHomeostat` in `homeostasis_apps.c` | inventory, demand, lead time |

## L8: Advanced Topics
| Topic | Implementation | Status |
|-------|---------------|--------|
| Bayesian Adaptive Regulation | `BayesianParameterBelief` | ✅ Complete |
| Multi-Level Ultrastability | `ULTRASTABLE_DUAL`, `ULTRASTABLE_MULTI` enums | Partial (structure only) |
| Parameter Sensitivity | `ultrastable_parameter_sensitivity()` | ✅ Complete |
| Variety Rate-Distortion | `variety_rate_distortion()` | ✅ Complete |

## L9: Research Frontiers
| Topic | Documentation | Status |
|-------|--------------|--------|
| Homeostasis in AI Alignment | `gap-report.md` | 📄 Documented |
| Quantum Homeostat Concepts | `gap-report.md` | 📄 Documented |
| Meta-Ultrastability | `gap-report.md` | 📄 Documented |
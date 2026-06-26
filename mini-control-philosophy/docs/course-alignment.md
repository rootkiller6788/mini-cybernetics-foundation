# Course Alignment — mini-control-philosophy

## Nine-School Mapping

### MIT
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| 6.241J Dynamic Systems | State-space, stability, Lyapunov | `cp_system_lyapunov_value`, `ControlSystem` struct |
| 6.832 Underactuated Robotics | Hierarchical control | `cp_hierarchical_step`, `ControlLayer` |
| 6.841 Advanced Complexity | Information in control | `cp_system_channel_capacity`, `cp_information_bottleneck` |

### Stanford
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| AA203 Optimal Control | Utility functions, cost-to-go | `cp_utility_evaluate`, `cp_system_step` cost accumulation |
| EE363 Convex Optimization | Gramians, LMIs | `cp_observability_gramian_solve` |
| AA274 Multi-agent | Coordination overhead | `cp_hierarchical_coordination_overhead` |

### Berkeley
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| EE221A Linear Systems | Observability, controllability | `cp_observability_matrix`, `cp_observability_rank` |
| EE222 Nonlinear Systems | Lyapunov stability | `cp_system_classify_stability` |

### CMU
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| 18-771 Linear Systems | Separation principle | `cp_observer_controller_check_separation` |
| 24-677 Nonlinear Control | Feedback linearization concepts | `cp_system_set_feedback` |
| 24-654 Systems Thinking | Emergence, hierarchy | `cp_emergence_analyze`, `cp_near_decomposability` |

### Princeton
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| MAE 546 Optimal Control | Value of information | `cp_value_of_information` |
| ELE 530 Estimation | Kalman filtering | `cp_observer_predict`, `cp_observer_update` |

### Caltech
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| CDS110 Intro Control | Feedback fundamentals | `cp_system_step` (the four-step control loop) |
| CDS140 Nonlinear Dynamics | Stability classification | `cp_system_classify_stability` |

### Cambridge
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| 4F2 Robust Control | Bode integral, waterbed | `cp_bode_sensitivity_integral`, `cp_waterbed_index` |
| 4F3 Nonlinear Control | Internal model principle | `cp_internal_model_principle_check` |

### Oxford
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| B4 Predictive Control | Model-based regulation | `cp_regulator_update`, `cp_internal_model_predict` |
| C20 Adaptive Control | Parameter adaptation | `cp_system_set_paradigm(CP_ADAPTIVE)` |

### ETH
| Course | Topic | Our Implementation |
|--------|-------|--------------------|
| 263-4650 Advanced Complexity | Information theory in systems | `cp_channel_compute_capacity`, `cp_transfer_entropy_discrete` |
| 252-0400 Logic & Computation | Lean formal verification | 8 theorems in `control_philosophy.lean` |

## Core Textbook Mapping

| Textbook | Author | Coverage |
|----------|--------|----------|
| Cybernetics (1948) | Wiener | Full: feedback, communication, purpose |
| Introduction to Cybernetics (1956) | Ashby | Full: variety, regulation, homeostasis |
| Design for a Brain (1952) | Ashby | Partial: adaptation, learning |
| Brain of the Firm (1972) | Beer | Full: VSM, hierarchical control |
| Theory of Hierarchical Multilevel Systems (1970) | Mesarovic et al. | Full: layers, coordination |
| The Architecture of Complexity (1962) | Simon | Full: near-decomposability, emergence |
| General System Theory (1968) | Bertalanffy | Partial: open systems, hierarchy |
| Linear Multivariable Control (1979) | Wonham | Partial: internal model principle |
| Behavior, Purpose and Teleology (1943) | Rosenblueth, Wiener, Bigelow | Full: teleology taxonomy |

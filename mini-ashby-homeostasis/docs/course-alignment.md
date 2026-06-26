# Course Alignment — Mini Ashby Homeostasis

## Nine-School Curriculum Mapping

### MIT
- **6.241 Dynamical Systems and Control** (Frazzoli, Megretski)
  - State-space models: `LinearDynamicalModel`, `AshbySystem::state_vector`
  - Stability analysis: `HomeostasisStatus`, `homeostat_unit_check_stability()`
  - Lyapunov methods: energy-based stability via `homeostat_energy_*()` functions
- **6.841 Advanced Complexity Theory** (Aaronson)
  - Variety as complexity measure: `VarietySpace` ↔ computational distinguishability
  - Search complexity: `ultrastable_search_complexity()`

### Stanford
- **CS229 Machine Learning** (Ng, Ma)
  - Bayesian adaptation: `BayesianParameterBelief`, `bayesian_belief_update()`
  - Online learning: `adaptive_regulator_update_model()`
- **EE263 Introduction to Linear Dynamical Systems** (Boyd)
  - Linear models: `LinearDynamicalModel` with A, B, C matrices
  - Kalman-like updates: `bayesian_belief_update()`

### Berkeley
- **EE221A Linear Systems Theory** (Tomlin)
  - Feedback control: `adaptive_regulator_pid_action()`
  - Feedforward: `adaptive_regulator_feedforward_action()`
  - Observability: implicit in model-predictive regulation

### CMU
- **18-771 Linear Systems** (Gharesifard)
  - Linear dynamical models: `LinearDynamicalModel`
  - Coupling matrices: `AHMatrix`, `env_coupling`, `internal_coupling`
- **15-455/855 Complexity Theory** (Impagliazzo)
  - Search complexity: ultrastable parameter search as NP-like search

### Princeton
- **MAE 546 Optimal Control and Estimation** (Stengel)
  - Predictive regulation: `REGULATOR_PREDICTIVE`
  - Model-based control: Conant-Ashby theorem implementation
- **COS 551 Advanced Complexity** (Allender)
  - Information-theoretic limits: Ashby's Law as complexity bound

### Caltech
- **CDS 110 Introduction to Control Systems** (Murray)
  - Feedback fundamentals: `REGULATOR_ERROR_DRIVEN`
  - Adaptive control: `REGULATOR_ADAPTIVE`
- **CS/CNS 154 Limits of Computation** (Wigderson)
  - Fundamental limits: Law of Requisite Variety

### Cambridge
- **Part II Control Theory** (Sepulchre)
  - Two-level control: `UltrastableSystem` architecture
  - Robustness: disturbance rejection via variety management
- **Part II Complexity Theory** (Gowers)
  - Variety as information-theoretic complexity

### Oxford
- **Control Theory** (Papachristodoulou)
  - Robust regulation: variety-based disturbance attenuation
  - Adaptive systems: step-function parameter adaptation
- **Advanced Complexity Theory** (Krauthgamer)
  - Search-vs-decision: ultrastability as heuristic search

### ETH Zurich
- **227-0216 Control Systems II** (Smith, D'Andrea)
  - Adaptive control: `AdaptiveRegulator` with online model updates
  - System identification: `adaptive_regulator_update_model()`
- **263-4650 Advanced Complexity Theory** (Holenstein)
  - Information theory: variety and Shannon entropy

## Topic-to-Course Matrix

| Topic | MIT | Stanford | Berkeley | CMU | Princeton | Caltech | Cambridge | Oxford | ETH |
|-------|-----|----------|----------|-----|-----------|---------|-----------|--------|-----|
| Essential Variables | 6.241 | - | EE221A | 18-771 | MAE546 | CDS110 | PTII | CT | 227-0216 |
| Homeostat Model | 6.241 | - | - | - | - | CDS110 | - | - | - |
| Ultrastability | 6.241 | CS229 | - | 15-855 | - | - | PTII | CT | 227-0216 |
| Ashby's Law | 6.841 | - | - | 15-455 | COS551 | CS154 | PTII | ACT | 263-4650 |
| Conant-Ashby | 6.241 | CS229 | EE221A | 18-771 | MAE546 | CDS110 | PTII | CT | 227-0216 |
| Variety Engineering | 6.841 | - | - | 15-455 | COS551 | CS154 | PTII | ACT | 263-4650 |
| Adaptive Regulation | 6.241 | CS229 | EE221A | 18-771 | MAE546 | CDS110 | PTII | CT | 227-0216 |
| Bayesian Adaptation | 6.841 | CS229 | - | - | - | - | - | - | - |
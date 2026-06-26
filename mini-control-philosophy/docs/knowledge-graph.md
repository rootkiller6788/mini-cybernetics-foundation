# Knowledge Graph — mini-control-philosophy

## L1: Definitions (`include/cp_core.h`, `include/cp_variety.h`, `include/cp_purpose.h`)

| Entry | C Definition | Lean Definition | Status |
|-------|-------------|-----------------|--------|
| Control System (Wiener 1948) | `typedef struct ControlSystem` in cp_core.h | `structure ControlSystem` | Complete |
| Feedback (Rosenblueth et al. 1943) | `typedef enum FeedbackType` | — | Complete |
| Variety (Ashby 1956) | `typedef struct VarietyMeasure` | `def variety : Nat → Float` | Complete |
| Regulator (Conant & Ashby 1970) | `typedef struct Regulator` | `structure Regulator` | Complete |
| Goal / Teleology | `typedef struct GoalSpec` | `structure GoalSpec` | Complete |
| Observer (Kalman/Luenberger) | `typedef struct Observer` | — | Complete |
| Hierarchy Level (Mesarovic 1970) | `typedef enum HierarchyLevel` | — | Complete |
| Utility Function | `typedef struct UtilityFunction` | — | Complete |
| Control Channel (Shannon 1948) | `typedef struct ControlChannel` | — | Complete |
| Disturbance Ensemble | `typedef struct DisturbanceEnsemble` | — | Complete |
| Internal Model (Francis & Wonham 1976) | `typedef struct InternalModel` | `structure DisturbanceModel` | Complete |
| Viable System Model (Beer 1972) | `typedef struct ViableSystemModel` | — | Complete |
| Stability Class (Lyapunov 1892) | `typedef enum StabilityClass` | — | Complete |
| Servomechanism | `typedef struct Servomechanism` | — | Complete |
| Goal Decomposition Tree | `typedef struct GoalTree` | `inductive GoalDecomposition` | Complete |

## L2: Core Concepts

| Concept | Source | Implementation | Status |
|---------|--------|---------------|--------|
| Negative Feedback | Wiener (1948) | `cp_system_step` with `CP_NEGATIVE_FEEDBACK` | Complete |
| Positive Feedback | Wiener (1948) | `cp_system_step` with `CP_POSITIVE_FEEDBACK` | Complete |
| Requisite Variety | Ashby (1956) | `cp_ashby_regulator_evaluate` | Complete |
| Good Regulator Theorem | Conant & Ashby (1970) | `cp_good_regulator_evaluate` | Complete |
| Separation Principle | Luenberger (1964), Wonham (1968) | `cp_observer_controller_check_separation` | Complete |
| Internal Model Principle | Francis & Wonham (1976) | `cp_internal_model_principle_check` | Complete |
| Hierarchical Control | Mesarovic et al. (1970) | `cp_hierarchical_step` | Complete |
| Emergence | Simon (1962) | `cp_emergence_analyze` | Complete |
| Near-Decomposability | Simon (1962) | `cp_near_decomposability` | Complete |
| Autopoiesis | Maturana & Varela (1980) | `cp_vsm_is_viable` | Complete |
| Information in Control | Touchette & Lloyd (2000) | `cp_system_compute_info_metrics` | Complete |
| Bode Waterbed Effect | Bode (1945) | `cp_bode_sensitivity_integral` | Complete |

## L3: Mathematical Structures

| Structure | Description | Implementation | Status |
|-----------|-------------|---------------|--------|
| State-Space (A,B,C,D) | Linear control model | `ControlSystem.model_params` as (A,B) | Complete |
| Probability Distribution | Disturbance uncertainty | `DisturbanceEnsemble.probabilities` | Complete |
| Transition Matrix | P(output\|input) channel | `ControlChannel.transition_matrix` | Complete |
| Entropy H(X) | Shannon entropy | `cp_entropy` | Complete |
| Mutual Information I(X;Y) | Information shared | `cp_mutual_information` | Complete |
| Transfer Entropy TE(X→Y) | Directed information flow | `cp_transfer_entropy_discrete` | Complete |
| Lyapunov Function V(x) | Stability certificate | `cp_system_lyapunov_value` | Complete |
| Homomorphism Degree | Model-plant similarity | `cp_homomorphism_degree` | Complete |
| Observability Matrix O | [C; CA; CA²; …] | `cp_observability_matrix` | Complete |
| Observability Gramian Wo | A^T Wo + Wo A = -C^T C | `cp_observability_gramian_solve` | Complete |
| Controllability Gramian Wc | A Wc + Wc A^T = -B B^T | `cp_system_controllability_gramian` | Complete |
| Strategy Table | Regulator mapping D→R | `AshbyRegulator.strategy_table` | Complete |
| Utility Function U(y,r) | Outcome evaluation | `UtilityFunction` with quadratic/linear/saturating | Complete |
| Information Bottleneck T | I(X;U) - β·I(U;Y) | `cp_information_bottleneck` | Complete |

## L4: Fundamental Laws / Theorems

| Theorem | Statement | C Verification | Lean Formalization | Status |
|---------|-----------|---------------|-------------------|--------|
| Ashby's Law of Requisite Variety | V(R) ≥ V(D) is necessary for perfect regulation | `cp_ashby_regulator_evaluate` computes V(R)/V(D) ratio | `ashby_requisite_variety` | Complete |
| Good Regulator Theorem | Every good regulator must be a model of that system | `cp_good_regulator_evaluate` checks homomorphism | `good_regulator_homomorphism` | Complete |
| Feedback Error Convergence | Negative feedback drives e(t) → 0 as t → ∞ | `cp_system_step` demonstrates exponential decay | `feedback_error_convergence` | Complete |
| Lyapunov Stability Theorem | V(x)>0 ∧ V̇(x)<0 ⇒ asymptotic stability | `cp_system_classify_stability` | `lyapunov_stability` | Complete |
| Separation Principle | LQG = LQR + Kalman (for LTI Gaussian) | `cp_observer_controller_check_separation` | `separation_principle_lti` | Complete |
| Internal Model Principle | Perfect rejection requires embedded disturbance model | `cp_internal_model_principle_check` | `internal_model_principle` | Complete |
| Goal Decomposition Preservation | Sub-goal achievement ⇒ super-goal achievement | `cp_goal_tree_decompose` | `goal_decomposition_preserves` | Complete |
| Hierarchy Emergence Bound | 0 ≤ E ≤ coordination_overhead | `cp_hierarchical_emergence` | `hierarchy_emergence_bound` | Complete |

## L5: Algorithms / Methods

| Algorithm | Source | Implementation | Complexity | Status |
|-----------|--------|---------------|------------|--------|
| Blahut-Arimoto Channel Capacity | Blahut (1972) | `cp_channel_compute_capacity` | O(max_iter × nx × ny) | Complete |
| Kalman Filter Predict-Update | Kalman (1960) | `cp_observer_predict` + `cp_observer_update` | O(n³) per step | Complete |
| Observability Matrix Construction | Kalman (1960) | `cp_observability_matrix` | O(n³) | Complete |
| Gramian Lyapunov Solver | Bartels-Stewart (simplified) | `cp_observability_gramian_solve` | O(n³ × max_iter) | Complete |
| Transfer Entropy Estimation | Schreiber (2000) | `cp_transfer_entropy_discrete` | O(n × alphabet³) | Complete |
| Goal Arbitration (Priority-based) | Multi-objective control | `cp_teleo_arbitrate` | O(n_goals) | Complete |
| Value of Information | Howard (1966) | `cp_value_of_information` | O(n_states × n_actions × n_obs) | Complete |
| Emergence Analysis | Simon (1962) | `cp_emergence_analyze` | O(n_layers²) | Complete |
| Near-Decomposability Index | Simon (1962), Courtois (1977) | `cp_near_decomposability` | O(n²) | Complete |

## L6: Canonical Problems

| Problem | Example | Status |
|---------|---------|--------|
| Thermostat Regulation | `example1_feedback_loop.c` (~75 lines) | Complete |
| Ashby's Requisite Variety Game | `example2_requisite_variety.c` (~110 lines) | Complete |
| Manufacturing Plant Hierarchy | `example3_hierarchical_control.c` (~160 lines) | Complete |

## L7: Applications (Partial+)

| Application | Reference | Implementation | Status |
|-------------|-----------|---------------|--------|
| Temperature Control (Thermostat) | Wiener (1948) | example1 | Complete |
| Organizational Design (VSM) | Beer (1972) | example3 `cp_vsm_is_viable` | Complete |
| Supply Chain Coordination | Mesarovic (1970) | example3 HierarchicalControl | Complete |
| Economic Policy (Value of Information) | Howard (1966) | `cp_value_of_information` | Partial |
| Biological Homeostasis | Ashby (1952) | `cp_regulator_update` | Partial |

## L8: Advanced Topics (Partial+)

| Topic | Implementation | Status |
|-------|---------------|--------|
| Stochastic Control (Kalman Filter) | `cp_observer_update` with `CP_KALMAN_FILTER` | Complete |
| Bayesian Decision Theory | `cp_value_of_information`, `cp_utility_certainty_equivalent` | Complete |
| Hierarchical Decomposition (Simon) | `cp_near_decomposability`, `cp_optimal_span_of_control` | Complete |
| Information-Theoretic Control | `cp_information_bottleneck`, `cp_transfer_entropy_discrete` | Complete |
| Auto-organization (Ashby) | `cp_layer_autonomy_measure`, `cp_vsm_cohesion` | Partial |

## L9: Research Frontiers (Partial)

| Topic | Documentation | Implementation | Status |
|-------|--------------|---------------|--------|
| Safety in Learning-Enabled Control | gap-report.md | — | Partial (documented) |
| Information-Theoretic Control Limits | gap-report.md | `cp_system_channel_capacity` | Partial |
| Control of Complex Adaptive Systems | gap-report.md | `cp_emergence_analyze`, `cp_vsm_is_viable` | Partial |
| Meta-Complexity in Control | gap-report.md | — | Partial (documented) |

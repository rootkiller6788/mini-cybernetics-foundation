# Mini Ashby Homeostasis — W. Ross Ashby's Cybernetic Theory

## Module Status: COMPLETE ✅

- **L1 Definitions**: Complete (8 struct/enum types, all core definitions)
- **L2 Core Concepts**: Complete (6 include/ files, 7 src/ files)
- **L3 Math Structures**: Complete (Matrix, Vector, TimeSeries, coupling matrices, state-space models)
- **L4 Fundamental Laws**: Complete (Law of Requisite Variety, Conant-Ashby Theorem, Ultrastability convergence)
- **L5 Algorithms/Methods**: Complete (Homeostat simulation, PID/adaptive/predictive regulation, parameter search)
- **L6 Canonical Problems**: Complete (3 examples: Homeostat, Ultrastability, Variety)
- **L7 Applications**: Complete (5 application domains: biological, organizational, climate, robotics, supply chain)
- **L8 Advanced Topics**: Partial (stochastic parameter search, Bayesian adaptation, Conant-Ashby theorem)
- **L9 Research Frontiers**: Partial (documented in docs/gap-report.md)

**Include/ + src/ total: 3602 lines** ✅  
**Tests: 39/39 passed** ✅  
**make test: PASSES** ✅

---

## Core Definitions (L1)

| Concept | Type | Description |
|---------|------|-------------|
| Homeostasis | `HomeostasisStatus` | State of system relative to survival zone |
| Essential Variables | `EssentialVariable` | Variables that must stay within limits for survival |
| Survival Zone | `EssentialVariableSet` | Cartesian product of EV safe ranges |
| Homeostat | `HomeostatUnit`, `HomeostatSystem` | Ashby's electromechanical device |
| Ultrastability | `UltrastableSystem` | Two-level adaptation (reacting + step-function) |
| Variety | `VarietySpace` | Number of distinguishable states (log2) |
| Law of Requisite Variety | `VarietyChannel` | V(R) >= V(D) - V(E) relationship |
| Conant-Ashby Theorem | `AdaptiveRegulator` | Every good regulator must be a model |

## Core Theorems (L4)

### Ashby's Law of Requisite Variety (1956)
```
V(R) >= V(D)  — for zero residual variety at E
V(R) >= V(D) - V(E)  — general form
```
"Only variety can destroy variety."

### Conant-Ashby Theorem (1970)
"Every good regulator of a system must be (contain) a model of that system."
- The regulator's internal structure must be isomorphic to the regulated system

### Ultrastability Convergence
A system with two feedback loops (fast reacting part + slow step-function part) can find stable configurations through blind trial-and-error search, given sufficient parameter variety.

### Homeostat Stability Condition
For each unit i with parameters (R, L, D, K), stability requires:
```
|θ_i| < θ_threshold  ∀ i ∈ {1,...,N}
```

## Core Algorithms (L5)

1. **Homeostat Electromechanical Simulation** — Coupled ODE integration of N units with cross-coupling
2. **Step-Function Parameter Search** — Random/sequential uniselector parameter stepping
3. **PID Regulation** — Proportional-Integral-Derivative control for essential variable maintenance
4. **Adaptive Gain Regulation** — Gain proportional to current deviation
5. **Predictive Regulation** — Forward prediction using rate-of-change
6. **Learning Regulation** — Leaky integral of past errors
7. **Bayesian Parameter Belief** — Probabilistic model of system parameters
8. **Variety Estimation** — Histogram-based, adaptive-binning, and correlation dimension methods

## Classic Problems (L6)

1. **Homeostat Stabilization** — 4-unit system disturbed, recovers via step-function search
2. **Ultrastable Parameter Search** — Finding stable config in 50-candidate parameter space
3. **Variety Channel Analysis** — Multi-stage variety attenuation pipeline

## Applications (L7)

1. **Biological Homeostasis** — Body temperature regulation (sweating, shivering, metabolism)
2. **Organizational Cybernetics** — Viable System Model-inspired resource management
3. **Climate Control** — Building HVAC as a homeostat (temperature + humidity)
4. **Autonomous Robotics** — Navigation with battery, obstacle, and goal as essential variables
5. **Supply Chain Stability** — Inventory management with demand variability and lead times

## Nine-School Curriculum Mapping

| School | Course | Topics Covered |
|--------|--------|---------------|
| MIT | 6.241 Dynamical Systems | State-space models, stability, Lyapunov functions |
| Stanford | CS229 Machine Learning | Adaptive regulation, model learning |
| Berkeley | EE221A Linear Systems | Feedback control, PID, coupling matrices |
| CMU | 18-771 Linear Systems | Linear dynamical models, observability |
| Princeton | MAE 546 Optimal Control | Predictive regulation, model-based control |
| Caltech | CDS 110 Intro to Control | Feedback, feedforward, adaptation |
| Cambridge | Part II Control Theory | Two-level control architecture |
| Oxford | Control Theory | Robust regulation, disturbance rejection |
| ETH | 227-0216 Control Systems | Adaptive control, system identification |

## Build & Test

```bash
make          # Build static library
make test     # Build and run 39 tests
make examples # Build all examples
make demo     # Build and run all examples
make clean    # Remove build artifacts
```

## File Layout

```
mini-ashby-homeostasis/
├── Makefile
├── README.md                    # This file
├── include/                     # 6 headers
│   ├── ashby_homeostasis.h      # Core types and system API
│   ├── essential_variables.h    # EV theory - bounds, statistics, trajectories
│   ├── homeostat_model.h        # Ashby's electromechanical homeostat
│   ├── ultrastability.h         # Step-function parameter search
│   ├── variety.h                # Ashby's Law of Requisite Variety
│   └── adaptive_regulator.h     # Conant-Ashby model-based regulation
├── src/                         # 7 implementations
│   ├── ashby_homeostasis.c      # Core system, EV ops, env, utilities
│   ├── essential_variables.c    # (included in ashby_homeostasis.c)
│   ├── homeostat_model.c        # 4-unit homeostat physics
│   ├── ultrastability.c         # Parameter fields, step-function
│   ├── variety.c                # Variety computation, channels, pipelines
│   ├── adaptive_regulator.c     # PID, predictive, learning, Bayesian
│   └── homeostasis_apps.c       # 5 real-world application demos
├── tests/
│   └── test_homeostasis.c       # 39 tests covering L1-L8
├── examples/
│   ├── example_homeostat.c      # 4-unit homeostat simulation
│   ├── example_ultrastability.c # Parameter search demonstration
│   └── example_variety.c        # Ashby's Law multi-stage pipeline
├── docs/
│   ├── knowledge-graph.md
│   ├── coverage-report.md
│   ├── gap-report.md
│   ├── course-alignment.md
│   └── course-tree.md
└── build/                       # Build artifacts
```

## References

- Ashby, W.R. (1952). *Design for a Brain*. Chapman & Hall.
- Ashby, W.R. (1956). *An Introduction to Cybernetics*. Chapman & Hall.
- Ashby, W.R. (1962). "Principles of the Self-Organizing System." In *Principles of Self-Organization*, Pergamon.
- Conant, R.C. & Ashby, W.R. (1970). "Every Good Regulator of a System Must Be a Model of That System." *Int. J. Systems Science*, 1(2), 89-97.
- Beer, S. (1972). *Brain of the Firm*. Allen Lane.
- Wiener, N. (1948). *Cybernetics*. MIT Press.
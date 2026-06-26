# Course Tree — Mini Ashby Homeostasis

## Dependency Graph (What You Need Before This Module)

```
                          ┌─────────────────────┐
                          │ mini-ashby-          │
                          │ homeostasis          │
                          └──────────┬──────────┘
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
    ┌─────────▼─────────┐  ┌────────▼────────┐  ┌──────────▼──────────┐
    │ mini-wiener-      │  │ mini-complex-   │  │ mini-autopoiesis-    │
    │ cybernetics       │  │ adaptive-system │  │ theory               │
    │ (feedback,        │  │ (emergence,     │  │ (self-production,    │
    │  control)         │  │  agents)        │  │  structural coupling)│
    └─────────┬─────────┘  └────────┬────────┘  └──────────┬──────────┘
              │                      │                      │
    ┌─────────▼──────────────────────▼──────────────────────▼─────────┐
    │ 0. mini-general-system-theory                                   │
    │ (system definitions, state-space, hierarchies, thermodynamics)  │
    └─────────────────────────────────────────────────────────────────┘
                                     │
    ┌────────────────────────────────▼────────────────────────────────┐
    │ Prerequisites: Calculus, Linear Algebra, ODEs, Probability      │
    └─────────────────────────────────────────────────────────────────┘
```

## Knowledge Dependencies Within This Module

```
L1: Definitions (EssentialVariable, HomeostatUnit, Variety, ...)
 │
 ├──► L2: Core Concepts (feedback, ultrastability, trial-and-error)
 │     │
 │     ├──► L3: Math Structures (matrices, time-series, state-space)
 │     │     │
 │     │     ├──► L4: Fundamental Laws
 │     │     │     ├── Ashby's Law of Requisite Variety
 │     │     │     ├── Conant-Ashby Theorem
 │     │     │     └── Ultrastability Convergence
 │     │     │
 │     │     ├──► L5: Algorithms
 │     │     │     ├── Homeostat ODE simulation
 │     │     │     ├── Step-function parameter search
 │     │     │     ├── PID/Adaptive/Predictive/Learning regulation
 │     │     │     ├── Bayesian parameter belief
 │     │     │     └── Variety estimation
 │     │     │
 │     │     └──► L6: Canonical Problems
 │     │           ├── Homeostat stabilization
 │     │           ├── Ultrastable parameter search
 │     │           └── Multi-stage variety pipeline
 │     │
 │     ├──► L7: Applications
 │     │     ├── Biological thermoregulation
 │     │     ├── Organizational cybernetics
 │     │     ├── Building climate control
 │     │     ├── Robot navigation
 │     │     └── Supply chain stability
 │     │
 │     └──► L8: Advanced Topics
 │           ├── Bayesian adaptive regulation
 │           ├── Parameter sensitivity analysis
 │           └── Multi-level ultrastability (partial)
 │
 └──► L9: Research Frontiers (documented)
       ├── Homeostasis in AI alignment
       ├── Quantum homeostat concepts
       └── Meta-ultrastability
```

## Postrequisites (Modules That Depend on This)

```
min-ashby-homeostasis
 │
 ├──► mini-second-order-cybernetics
 │    (observing systems, von Foerster)
 │
 ├──► mini-viable-system-model (Stafford Beer)
 │    (applications of homeostasis to management)
 │
 ├──► mini-self-organizing-system
 │    (Ashby's principle of self-organization)
 │
 └──► mini-control-philosophy
      (philosophical implications of homeostatic regulation)
```

## Key Equations Chain

```
Ashby's Definition:
  "A system is homeostatic iff all essential variables remain within
   their physiological limits"
       │
       ▼
Essential Variable Bounds:
  ∀i: lower_i ≤ EV_i(t) ≤ upper_i
       │
       ▼
Law of Requisite Variety:
  V(R) ≥ V(D) - V(E)
  where V(X) = log₂(|X|) = -∑ p(x) log₂ p(x)
       │
       ▼
Conant-Ashby Theorem:
  regulator_quality ∝ model_fidelity
  model_fidelity = cos_sim(regulator_params, system_params)
       │
       ▼
Ultrastability Search Complexity:
  E[trials] ≈ log(1-P_success) / log(1 - 1/|configs|)
```

## Learning Path

1. Start with `include/ashby_homeostasis.h` — understand all type definitions (L1)
2. Read `docs/knowledge-graph.md` — map concepts to implementations
3. Study `src/ashby_homeostasis.c` — essential variables and system dynamics (L2-L3)
4. Study `src/homeostat_model.c` — electromechanical homeostat physics (L4)
5. Study `src/ultrastability.c` — parameter search mechanism (L4-L5)
6. Study `src/variety.c` — Ashby's Law implementation (L4, L6)
7. Study `src/adaptive_regulator.c` — Conant-Ashby theorem and regulation (L5, L7)
8. Run `make test` — verify understanding through 39 tests
9. Run `make demo` — see real-world application demonstrations
10. Read `docs/gap-report.md` — understand research frontiers (L9)
# Course Tree — Mini-Autopoiesis-Theory

## Prerequisite Dependency Tree

```
Level 0: Mathematical Foundations
  ├── Linear algebra (matrix operations, eigenvalues)
  ├── Ordinary differential equations (ODE theory)
  ├── Graph theory (cycles, connectivity, closure)
  └── Elementary probability (random emergence)

Level 1: Core Definitions (L1)
  ├── Autopoietic system definition → ap_system_t
  ├── Component types and roles → ap_component_t
  ├── Production relations → ap_production_t
  ├── Boundary topology → ap_boundary_t
  └── Reaction network → rn_network_t

Level 2: Core Concepts (L2)
  ├── Operational closure ← L1 components + productions
  ├── Structural determinism ← L1 boundary + environment
  ├── Autocatalysis ← L1 reactions + catalysis
  ├── Consensual domain ← L1 coupling + interactions
  └── Organization invariance ← L1 system identity

Level 3: Mathematical Structures (L3)
  ├── Stoichiometric matrix S ← L1 reactions + species
  ├── Production adjacency ← L1 production network
  ├── Catalytic graph ← L2 autocatalysis
  └── Transitive closure ← L2 operational closure

Level 4: Fundamental Theorems (L4)
  ├── Autopoiesis criteria (Maturana-Varela) ← L1+L2
  ├── Organizational closure theorem ← L3 closure + L2
  ├── Production closure theorem ← L3 transitive closure
  ├── Catalytic closure theorem ← L3 catalytic graph
  ├── RAF theorem (Hordijk-Steel) ← L3 reaction network
  ├── Hypercycle stability (Eigen) ← L2 autocatalysis
  └── Structural coupling dynamics ← L2 structural determinism

Level 5: Algorithms (L5)
  ├── RAF detection ← L4 RAF theorem
  ├── Closure fixed-point ← L4 closure theorems
  ├── RK4 integration ← L3 ODE structures
  ├── QR eigenvalues ← L3 linear algebra
  └── Steady-state detection ← L3 ODE + L4 equilibrium

Level 6: Canonical Problems (L6)
  ├── Minimal autopoietic cell ← L4 autopoiesis criteria
  ├── Chemoton assembly ← L4 Gánti criteria + L5 ODE
  ├── Hypercycle dynamics ← L4 Eigen stability + L5 ODE
  ├── Error catastrophe ← L4 hypercycle + L4 eigenvalues
  └── RAF emergence ← L5 RAF detection + L8 random

Level 7: Applications (L7)
  ├── Social autopoiesis ← L4 structural coupling + L2 consensual
  ├── Niche construction ← L4 coupling + L2 environment
  └── Organizational coupling ← L4 organizational closure

Level 8: Advanced Topics (L8)
  ├── Quasispecies ← L4 hypercycle + L4 eigenvalues
  ├── Spatial hypercycle ← L4 hypercycle + diffusion PDE
  ├── Random RAF emergence ← L5 RAF + L7 stochastic
  └── Adaptive integration ← L5 RK4 + error control

Level 9: Research Frontiers (L9)
  ├── Multi-scale autopoiesis ← L7 social + L8
  ├── ALife autopoiesis ← L6 minimal cell + L8
  └── Chemical Organization Theory ← L4 RAF + L5 closure
```

## Key Dependencies

| Module | Depends On | Provides To |
|--------|-----------|-------------|
| `autopoiesis.c` | (none) | L1-L4 core types |
| `reaction_network.c` | (none) | L1-L4 reaction types |
| `stoichiometry.c` | (none) | L3-L5 matrix methods |
| `organizational_closure.c` | reaction_network types | L4-L5 closure algorithms |
| `autocatalytic_set.c` | reaction_network types | L4-L5 RAF detection |
| `structural_coupling.c` | autopoiesis types | L2-L7 coupling dynamics |
| `chemoton.c` | (none, self-contained) | L4-L6 chemoton model |
| `hypercycle.c` | (none, self-contained) | L4-L8 hypercycle model |

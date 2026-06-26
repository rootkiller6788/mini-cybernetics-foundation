# Course Tree — mini-complex-adaptive-system

## Prerequisites (Dependencies)

### Required Foundation
- **Basic calculus**: gradients, ODEs → `Langevin dynamics`, `gradient ascent`
- **Linear algebra**: vectors, matrices, eigenvalues → `state space`, `laplacian_spectrum`
- **Probability theory**: distributions, expectations → `master equation`, `entropy`
- **Graph theory**: adjacency, paths, connectivity → `network_analysis`

### Recommended Prerequisites
- **Evolutionary biology**: selection, fitness, variation → `GA`, `Price equation`
- **Information theory**: Shannon entropy, mutual information → `emergence_detection`
- **Statistical mechanics**: ensembles, phase transitions → `SOC`, `mean_field`

## Knowledge Dependency Graph

```
L1: Definitions (no prerequisites)
├── Agent, Internal Model, Tag, Building Block, Fitness
├── NK Landscape, Genotype, Population, Network
│
├──> L2: Core Concepts
│    ├── OODA Loop ← Agent
│    ├── Bounded Rationality ← Internal Model
│    ├── Emergence ← Population + Network
│    ├── Co-evolution ← Agent + Fitness
│    └── Self-Organization ← Population
│
├──> L3: Mathematical Structures
│    ├── State Space R^n ← Agent
│    ├── Boolean Hypercube ← NK Landscape
│    ├── Graph Laplacian ← Network
│    └── Schema Template ← Building Block
│
├──> L4: Fundamental Laws
│    ├── NK Correlation ← NK Landscape (L1) + Hypercube (L3)
│    ├── Complexity Catastrophe ← NK Landscape + Adaptive Walk
│    ├── Schema Theorem ← Schema (L3) + GA (L5)
│    ├── Price Equation ← Fitness (L1) + Population (L2)
│    ├── Fisher's Theorem ← Fitness + Selection
│    ├── Sandpile SOC ← Statistical mechanics
│    └── Causal Emergence ← Information theory
│
├──> L5: Algorithms
│    ├── NK Adaptive Walk ← NK Landscape (L1)
│    ├── Genetic Algorithm ← Schema (L3) + Selection
│    ├── TD Learning ← Internal Model (L1) + OODA (L2)
│    ├── Louvain Communities ← Network (L1) + Graph (L3)
│    ├── Brandes Betweenness ← Network + BFS
│    ├── Transfer Entropy ← Information theory
│    └── Lyapunov Exponent ← State space + ODE
│
└──> L6: Canonical Problems ← All L1-L5
     ├── NK Optimization
     ├── El Farol Bar
     ├── Minority Game
     ├── Sandpile SOC
     └── GA on NK Landscape
```

## Upward Dependencies (What depends on this module)

```
mini-complex-adaptive-system
├──> mini-agent-based-modeling (future module)
├──> mini-evolutionary-dynamics (future module)
├──> mini-network-science (future module)
├──> mini-self-organized-criticality (future module)
└──> L7-L9 applications in:
     ├── Computational economics
     ├── Ecological modeling
     ├── Organizational science
     └── Artificial life
```

## Course Sequence

1. **Week 1-2**: L1 — Agent definitions, Holland's 7 basics
2. **Week 3-4**: L2 — OODA loop, emergence, co-evolution
3. **Week 5-6**: L3 — NK Boolean hypercube, graph Laplacian, information theory
4. **Week 7-8**: L4 — Key theorems (NK correlation, Schema Theorem, Price Eq.)
5. **Week 9-10**: L5 — Algorithms (GA, TD learning, community detection)
6. **Week 11-12**: L6 — Canonical problems and examples

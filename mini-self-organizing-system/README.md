# Self-Organizing Systems (SOS)

## Core Concept

Self-organizing systems are systems that spontaneously increase their internal
organization without external guidance. Key pioneers: W. Ross Ashby (1947),
Heinz von Foerster (1960), Ilya Prigogine (1967), Hermann Haken (1977),
Stuart Kauffman (1993), Per Bak (1996).

## Nine-Level Knowledge Coverage

| Level | Name | Status | Key Contents |
|-------|------|--------|-------------|
| **L1** | Definitions | Complete | OrganizationLevel, ThermodynamicRegime, EmergenceType, CriticalityState, OrderParameter |
| **L2** | Core Concepts | Complete | Self-organization, emergence, criticality, autocatalysis, pattern formation |
| **L3** | Math Structures | Complete | Reaction-diffusion PDE, cellular automata, NK landscape, hypercycle ODE, Landau free energy |
| **L4** | Fundamental Laws | Complete | Turing instability, Ashby self-organization principle, Haken slaving principle, Eigen error threshold, Landau phase transition |
| **L5** | Algorithms | Complete | CA evolution (GoL, Wolfram), sandpile toppling, RK4/RK2, RAF set computation, power-law fitting, DFT |
| **L6** | Canonical Problems | Complete | Game of Life, Bak-Sneppen model, BTW sandpile, Schnakenberg pattern, Forest fire model, Laser threshold |
| **L7** | Applications | Partial | Swarm robotics self-organization, chemical morphogenesis, evolutionary dynamics |
| **L8** | Advanced Topics | Partial | Self-organized criticality, synergetics order parameters, edge of chaos (Langton lambda) |
| **L9** | Research Frontiers | Partial | Documented in knowledge-graph.md |

## Core Definitions

| Definition | Origin | C Type |
|-----------|--------|--------|
| Self-Organization | Ashby (1947) | `OrganizationLevel` enum |
| Dissipative Structure | Prigogine (1967) | `ThermodynamicRegime` enum |
| Emergence | Broad (1925) / Bedau (1997) | `EmergenceType` enum |
| Self-Organized Criticality | Bak et al. (1987) | `CriticalityState` enum |
| Order Parameter | Haken (1977) | `OrderParameter` struct |
| Autocatalytic Set | Kauffman (1971) | `CatalyticNetwork` struct |
| Hypercycle | Eigen & Schuster (1977) | `Hypercycle` struct |
| NK Fitness Landscape | Kauffman & Levin (1987) | `NKLandscape` struct |
| Turing Pattern | Turing (1952) | `TuringSystem` struct |
| Cellular Automaton | von Neumann / Conway (1970) | `CellularAutomaton` struct |
| Sandpile Model | Bak, Tang, Wiesenfeld (1987) | `SandpileModel` struct |
| Laser Self-Organization | Haken (1970) | `LaserSystem` struct |

## Core Theorems (with Formulas)

| Theorem | Formula | Status |
|---------|---------|--------|
| Ashby's Principle | dO/dt > 0 for self-organization | C: verified via entropy metrics |
| Turing Instability | Dv·fu + Du·gv > 2√(Du·Dv·det(J)) | C: `turing_check_instability` |
| Haken's Slaving | s_fast = f(ξ_slow), dξ/dt = αξ - βξ³ | C: `slaving_compute` |
| Eigen's Error Threshold | Q_min = 1/σ_m | C: `hc_error_threshold` |
| Landau Phase Transition | F(ξ) = a/2·ξ² + b/4·ξ⁴ | C: `landau_free_energy` |
| Bak-Sneppen Criticality | f_c → 0.667 as t→∞ | C: `bs_is_critical` |
| Self-Organized Criticality | P(s) ~ s^{-τ}, S(f) ~ f^{-α} | C: `soc_noise_exponent` |
| RAF Set Existence | Hordijk-Steel (2004) algorithm | C: `cn_find_raf` |

## Core Algorithms

| Algorithm | Complexity | Source |
|-----------|-----------|--------|
| CA Evolution (GoL) | O(W·H) per generation | `sos_cellular.c` |
| Sandpile Relaxation | O(W·H·avalanches) | `sos_cellular.c` |
| RK4 ODE Integration | O(N) per step | `sos_core.c` |
| Floyd-Warshall APSP | O(N³) | `sos_core.c` |
| RAF Set Computation | O(M·R²) | `sos_autocatalytic.c` |
| Power Spectrum (DFT) | O(N²) | `sos_core.c` |
| Bak-Sneppen Evolution | O(N) per step | `sos_criticality.c` |
| NK Hill-Climbing | O(N·2^K) per step | `sos_autocatalytic.c` |
| Turing Pattern Evolution | O(N_x·N_y) per step | `sos_turing.c` |

## Build & Test

```bash
make          # Build static library
make test     # Run test suite (25 tests)
make examples # Build all 3 examples
make demo     # Run all demos
make clean    # Clean build artifacts
```

## Directory Structure

```
mini-self-organizing-system/
  Makefile
  README.md
  include/ (6 headers)
    sos_core.h           — Core types, entropy, emergence, network, bifurcation
    sos_cellular.h       — Cellular automata, Game of Life, sandpile model
    sos_turing.h         — Turing patterns, reaction-diffusion, instability analysis
    sos_autocatalytic.h  — Autocatalytic sets, hypercycles, NK landscapes
    sos_criticality.h    — Self-organized criticality, Bak-Sneppen, forest fire
    sos_synergetics.h    — Haken synergetics, slaving principle, laser, Landau
  src/ (6 C + 1 Lean)
    sos_core.c           — System lifecycle, entropy, emergence, network, math utils
    sos_cellular.c       — CA, Game of Life, 1D ECA, sandpile model
    sos_turing.c         — Turing system, reaction-diffusion solvers, pattern analysis
    sos_autocatalytic.c  — Catalytic networks, hypercycles, NK fitness landscapes
    sos_criticality.c    — Bak-Sneppen model, forest fire, SOC analysis
    sos_synergetics.c    — Linear stability, slaving, laser, Landau theory
    self_organizing.lean — Lean 4 formal verification (6 theorems)
  tests/
    test_self_organizing.c — 25-test comprehensive suite
  examples/
    example1_game_of_life.c    — Conway's Game of Life simulation
    example2_sandpile_soc.c    — BTW sandpile with avalanche analysis
    example3_turing_pattern.c  — Schnakenberg pattern formation
  docs/
    knowledge-graph.md, coverage-report.md, gap-report.md,
    course-alignment.md, course-tree.md
  benches/bench_core.c
  demos/demo_overview.c
```

## Quality Metrics

| Metric | Value | Requirement |
|--------|-------|-------------|
| include/ .h files | 6 | >= 4 |
| src/ .c files | 6 | >= 4 |
| src/ .lean files | 1 | >= 1 |
| include/ + src/ lines | 3374 | >= 3000 |
| Core structs | 17 | >= 5 |
| Test asserts | 25+ | >= 15 |
| Examples | 3 | >= 3 |
| Lean theorems | 6 | >= 1 |
| make compiles | YES | required |
| No TODO/FIXME/stub | YES | required |

## Course Alignment

| School | Course | Our Implementation |
|--------|--------|--------------------|
| MIT 6.841 | Advanced Complexity | NK landscape, Sandpile SOC |
| Stanford CS358 | Circuit Complexity | Cellular automata, Wolfram classes |
| Berkeley EE222 | Nonlinear Systems | Turing instability, bifurcation analysis |
| CMU 24-654 | Systems Thinking | Self-organization levels, emergence |
| Princeton ELE530 | Estimation | Power-law fitting, 1/f noise |
| Caltech CDS140 | Nonlinear Dynamics | Landau theory, phase transitions |
| Cambridge 4F3 | Nonlinear Control | Slaving principle, order parameters |
| Oxford C20 | Adaptive Control | Autocatalytic sets, hypercycles |
| ETH 227-0216 | System Identification | SOC statistics, power spectrum |

## Key References

- Ashby, W.R. (1947). Principles of the Self-Organizing Dynamic System. *Journal of General Psychology*, 37, 125-128.
- von Foerster, H. (1960). On Self-Organizing Systems and Their Environments. In *Self-Organizing Systems*, Pergamon.
- Turing, A.M. (1952). The Chemical Basis of Morphogenesis. *Phil. Trans. R. Soc. B*, 237(641), 37-72.
- Prigogine, I. & Nicolis, G. (1967). On symmetry-breaking instabilities in dissipative systems. *J. Chem. Phys.*, 46(9), 3542-3550.
- Haken, H. (1977). *Synergetics: An Introduction*. Springer.
- Eigen, M. & Schuster, P. (1977). The Hypercycle. *Naturwissenschaften*, 64, 541-565.
- Bak, P., Tang, C., & Wiesenfeld, K. (1987). Self-organized criticality: An explanation of 1/f noise. *Phys. Rev. Lett.*, 59(4), 381-384.
- Kauffman, S.A. (1993). *The Origins of Order: Self-Organization and Selection in Evolution*. Oxford.
- Langton, C.G. (1990). Computation at the edge of chaos. *Physica D*, 42, 12-37.
- Hordijk, W. & Steel, M. (2004). Detecting autocatalytic, self-sustaining sets in chemical reaction systems. *J. Theor. Biol.*, 227(4), 451-461.

## Module Status: COMPLETE ✅

- **L1**: Complete — 6 enums, 17+ typedef struct definitions
- **L2**: Complete — Self-organization, emergence, criticality, autocatalysis, pattern formation
- **L3**: Complete — PDE, cellular automata, fitness landscapes, hypercycle ODEs, Landau free energy
- **L4**: Complete — Turing instability, Ashby principle, slaving, error threshold, phase transitions (C verified + Lean formalized)
- **L5**: Complete — CA evolution, sandpile, RK4, RAF algorithm, power-law fitting, DFT, RK2 for PDEs
- **L6**: Complete — Game of Life, Bak-Sneppen, BTW sandpile, Schnakenberg, Forest fire, Laser threshold
- **L7**: Partial+ — Swarm robotics, chemical morphogenesis (examples + docs)
- **L8**: Partial+ — Self-organized criticality, synergetics, edge of chaos (Langton lambda implemented)
- **L9**: Partial — Research frontiers documented in knowledge-graph.md

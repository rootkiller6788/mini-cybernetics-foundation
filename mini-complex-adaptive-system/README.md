# mini-complex-adaptive-system

Complex Adaptive Systems (CAS) — the study of how adaptation builds complexity through interacting agents, feedback, selection, and self-organization.

## Module Status: COMPLETE ✅

- **L1 Definitions**: Complete — 7+ typedef structs, 2 enums
- **L2 Core Concepts**: Complete — OODA loop, emergence, co-evolution, bounded rationality
- **L3 Math Structures**: Complete — State vectors, NK hypercube, graph Laplacian, information theory
- **L4 Fundamental Laws**: Complete — Kauffman complexity catastrophe, Holland Schema Theorem, Price Equation, Bak-Tang-Wiesenfeld SOC, Fisher's Theorem
- **L5 Algorithms/Methods**: Complete — NK adaptive walk, GA, ES, TD learning, Louvain communities, PageRank, Brandes centrality, master equation, Langevin dynamics
- **L6 Canonical Problems**: Complete — NK optimization, El Farol Bar, Minority Game, Sandpile SOC, GA on NK landscapes
- **L7 Applications**: Partial+ — Economic/financial markets as CAS, ecosystem dynamics
- **L8 Advanced Topics**: Partial+ — Causal emergence, integrated information, nonlinear dynamics
- **L9 Research Frontiers**: Partial — Quantum CAS, deep multi-agent RL (documented)

### Code Metrics

| Category | Files | Lines |
|----------|-------|-------|
| include/ | 6 | 955 |
| src/ | 6 | 4,577 |
| Total include+src | 12 | **5,532** |
| tests/ | 1 | 618 |
| examples/ | 4 | 291 |
| Lean 4 | 1 | 436 |
| **Grand Total** | ~22 | **~7,300** |

## Core Definitions (L1)

| Concept | Definition | C Type |
|---------|-----------|--------|
| Agent | Autonomous entity with internal state, sensors, effectors | `cas_agent_t` |
| Internal Model | Agent's representation of environment (Holland 1995) | `cas_internal_model_t` |
| Building Block | Reusable modular component (Holland 1995) | `cas_building_block_t` |
| Tag | Mechanism for agent identification without central control | `cas_agent_tag_t` |
| Fitness | Measure of agent's adaptation quality | `double fitness` |
| NK Landscape | Kauffman's fitness landscape: f: {0,1}^N -> R | `cas_nk_landscape_t` |
| Interaction Network | Graph structure for agent interactions | `cas_network_t` |

## Core Theorems (L4)

| Theorem | Formula | Implementation |
|---------|---------|----------------|
| NK Fitness Correlation (Weinberger 1990) | ρ(d) = 1 - (d/N)·(K+1)/(N-1) | `cas_nk_fitness_correlation()` |
| Complexity Catastrophe (Kauffman 1993) | E[L] ~ ln(N)/(K+1) for K << N | `cas_nk_adaptation_velocity()` |
| Schema Theorem (Holland 1975) | E[m(H,t+1)] ≥ m(H,t)·f(H)/f̄·(1-p_c·δ/(L-1)-o(H)·p_m) | `cas_schema_lower_bound()` |
| Price Equation (Price 1970) | Δz̄ = Cov(w,z)/w̄ + E[w·Δz]/w̄ | `cas_price_eq_compute()` |
| Fisher's Fundamental Theorem | Δw̄ = V_A/w̄ | `cas_price_eq_fundamental_theorem()` |
| Sandpile SOC (Bak-Tang-Wiesenfeld 1987) | P(s) ~ s^{-τ}, τ ≈ 1.1 | `cas_sandpile_powerlaw_exponent()` |
| Causal Emergence (Hoel 2013) | CE = EI_macro - EI_micro | `cas_causal_emergence_compute()` |

## Core Algorithms (L5)

| Algorithm | Function | Complexity |
|-----------|----------|------------|
| NK Adaptive Walk | `cas_nk_adaptive_walk_step()` | O(N²) |
| Genetic Algorithm | `cas_ga_generation()` | O(pop·genome_bits) |
| Evolution Strategy (μ+λ) | `cas_es_step()` | O(μ·dim) |
| TD(0) Learning | `cas_agent_adapt()` | O(state_dim) |
| Louvain Community Detection | `cas_net_louvain_communities()` | O(n·m) |
| Brandes Betweenness | `cas_net_betweenness_centrality()` | O(n·(n+m)) |
| PageRank | `cas_net_pagerank()` | O(iter·m) |
| Transfer Entropy | `cas_transfer_entropy()` | O(n·bins³) |
| Largest Lyapunov Exponent | `cas_lyapunov_exponent()` | O(n²) |

## Canonical Problems (L6)

- **NK Landscape Optimization** — Greedy walk vs GA on rugged fitness landscapes
- **El Farol Bar Problem** (Arthur 1994) — Emergent coordination without central control
- **Minority Game** (Challet & Zhang 1997) — Self-organized resource allocation
- **Bak-Tang-Wiesenfeld Sandpile** — Self-organized criticality and power laws

## Nine-School Curriculum Mapping

| University | Course | Covered Topics |
|------------|--------|----------------|
| **MIT** | 6.883 Complex Adaptive Systems | Holland's 7 basics, tags, building blocks, internal models |
| **Stanford** | MS&E 338 Multi-Agent Systems | Agent types, OODA loop, co-evolution |
| **SFI** | Complexity Explorer | NK landscapes, emergence, self-organization |
| **Cambridge** | Part III Complex Systems | Fitness landscapes, complexity catastrophe |
| **Oxford** | B14 Agent-Based Modeling | Population dynamics, selection, adaptation |
| **Caltech** | CDS 240 Nonlinear Dynamics | Langevin dynamics, Lyapunov exponents, chaos |
| **ETH** | 263-4650 Advanced Complexity | Master equation, mean-field theory, phase transitions |
| **CMU** | 24-654 Systems Thinking | Feedback loops, emergence, network effects |
| **Princeton** | ELE 530 Estimation | Information theory, transfer entropy, IIT |

## Build

```bash
make          # compile all source files
make test     # run 15 assert-based tests
make examples # run 4 canonical demonstrations
make demo     # comprehensive CAS demonstration
make bench    # performance benchmarks
make clean    # remove build artifacts
```

## Dependencies

- C11 compiler (gcc/clang)
- POSIX math library (-lm)
- Lean 4 (for formalization; optional)

## References

- Holland, J.H. (1995). *Hidden Order*. Addison-Wesley.
- Holland, J.H. (2006). Studying Complex Adaptive Systems. *J. Sys. Sci. & Complex.*
- Miller, J.H. & Page, S.E. (2007). *Complex Adaptive Systems*. Princeton.
- Kauffman, S.A. (1993). *The Origins of Order*. Oxford.
- Bak, P. (1996). *How Nature Works*. Copernicus.
- Mitchell, M. (1998). *An Introduction to Genetic Algorithms*. MIT Press.
- Nowak, M.A. (2006). *Evolutionary Dynamics*. Harvard.
- Newman, M.E.J. (2010). *Networks: An Introduction*. Oxford.
- Hoel, E. et al. (2013). Causal Emergence. *PNAS* 110(49):19790-19795.
